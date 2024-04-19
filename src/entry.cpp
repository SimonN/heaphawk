#include "entry.h"
#include "common.h"
#include "snapshot.h"
#include <inttypes.h>
#include <array>

struct ValueDesc {
    typedef uint64_t Entry::*MemberPointer;

    ValueDesc(const std::string& name, MemberPointer ptr, size_t index) : mName(name), mMember(ptr), mIndex(index) {}

    void writeValue(std::ofstream& stream, uint32_t& flags, const Entry& curEntry, const Entry* prevEntry) const;

    void readValue(std::ifstream& stream, uint32_t flags, Entry& curEntry, const Entry* prevEntry) const;

    std::string mName;
    uint64_t Entry::*mMember;
    size_t mIndex;
};

void ValueDesc::writeValue(std::ofstream& stream, uint32_t& flags, const Entry& curEntry, const Entry* prevEntry) const {
    auto value = curEntry.*mMember;
    if (prevEntry) {
        auto prevValue = prevEntry->*mMember;
        if (value == prevValue) {
            return;
        }
    }
    flags |= (1 << mIndex);
    writeUInt64(stream, value);
}

void ValueDesc::readValue(std::ifstream& stream, uint32_t flags, Entry& curEntry, const Entry* prevEntry) const {
    if (flags & (1 << mIndex)) {
        uint64_t value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
        curEntry.*mMember = value;
    } else {
        curEntry.*mMember = prevEntry->*mMember;
    }
}


#define MAKE_VALUE(name, index) ValueDesc(#name, &Entry::m##name, index)

const std::array<ValueDesc, 22>& valueDescs() {
    static const std::array<ValueDesc, 22> ValueDescs = {
        MAKE_VALUE(Size, 0),
        MAKE_VALUE(KernelPageSize, 1),
        MAKE_VALUE(MMUPageSize, 2),
        MAKE_VALUE(Rss, 3),
        MAKE_VALUE(Pss, 4),
        MAKE_VALUE(Pss_Dirty, 5),
        MAKE_VALUE(Shared_Clean, 6),
        MAKE_VALUE(Shared_Dirty, 7),
        MAKE_VALUE(Private_Clean, 8),
        MAKE_VALUE(Private_Dirty, 9),
        MAKE_VALUE(Referenced, 10),
        MAKE_VALUE(Anonymous, 11),
        MAKE_VALUE(KSM, 12),
        MAKE_VALUE(LazyFree, 13),
        MAKE_VALUE(AnonHugePages, 14),
        MAKE_VALUE(ShmemPmdMapped, 15),
        MAKE_VALUE(Shared_Hugetlb, 16),
        MAKE_VALUE(Private_Hugetlb, 17),
        MAKE_VALUE(Swap, 18),
        MAKE_VALUE(SwapPss, 19),
        MAKE_VALUE(Locked, 20),
        MAKE_VALUE(FilePmdMapped, 21)
    };

    return ValueDescs;
}



// example: "1084 kB"
Entry::ParseResult Entry::parseValue(const std::string& name, const std::string& valueAndUnit) {

    const ValueDesc* valueDesc = nullptr;
    for (const auto& desc : valueDescs()) {
        if (desc.mName == name) {
            valueDesc = &desc;
            break;
        }
    }

    if (!valueDesc) {
        //printf("warning: value %s not found in descs\n", name.c_str());
        return ParseResult::unknown;
    }

    auto parts = splitString(valueAndUnit);

    if (parts.size() != 2) {
        printf("parts != 2 (%d)\n", static_cast<int>(parts.size()));
        return ParseResult::error;
    }

    if (parts[1] != "kB") {
        printf("parts 1 is not kB but \"%s\"", parts[1].c_str());
        return ParseResult::error;
    }

    uint64_t value;
    auto rd = sscanf(parts[0].c_str(), "%" PRIu64, &value);
    if (rd != 1) {
        printf("failed to parse value %s\n", parts[0].c_str());
        return ParseResult::error;
    }

    this->*valueDesc->mMember = value;
    return ParseResult::ok;
}

bool Entry::write(std::ofstream& stream, const Entry* prevEntry) const {
    bool ok = true;
    writeInt32(stream, 0x12563478); // sync
    
    writeUInt64(stream, mFrom);
    writeUInt64(stream, mTo);
    writeString(stream, mPermissions);
    writeUInt64(stream, mOffset);
    writeString(stream, mDevice);
    writeString(stream, mPathName);

    uint32_t flags = 0;
    auto flagsPos = stream.tellp();

    writeUInt32(stream, flags);
    for (const auto& desc : valueDescs()) {
       desc.writeValue(stream, flags, *this, prevEntry);
    }
    
    auto endPos = stream.tellp();

    stream.seekp(flagsPos);
    writeUInt32(stream, flags);
        
    stream.seekp(endPos);
    return ok;
}

bool Entry::read(std::ifstream& stream, const Snapshot* prevSnapshot) {
    bool ok = true;

    int sync;
    readInt32(stream, sync);
    if (sync != 0x12563478) {
        printf("out of sync at %x %d\n", sync, static_cast<int>(stream.tellg()));
    }

    readUInt64(stream, mFrom);
    readUInt64(stream, mTo);
    readString(stream, mPermissions);
    readUInt64(stream, mOffset);
    readString(stream, mDevice);
    readString(stream, mPathName);

    const Entry* prevEntry = nullptr;
    if (prevSnapshot) {
        prevEntry = prevSnapshot->findEntryByStartAddress(mFrom);
    }

    uint32_t flags = 0;
    readUInt32(stream, flags);

    for (const auto& desc : valueDescs()) {
        desc.readValue(stream, flags, *this, prevEntry);
    }

    return ok;
}

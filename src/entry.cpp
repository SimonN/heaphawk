#include "entry.h"
#include "common.h"
#include "snapshot.h"
#include <inttypes.h>
#include <array>

enum class Type {
    typeUInt64,
    typeString,
};

struct ValueDescBase {
    ValueDescBase(const std::string name, Type type, size_t index) : mName(name), mType(type), mIndex(index) {}

    virtual void writeValue(std::ofstream& stream, uint32_t& flags, const Entry& curEntry, const Entry* prevEntry) const = 0;

    virtual void readValue(std::ifstream& stream, uint32_t flags, Entry& curEntry, const Entry* prevEntry) const = 0;

    virtual bool equals(const Entry& a, const Entry& b) const = 0;

    std::string mName;

    Type mType;

    size_t mIndex;
};

template<class T>
struct ValueDesc : public ValueDescBase {

    using MemberPointer = T Entry::*;

    ValueDesc(const std::string& name, Type type, MemberPointer ptr, size_t index) : ValueDescBase(name, type, index), mMember(ptr) {}

    void writeValue(std::ofstream& stream, uint32_t& flags, const Entry& curEntry, const Entry* prevEntry) const override;

    void readValue(std::ifstream& stream, uint32_t flags, Entry& curEntry, const Entry* prevEntry) const override;

    virtual bool equals(const Entry& a, const Entry& b) const override;

    T Entry::*mMember;
};

template<>
void ValueDesc<uint64_t>::writeValue(std::ofstream& stream, uint32_t& flags, const Entry& curEntry, const Entry* prevEntry) const {
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

template<>
void ValueDesc<std::string>::writeValue(std::ofstream& stream, uint32_t& flags, const Entry& curEntry, const Entry* prevEntry) const {
    auto value = curEntry.*mMember;
    if (prevEntry) {
        auto prevValue = prevEntry->*mMember;
        if (value == prevValue) {
            return;
        }
    }
    flags |= (1 << mIndex);
    writeString(stream, value);
}

template<>
void ValueDesc<uint64_t>::readValue(std::ifstream& stream, uint32_t flags, Entry& curEntry, const Entry* prevEntry) const {
    if (flags & (1 << mIndex)) {
        uint64_t value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
        curEntry.*mMember = value;
    } else {
        curEntry.*mMember = prevEntry->*mMember;
    }
}

template<>
void ValueDesc<std::string>::readValue(std::ifstream& stream, uint32_t flags, Entry& curEntry, const Entry* prevEntry) const {
    if (flags & (1 << mIndex)) {
        std::string value;
        readString(stream, value);
        curEntry.*mMember = value;
    } else {
        curEntry.*mMember = prevEntry->*mMember;
    }
}

template<class T>
bool ValueDesc<T>::equals(const Entry& a, const Entry& b) const {
    return a.*mMember == b.*mMember;
}

template<>
bool ValueDesc<uint64_t>::equals(const Entry& a, const Entry& b) const {
    return (a.*mMember) == (b.*mMember);
}


#define MAKE_UINT64_VALUE(name, index) new ValueDesc<uint64_t>(#name, Type::typeUInt64, &Entry::m##name, index)
#define MAKE_STRING_VALUE(name, index) new ValueDesc<std::string>(#name, Type::typeString, &Entry::m##name, index)

const std::array<ValueDescBase*, 26>& valueDescs() {
    static const std::array<ValueDescBase*, 26> ValueDescs = {
        MAKE_UINT64_VALUE(From, 0),
        MAKE_UINT64_VALUE(To, 1),

        MAKE_STRING_VALUE(Permissions, 2),
        MAKE_UINT64_VALUE(Offset, 3),
        MAKE_STRING_VALUE(Device, 4),
        MAKE_STRING_VALUE(PathName, 5),

        MAKE_UINT64_VALUE(Size, 6),
        MAKE_UINT64_VALUE(KernelPageSize, 7),
        MAKE_UINT64_VALUE(MMUPageSize, 8),
        MAKE_UINT64_VALUE(Rss, 9),
        MAKE_UINT64_VALUE(Shared_Clean, 10),
        MAKE_UINT64_VALUE(Shared_Dirty, 11),
        MAKE_UINT64_VALUE(Private_Clean, 12),
        MAKE_UINT64_VALUE(Private_Dirty, 13),
        MAKE_UINT64_VALUE(Referenced, 14),
        MAKE_UINT64_VALUE(Anonymous, 15),
        MAKE_UINT64_VALUE(KSM, 16),
        MAKE_UINT64_VALUE(LazyFree, 17),
        MAKE_UINT64_VALUE(AnonHugePages, 18),
        MAKE_UINT64_VALUE(ShmemPmdMapped, 19),
        MAKE_UINT64_VALUE(Shared_Hugetlb, 20),
        MAKE_UINT64_VALUE(Private_Hugetlb, 21),
        MAKE_UINT64_VALUE(Swap, 22),
        MAKE_UINT64_VALUE(SwapPss, 23),
        MAKE_UINT64_VALUE(Locked, 24),
        MAKE_UINT64_VALUE(FilePmdMapped, 25)
    };

    return ValueDescs;
}

bool Entry::operator == (const Entry& other) const {
    for (const auto* desc : valueDescs()) {
        if (!desc->equals(*this, other)) {
            return false;
        }
    }

    return true;
}

bool Entry::operator != (const Entry& other) const {
    for (const auto* desc : valueDescs()) {
        if (!desc->equals(*this, other)) {
            return true;
        }
    }

    return false;
}

// example: "1084 kB"
Entry::ParseResult Entry::parseValue(const std::string& name, const std::string& valueAndUnit) {

    const ValueDescBase* valueDesc = nullptr;
    for (const auto* desc : valueDescs()) {
        if (desc->mName == name) {
            valueDesc = desc;
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

    if (valueDesc->mType == Type::typeUInt64) {
        auto vd = reinterpret_cast<const ValueDesc<uint64_t>*>(valueDesc);
        this->*vd->mMember = value;
    } else if (valueDesc->mType == Type::typeString) {
        auto vd = reinterpret_cast<const ValueDesc<std::string>*>(valueDesc);
        this->*vd->mMember = value;
    }

    return ParseResult::ok;
}

bool Entry::write(std::ofstream& stream, const Entry* prevEntry) const {
    bool ok = true;
    writeInt32(stream, 0x12563478); // sync

    writeUInt64(stream, mFrom);

    uint32_t flags = 0;
    auto flagsPos = stream.tellp();

    writeUInt32(stream, flags);
    for (const auto* desc : valueDescs()) {
        if (desc->mName == "From") {
            continue;
        }

        desc->writeValue(stream, flags, *this, prevEntry);
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

    const Entry* prevEntry = nullptr;
    if (prevSnapshot) {
        prevEntry = prevSnapshot->findEntryByStartAddress(mFrom);
    }

    uint32_t flags = 0;
    readUInt32(stream, flags);
    for (const auto* desc : valueDescs()) {
        if (desc->mName == "From") {
            continue;
        }

        desc->readValue(stream, flags, *this, prevEntry);
    }

    return ok;
}

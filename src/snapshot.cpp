#include "snapshot.h"
#include "common.h"
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <dirent.h>

Snapshot::Snapshot(pid_t processId, int64_t timestamp) {
    mProcessId = processId;
    mTimestamp = timestamp;
}

Snapshot::~Snapshot() {
}

const Entry* Snapshot::findEntryByStartAddress(uint64_t startAddress) const {
    for (auto& entry : mEntries) {
        if (entry.mFrom == startAddress) {
            return &entry;
        }
    }

    return nullptr;
}

std::string Snapshot::getProcessName() {
    char buf[1024];
    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", mProcessId);

    auto f = fopen(buf, "rb");
    if (!f) {
        return {};
    }

    char cmdLine[1024];
    auto rd = fread(cmdLine, 1, sizeof(cmdLine) - 1, f);
    if (rd <= 0) {
        fclose(f);
        return {};
    }

    fclose(f);

    cmdLine[rd] = 0;
    return cmdLine;
}

bool Snapshot::parseHeadline(const std::string& headline, Entry& entry) {
    // from-to           permissions offset   device  inode      pathname
    // ffff0000-ffff1000 r-xp        00000000 00:00   0          [vectors]
    char permissions[256];
    char pathName[256];
    char device[256];
    int inode;

    memset(pathName, 0, sizeof(pathName));

    int rd = sscanf(headline.c_str(),
                    "%" PRIx64 "-%" PRIx64 " %255s %" PRIx64 "  %255s %d %255s",
                    &entry.mFrom,
                    &entry.mTo,
                    permissions,
                    &entry.mOffset,
                    device,
                    &inode,
                    pathName);

    if (rd != 6 && rd != 7) {
        printf("scanf: error reading headline (rd=%d)\n", rd);
        return false;
    }

    entry.mPermissions = permissions;
    entry.mDevice = device;
    entry.mPathName = pathName;

    return true;
}

bool Snapshot::isHeadline(const std::string& str) {
    uint64_t a, b;
    auto count = sscanf(str.c_str(), "%" PRIx64 "-%" PRIx64 , &a, &b);
    return count == 2;
}

std::string Snapshot::readLine(FILE* f) {
    char* line = nullptr;
    size_t lineLength;
    auto rd = getline(&line, &lineLength, f);

    std::string result;
    if (rd > 0) {
        result = line;
    }

    if (line) {
        free(line);
    }

    return result;
}

// 1084 kB
bool Snapshot::parseValue(const std::string& line, Entry& entry) {
    auto idx = line.find(':');
    if (idx == std::string::npos) {
        printf("missing : in value line \"%s\"\n", line.c_str());
        return false;
    }

    auto name = line.substr(0, idx);
    auto valueAndUnit = line.substr(idx + 1);

    auto result = entry.parseValue(name, valueAndUnit);


    return result != Entry::ParseResult::error;
}

bool Snapshot::writeToFile(std::ofstream& stream, const Snapshot* prevSnapshot) {
    // write version
    uint32_t version = 1;
    writeInt32(stream, version);

    // process id
    writeInt32(stream, mProcessId);

    // write timestamp
    writeUInt64(stream, mTimestamp);

    // write name
    writeString(stream, mName);

    // count
    int count = static_cast<int>(mEntries.size());
    writeInt32(stream, count);

    for (const auto& ent : mEntries) {
        const Entry* prevEntry = nullptr;
        if (prevSnapshot) {
            prevEntry = prevSnapshot->findEntryByStartAddress(ent.mFrom);
        }

        if (!ent.write(stream, prevEntry)) {
            return false;
        }
    }

    return true;
}

bool Snapshot::readFromFile(std::ifstream& stream, const std::map<pid_t, Snapshot*>& prevSnapshots) {
   
    // version
    int32_t version;
    readInt32(stream, version);    
    if (stream.eof()) {
        return false;
    }
    if (version != 1) {
        printf("Invalid snapshot version in history (%d) at %x\n", version, static_cast<int>(stream.tellg()));
        return false;
    }

    // process id
    readInt32(stream, mProcessId);
    
    // timestamp
    readInt64(stream, mTimestamp);

    // read name
    readString(stream, mName);

    // count
    int count;
    readInt32(stream, count);

    const Snapshot* prevSnapshot = nullptr;
    auto it = prevSnapshots.find(mProcessId);
    if (it != prevSnapshots.end()) {
        prevSnapshot = it->second;
    }
    
    for (int i = 0; i < count; i++) {
        Entry ent;
        if (!ent.read(stream, prevSnapshot)) {
            return false;
        }
        mEntries.push_back(ent);
    }

    return true;
}

int64_t Snapshot::calcHeapUsage() const {
    int64_t heapUsage = 0;
    for (const auto& entry : mEntries) {
        if (entry.mPathName == "[heap]") {
            heapUsage += entry.mReferenced;
        } else if (entry.mPathName.empty()) {
            heapUsage += entry.mReferenced;
        }
    }

    return heapUsage;
}

bool Snapshot::take() {
    mName = getProcessName();

    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/smaps", mProcessId);

    auto f = fopen(path, "r");
    if (!f) {
        printf("failed to open %s\n", path);
        return false;
    }

    bool result = true;
    auto line = readLine(f);
    while (!line.empty()) {
        Entry entry;
        if (!parseHeadline(line, entry)) {
            result = false;
            break;
        }

        while (true) {
            line = readLine(f);
            if (line.empty()) {
                // end of file
                break;
            }
            if (isHeadline(line)) {
                break;
            }

            if (!parseValue(line, entry)) {
                result = false;
                break;
            }
        }
        if (!result) {
            break;
        }
        mEntries.push_back(entry);
    }

    fclose(f);

    return result;
}



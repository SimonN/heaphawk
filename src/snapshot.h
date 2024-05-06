#pragma once
#include "entry.h"
#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <map>

// Contains entries for one process at one point in time
class Snapshot {
public:
    enum class ReadFileResult {
        ok,
        failed,
        killed,
    };

    Snapshot() = default;

    Snapshot(pid_t processId, int64_t timestamp);

    ~Snapshot();

    pid_t processId() const { return mProcessId; }

    int64_t timestamp() const { return mTimestamp; }

    const std::string& name() const { return mName; }

    bool writeToFileKilled(std::ofstream& stream);

    bool writeToFile(std::ofstream& stream, const Snapshot* prevSnapshot);

    ReadFileResult readFromFile(std::ifstream& stream, const std::map<pid_t, Snapshot*>& prevSnapshots);

    const std::map<uint64_t, Entry>& entries() { return mEntries; }

    const std::map<uint64_t, Entry>& entries() const { return mEntries; }

    bool take();

    int64_t calcHeapUsage() const;

    const Entry* findEntryByStartAddress(uint64_t startAddress) const;

    bool isEqualTo(const Snapshot& other) const;

private:
    Snapshot(const Snapshot&) = delete;
    void operator= (const Snapshot&) = delete;

    static std::string readLine(FILE* f);

    static bool parseValue(const std::string& line, Entry& entry);

    static bool parseHeadline(const std::string& headline, Entry& entry);

    static bool isHeadline(const std::string& str);

    std::string getProcessName();

    pid_t mProcessId = 0;

    std::string mName;

    int64_t mTimestamp = 0;

    // entries by start address
    std::map<uint64_t, Entry> mEntries;
};

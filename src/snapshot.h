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
    Snapshot() = default;

    Snapshot(pid_t processId, int64_t timestamp);

    ~Snapshot();

    pid_t processId() const { return mProcessId; }

    int64_t timestamp() const { return mTimestamp; }

    const std::string& name() const { return mName; }

    bool writeToFile(std::ofstream& stream, const Snapshot* prevSnapshot);

    bool readFromFile(std::ifstream& stream, const std::map<pid_t, Snapshot*>& prevSnapshots);

    const std::vector<Entry>& entries() { return mEntries; }

    bool take();

    int64_t calcHeapUsage() const;

    const Entry* findEntryByStartAddress(uint64_t startAddress) const;

private:
    Snapshot(const Snapshot&) = delete;
    void operator= (const Snapshot&) = delete;

    static std::string readLine(FILE* f);

    static bool parseValue(const std::string& line, Entry& entry);

    static bool parseHeadline(const std::string& headline, Entry& entry);

    static bool isHeadline(const std::string& str);

    std::string getProcessName();

    pid_t mProcessId = 0;

    int64_t mTimestamp = 0;

    std::string mName;

    std::vector<Entry> mEntries;
};

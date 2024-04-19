#pragma once
#include "entry.h"
#include <map>
#include <string>

class Snapshot;

class Process {
public:
    Process(pid_t processId, const std::string& name);

    ~Process();

    pid_t processId() const { return mProcessId; }

    const std::string& name() const { return mName; }

    const std::string& shortName() const { return mShortName; }

    void addSnapshot(Snapshot* snapshot);

    const std::map<time_t, Snapshot*>& snapshots() const { return mSnapshots; }

    const Snapshot* firstSnapshot() const;

    const Snapshot* lastSnapshot() const;

private:
    pid_t mProcessId;

    std::string mName;

    std::string mShortName;

    std::map<time_t, Snapshot*> mSnapshots;
};

#include "process.h"
#include "snapshot.h"

Process::Process(pid_t processId, const std::string& name) {
    mProcessId = processId;
    mName = name;

    auto end = name.find(' ');
    mShortName = name.substr(0, end);
}

Process::~Process() {
    for (auto it : mSnapshots) {
        delete it.second;
    }
}

void Process::addSnapshot(Snapshot* snapshot) {
    mSnapshots[snapshot->timestamp()] = snapshot;
}

const Snapshot* Process::firstSnapshot() const {
    if (mSnapshots.empty()) {
        return nullptr;
    }

    return mSnapshots.begin()->second;
}

const Snapshot* Process::lastSnapshot() const {
    if (mSnapshots.empty()) {
        return nullptr;
    }

    return mSnapshots.rbegin()->second;
}

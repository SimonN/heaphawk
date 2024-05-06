#include "recorder.h"
#include "snapshot.h"
#include "entry.h"
#include "common.h"
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <fstream>
#include <memory>
#include <set>

Recorder::Recorder() {
}

Recorder::~Recorder() {
}

void Recorder::setSampleFilePath(const std::string& path) {
    mSampleFilePath = path;
}

void Recorder::setSampleInterval(std::chrono::seconds interval) {
    mSampleInterval = interval;
}

void Recorder::setSampleCount(std::optional<int> sampleCount) {
    mSampleCount = sampleCount;
}

bool Recorder::isPidDir(const struct dirent* entry) {
    const char* p;

    for (p = entry->d_name; *p; p++) {
        if (!isdigit(*p)) {
            return false;
        }
    }

    return true;
}

void Recorder::recordSnapshots(std::ofstream& stream, bool firstTake) {
    printf("taking snapshots\n");

    auto dir = opendir("/proc");
    if (!dir) {
        printf("failed to open /proc (errno=%d)\n", errno);
        exit(1);
    }

    auto timestamp = time(nullptr);

    int totalCount = 0;
    int changedCount = 0;
    int newCount = 0;

    std::set<pid_t> prevPids;
    for (const auto& it : mPrevSnapshots) {
        prevPids.insert(it.second->processId());
    }

    while (auto entry = readdir(dir)) {
        if (!isPidDir(entry)) {
            continue;
        }

        auto pid = atoi(entry->d_name);

        // ignore ourself
        if (pid == getpid()) {
            continue;
        }

        prevPids.erase(pid);

        auto snapshot = std::make_unique<Snapshot>(pid, timestamp);
        if(snapshot->take()) {
            Snapshot* prevSnapshot = nullptr;
            auto it = mPrevSnapshots.find(snapshot->processId());
            if (it != mPrevSnapshots.end()) {
                prevSnapshot = it->second.get();
                if (prevSnapshot->isEqualTo(*snapshot)) {
                    totalCount++;
                    continue;
                }
            } else {
                newCount++;
            }
            snapshot->writeToFile(stream, prevSnapshot);
            if (!firstTake) {
                printf("process %s [%d] changed\n", snapshot->name().c_str(), snapshot->processId());
            }
            mPrevSnapshots[snapshot->processId()] = std::move(snapshot);
        }


        changedCount++;
        totalCount++;
    }

    if (firstTake) {
        printf("took snapshots of %d processes\n", totalCount);
    } else {
        printf("took snapshots of %d processes, %d changed, %d new, %d removed\n", totalCount, changedCount, newCount, static_cast<int>(prevPids.size()));
    }

    for (auto pid : prevPids) {
        auto it = mPrevSnapshots.find(pid);
        it->second->writeToFileKilled(stream);
        mPrevSnapshots.erase(it);
    }

    closedir(dir);
}

void Recorder::record() {
    unlink(mSampleFilePath.c_str());

    std::ofstream stream(mSampleFilePath.c_str(), std::ofstream::binary | std::ofstream::ate | std::ofstream::out);
    if (!stream.is_open()) {
        printf("failed to open snapshots file %s\n", mSampleFilePath.c_str());
        exit(1);
    }

    constexpr uint32_t version = 1;
    writeUInt32(stream, version);

    int count = 0;
    while (true) {
        recordSnapshots(stream, count == 0);
        stream.flush();

        sleep(mSampleInterval.count());

        count++;
        if (mSampleCount
            && *mSampleCount == count) {
            break;
        }
    }
}


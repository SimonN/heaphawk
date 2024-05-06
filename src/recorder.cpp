#include "recorder.h"
#include "snapshot.h"
#include "entry.h"
#include "common.h"
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <fstream>
#include <memory>

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

void Recorder::recordSnapshots(std::ofstream& stream) {
    printf("taking snapshots\n");

    auto dir = opendir("/proc");
    if (!dir) {
        printf("failed to open /proc (errno=%d)\n", errno);
        exit(1);
    }
    
    auto timestamp = time(nullptr);
    
    int processCount = 0;
    while (auto entry = readdir(dir)) {
        if (!isPidDir(entry)) {
            continue;
        }

        auto pid = atoi(entry->d_name);

        // ignore ourself
        if (pid == getpid()) {
            continue;
        }

        auto snapshot = std::make_unique<Snapshot>(pid, timestamp);
        if(snapshot->take()) {
            Snapshot* prevSnapshot = nullptr;
            auto it = mPrevSnapshots.find(snapshot->processId());
            if (it != mPrevSnapshots.end()) {
                prevSnapshot = it->second.get();
            }
            snapshot->writeToFile(stream, prevSnapshot);
            mPrevSnapshots[snapshot->processId()] = std::move(snapshot);
        }
        processCount++;
    }
    printf("took snapshots of %d processes\n", processCount);

    closedir(dir);
}

void Recorder::record() {
    unlink(mSampleFilePath.c_str());
    
    std::ofstream stream(mSampleFilePath.c_str(), std::ofstream::binary | std::ofstream::ate | std::ofstream::out);
    if (!stream.is_open()) {
        printf("failed to open snapshots file %s\n", mSampleFilePath.c_str());
        exit(1);
    }

    int count = 0;
    while (true) {
        recordSnapshots(stream);
        stream.flush();

        sleep(mSampleInterval.count());

        count++;
        if (mSampleCount
            && *mSampleCount == count) {
            break;
        }
    }
}


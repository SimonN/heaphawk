#pragma once
#include "common.h"

#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <optional>
#include <chrono>
#include <map>
#include <memory>

class Snapshot;

class Recorder {
public:
    Recorder();

    ~Recorder();

    void record();

    void setSampleFilePath(const std::string& path);

    void setSampleInterval(std::chrono::seconds interval);

    void setSampleCount(std::optional<int> sampleCount);

private:
    static bool isPidDir(const struct dirent* entry);

    void recordSnapshots(std::ofstream& stream, bool firstTake);

    std::string mSampleFilePath = DEFAULT_SAMPLE_FILE_NAME;

    std::chrono::seconds mSampleInterval = std::chrono::minutes(1);

    std::optional<int> mSampleCount;

    std::map<pid_t, std::unique_ptr<Snapshot>> mPrevSnapshots;
};

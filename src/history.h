#pragma once
#include "common.h"
#include <map>
#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <string>

class Process;
class Snapshot;

class History {
public:
    History();

    ~History();

    void setSampleFilePath(const std::string& path);

    void load();

    void summary();

    void plot();

private:
    void readSnapshot(FILE* f);

    std::vector<Process*> processesSortedByGrowth();

    std::string mSampleFilePath = DEFAULT_SAMPLE_FILE_NAME;

    std::map<pid_t, Process*> mProcesses;

    std::map<pid_t, Snapshot*> mPrevSnapshots;
};

#include "history.h"
#include "snapshot.h"
#include "process.h"
#include "common.h"
#include <algorithm>
#include <chrono>
#include <fstream>

History::History() {
}

History::~History() {
    for (auto it : mProcesses) {
        delete it.second;
    }
}

void History::setSampleFilePath(const std::string& sampleFilePath) {
    mSampleFilePath = sampleFilePath;
}

void History::load() {
    std::ifstream stream(mSampleFilePath, stream.binary|stream.in);
    if (!stream.is_open()) {
        printf("failed to open archive file %s\n", mSampleFilePath.c_str());
        return;
    }

    int loadedSnapshotCount = 0;
    while (!stream.eof()) {
        auto snapshot = new Snapshot();
        if (!snapshot->readFromFile(stream, mPrevSnapshots)) {
            delete snapshot;
            if (stream.eof()) {
                break;
            }
            printf("failed to read snapshot from file\n");
            break;
        }

        auto it = mProcesses.find(snapshot->processId());
        Process* process;
        if (it == mProcesses.end()) {
            process = new Process(snapshot->processId(), snapshot->name());
            mProcesses[process->processId()] = process;
        } else {
            process = it->second;
        }

        process->addSnapshot(snapshot);

        mPrevSnapshots[snapshot->processId()] = snapshot;
        loadedSnapshotCount++;
    }

    printf("did load %d snapshots for %d processes\n", loadedSnapshotCount, static_cast<int>(mProcesses.size()));
}

struct SortHelper {
    int64_t mValue;
    Process* mProcess;

    SortHelper(Process* process, int64_t value) {
        mProcess = process;
        mValue = value;
    }

    bool operator < (const SortHelper& other) {
        return mValue > other.mValue;
    }
};

static std::string formatTimeInterval(std::chrono::seconds interval) {
    char buf[256];
    if (interval.count() < 60) {
        snprintf(buf, sizeof(buf), "%ds", static_cast<int>(interval.count()));
    } else if (interval.count() < 3600) {
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(interval);
        int partMinutes = interval.count() - minutes.count() * 60;
        snprintf(buf, sizeof(buf), "%02dm:%02ds", static_cast<int>(minutes.count()), partMinutes);
    } else {
        int hours = std::chrono::duration_cast<std::chrono::hours>(interval).count();
        int minutes = (interval.count() - hours * 3600) / 60;
        int partMinutes = interval.count() - hours * 3600 - minutes * 60;
        snprintf(buf, sizeof(buf), "%02dh:%02dm:%02ds", hours, minutes, partMinutes);

    }
    return std::string(buf);
}

std::vector<Process*> History::processesSortedByGrowth() {
      // sort by used heap
    std::vector<SortHelper> processes;
    for (auto it : mProcesses) {
        auto process = it.second;

        auto firstSnapshot = process->firstSnapshot();
        auto lastSnapshot = process->lastSnapshot();

        int64_t startSize = 0;
        int64_t endSize = 0;
        if (firstSnapshot && lastSnapshot && firstSnapshot != lastSnapshot) {
            startSize = firstSnapshot->calcHeapUsage();
            endSize = lastSnapshot->calcHeapUsage();
            int64_t deltaSize = endSize - startSize;
            if (deltaSize > 0) {
                processes.push_back(SortHelper(process, deltaSize));
            }
        }
    }

    std::sort(processes.begin(),
              processes.end());

    std::vector<Process*> list;
    for (auto it : processes) {
        list.push_back(it.mProcess);
    }

    return list;
}

void History::summary() {
    printf("summary:\n");

    // sort by heap growth
    auto processesSortedByGrowth = this->processesSortedByGrowth();
    if (processesSortedByGrowth.empty()) {
        printf("no processes with changing memory consumption found\n");
        return;
    }

    for (const auto& process : processesSortedByGrowth) {
        auto firstSnapshot = process->firstSnapshot();
        auto lastSnapshot = process->lastSnapshot();

        int64_t startSize = 0;
        int64_t endSize = 0;
        int64_t startTime = 0;
        int64_t endTime = 0;
        if (firstSnapshot && lastSnapshot && firstSnapshot != lastSnapshot) {
            startSize = firstSnapshot->calcHeapUsage();
            endSize = lastSnapshot->calcHeapUsage();
            startTime = firstSnapshot->timestamp();
            endTime = lastSnapshot->timestamp();
        }

        auto deltaTime = std::chrono::seconds(endTime - startTime);

        float growthPerDay = ((endSize - startSize) / static_cast<double>(deltaTime.count())) * 3600 * 24;

        printf("  [%d] %s: +%dkB heap in %s (~%.02fkB/day  %dkB - %dkB %d snapshots)\n",
               process->processId(),
               process->name().c_str(),
               static_cast<int>(endSize - startSize),
               formatTimeInterval(deltaTime).c_str(),
               growthPerDay,
               static_cast<int>(startSize),
               static_cast<int>(endSize),
               static_cast<int>(process->snapshots().size()));
    }
}

static std::string replaceInString(const std::string& str, const std::string& oldText, const std::string& newText) {
    size_t index = 0;
    std::string res = str;
    while (true) {
        index = res.find(oldText, index);
        if (index == std::string::npos) {
            break;
        }


        res.replace(index, oldText.length(), newText);

        index += newText.length();
    }
    return res;
}

void History::plot() {
    auto processesSortedByGrowth = this->processesSortedByGrowth();
    if (processesSortedByGrowth.empty()) {
        printf("no processes with changing memory consumption found\n");
        return;
    }

    const std::vector<std::string> colors = {
        "#0060ad",
        "#ad6000",
        "#60ad00",
        "#adad00",
        "#00adad"
    };

    std::ofstream plotFile("gnuplot.plt");

    plotFile << "set xlabel 'Time (hours:minutes)'\n";
    plotFile << "set ylabel 'Heap Consumption (kB)'\n";
    plotFile << "set xtics time format '%tH:%tM;'\n";

    int processCount = 1;
    for (const auto& process : processesSortedByGrowth) {
        auto firstSnapshot = process->firstSnapshot();

        // write data file
        char fileName[128];
        snprintf(fileName, sizeof(fileName), "process_%d.csv", static_cast<int>(process->processId()));
        std::ofstream csvFile(fileName);
        for (auto it : process->snapshots()) {
            auto snapshot = it.second;
            csvFile << (snapshot->timestamp() - firstSnapshot->timestamp()) << ", " << snapshot->calcHeapUsage() << "\n";
        }

        // write gnuplot file
        plotFile << "set style line " << processCount << " \\\n"
                    "    linecolor rgb \'" << colors[processCount % colors.size()] << "\' \\\n"
                    "    linetype 1 linewidth 2 \\\n"
                    "    pointtype 1 pointsize 1.5\n"
                    "\n";


        processCount++;
    }

    // plot commands
    plotFile << "plot ";

    processCount = 0;
    for (const auto process : processesSortedByGrowth) {
        // write data file
        char fileName[128];
        snprintf(fileName, sizeof(fileName), "process_%d.csv", static_cast<int>(process->processId()));

        if (processCount > 0) {
            plotFile << ", \\\n";
        }

        auto escapedTitle = replaceInString(process->shortName(), "_", "\\_");

        plotFile
            << "    '"
            << fileName
            << "' index 0 with lines linestyle "
            << (processCount + 1)
            << " title '"
            << escapedTitle
            << "'";

        processCount++;
    }


    printf("please run \"gnuplot -p gnuplot.plt\"\n");
}

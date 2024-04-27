#include "recorder.h"
#include "history.h"
#include "common.h"
#include <string.h>
#include <string>
#include <vector>
#include <optional>
#include <chrono>

void printHelp() {
    printf("usage: %s <command> [<args>]\n", APP_NAME);
    printf("\n");
    printf("commands:\n");
    printf("\n");
    printf("  record   Start recording memory samples\n");
    printf("  summary  Shows the summary of a sampling session\n");
    printf("\n");
}

void printRecordHelp() {
    printf("usage: %s record [<args>]\n", APP_NAME);
    printf("\n");
    printf("options:\n");
    printf("  --sample-file=<path>\n");
    printf("    Set path to sample file.\n");
    printf("  --sample-interval=<interval>\n");
    printf("    Set sampling interval in seconds(default=%d).\n", static_cast<int>(DEFAULT_SAMPLING_INTERVAL.count()));
    printf("  --sample-count=<count>\n");
    printf("    The number of samples to collect.\n");
    printf("  --include=<regexp>\n");
    printf("    Regexp describing the processes to include.\n");
    printf("  --exclude=<regexp>\n");
    printf("    Regexp describing the processes to exclude.\n");
}

void printSummaryHelp() {
    printf("usage: %s summary  [<args>]\n", APP_NAME);
    printf("\n");
    printf("options:\n");
    printf("  --sample-file=<path>\n");
    printf("    The path the the sample-file.\n");
}

void printPlotHelp() {
    printf("usage: %s plot [<args>]\n", APP_NAME);
    printf("\n");
    printf("options:\n");
    printf("  --sample-file=<path>\n");
    printf("    The path the the sample-file.\n");
}

void showErrorAndExit(const std::string& value) {
    printf("%s: %s\n", APP_NAME, value.c_str());
    exit(1);
}

bool tryToGetSwitchOption(char shortOption,
                          const std::string& longOption,
                          const std::vector<std::string>& args,
                          size_t& i) {

    if (args[i].length() == 2
        && args[i][0] == '-'
        && args[i][1] == shortOption) {
        return true;
    }

    std::string start = "--" + longOption;
    if (args[i] == start) {
        return true;
    }

    return false;
}

std::optional<std::string> tryToGetStringOption(char shortOption,
                                                const std::string& longOption,
                                                const std::vector<std::string>& args,
                                                size_t& i) {
    if (args[i].length() == 2
        && args[i][0] == '-'
        && args[i][1] == shortOption) {
        if (i >= args.size()) {
            showErrorAndExit("missing option");
        }

        i++;
        return args[i];
    }

    std::string start = "--" + longOption;
    if (args[i] == start) {
        if (i >= args.size()) {
            showErrorAndExit("missing option");
        }

        i++;
        return args[i];
    }

    start += "=";
    if (args[i].find(start) == 0) {
        return args[i].substr(start.length());
    }


    return {};
}

std::optional<int32_t> tryToGetOptionInt32Option(char shortOption,
                                                 const std::string& longOption,
                                                 const std::vector<std::string>& args,
                                                 size_t& i) {
    auto value = tryToGetStringOption(shortOption,
                                      longOption,
                                      args,
                                      i);
    if (!value) {
        return {};
    }

    // FIXME: check if value is convertible to int
    return atoi(value->c_str());
}

static std::string sampleFilePath;

void cmdHelp(const std::vector<std::string>& args) {
    if (args.empty()) {
        printHelp();
    } else if (args[0] == "record") {
        printRecordHelp();
    } else if (args[0] == "summary") {
        printSummaryHelp();
    } else {
        printHelp();
    }
}

void cmdRecord(const std::vector<std::string>& args) {

    Recorder recorder;

    for (size_t i = 0; i < args.size(); i++) {
        if (tryToGetSwitchOption('h', "help", args, i)) {
            printRecordHelp();
            exit(0);
        }

        auto sampleFile = tryToGetStringOption('\0', "sample-file", args, i);
        if (sampleFile) {
            recorder.setSampleFilePath(*sampleFile);
            continue;
        }

        auto sampleInterval = tryToGetOptionInt32Option('\0', "sample-interval", args, i);
        if (sampleInterval) {
            recorder.setSampleInterval(std::chrono::seconds(*sampleInterval));
            continue;
        }

        auto sampleCount = tryToGetOptionInt32Option('\0', "sample-count", args, i);
        if (sampleCount) {
            recorder.setSampleCount(*sampleCount);
            continue;
        }

        auto includeExp = tryToGetStringOption('\0', "include-exp", args, i);
        if (includeExp) {
            continue;
        }

        auto excludeExp = tryToGetStringOption('\0', "exclude-exp", args, i);
        if (excludeExp) {
            continue;
        }

        showErrorAndExit(std::string("invalid option ") + args[i]);
    }

    recorder.record();

}

void cmdSummary(const std::vector<std::string>& args) {

    History history;

    for (size_t i = 0; i < args.size(); i++) {

        auto sampleFile = tryToGetStringOption('\0', "sample-file", args, i);
        if (sampleFile) {
            history.setSampleFilePath(*sampleFile);
            continue;
        }
    }

    history.load(History::LoadHint::firstAndLast);
    history.summary();
}

void cmdPlot(const std::vector<std::string>& args) {
    History history;

    for (size_t i = 0; i < args.size(); i++) {

        auto sampleFile = tryToGetStringOption('\0', "sample-file", args, i);
        if (sampleFile) {
            history.setSampleFilePath(*sampleFile);
            continue;
        }
    }

    history.load(History::LoadHint::all);
    history.plot();
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(std::string(argv[i]));
    }

    if (args.empty()) {
        printHelp();
        return 1;
    }

    auto command = args[0];
    if (command == "record") {
        cmdRecord(std::vector<std::string>(args.begin() +1 , args.end()));
    } else if (command == "summary") {
        cmdSummary(std::vector<std::string>(args.begin() +1 , args.end()));
    } else if (command == "plot") {
        cmdPlot(std::vector<std::string>(args.begin() +1 , args.end()));
    } else if (command == "help") {
        cmdHelp(std::vector<std::string>(args.begin() + 1, args.end()));
    } else {
        showErrorAndExit("unknown command");
    }

   return 0;
}

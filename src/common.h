#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <stdint.h>

#define APP_NAME "heaphawk"

#define DEFAULT_SAMPLE_FILE_NAME "heaphawk.snapshots"

constexpr std::chrono::seconds DEFAULT_SAMPLING_INTERVAL = std::chrono::seconds(60);

std::vector<std::string> splitString(const std::string& s);

bool writeInt32(std::ofstream& stream, int32_t value);

bool writeUInt32(std::ofstream& stream, uint32_t value);

bool writeInt64(std::ofstream& stream, int64_t value);

bool writeUInt64(std::ofstream& stream, uint64_t value);

bool writeString(std::ofstream& stream, const std::string& value);

bool readInt32(std::ifstream& stream, int32_t& value);

bool readUInt32(std::ifstream& stream, uint32_t& value);

bool readInt64(std::ifstream& stream, int64_t& value);

bool readUInt64(std::ifstream& stream, uint64_t& value);

bool readString(std::ifstream& stream, std::string& value);


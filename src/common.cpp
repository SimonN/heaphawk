#include "common.h"

std::vector<std::string> splitString(const std::string& s)
{
    std::vector<std::string> parts;

    // " a  bv"
    size_t endPos = 0;
    while (true) {
        auto startPos = s.find_first_not_of("\t \r\n", endPos);
        if (startPos == std::string::npos) {
            return parts;
        }

        endPos = s.find_first_of("\t \r\n", startPos);
        if (endPos == std::string::npos) {
            auto str = s.substr(startPos);
            parts.push_back(str);
            return parts;
        }

        auto str = s.substr(startPos, endPos - startPos);
        parts.push_back(str);
    }

    return parts;
}


bool writeInt32(std::ofstream& stream, int32_t value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    return true;
}

bool writeUInt32(std::ofstream& stream, uint32_t value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    return true;
}

bool writeInt64(std::ofstream& stream, int64_t value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    return true;
}

bool writeUInt64(std::ofstream& stream, uint64_t value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    return true;
}

bool writeString(std::ofstream& stream, const std::string& value) {
    writeInt32(stream, value.length());
    stream.write(value.c_str(), value.length());
    return true;
}

bool readInt32(std::ifstream& stream, int32_t& value) {
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    return true;
}

bool readUInt32(std::ifstream& stream, uint32_t& value) {
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    return true;
}

bool readInt64(std::ifstream& stream, int64_t& value) {
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    return true;
}

bool readUInt64(std::ifstream& stream, uint64_t& value) {
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    return true;
}

bool readString(std::ifstream& stream, std::string& value) {
    int length = 0;
    if (!readInt32(stream, length)) {        
        return false;
    }

    value.clear();
    value.resize(length);

    stream.read(value.data(), length);
    return true;
}

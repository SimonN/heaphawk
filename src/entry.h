#pragma once
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <fstream>

class Snapshot;

class Entry {
public: 
    enum class ParseResult {
        ok,
        error,
        unknown,
    };

    bool write(std::ofstream& stream, const Entry* prevEntry) const;
    bool read(std::ifstream& stream, const Snapshot* prevSnapshot);
    ParseResult parseValue(const std::string& name, const std::string& valueAndUnit);

    uint64_t mFrom;
    uint64_t mTo;

    std::string mPermissions;
    uint64_t mOffset;
    std::string mDevice;
    std::string mPathName;

    uint64_t mSize = 0;
    uint64_t mKernelPageSize = 0;
    uint64_t mMMUPageSize = 0;
    uint64_t mRss = 0;
    uint64_t mPss = 0;
    uint64_t mPss_Dirty = 0;
    uint64_t mShared_Clean = 0;
    uint64_t mShared_Dirty = 0;
    uint64_t mPrivate_Clean = 0;
    uint64_t mPrivate_Dirty = 0;
    uint64_t mReferenced = 0;
    uint64_t mAnonymous = 0;
    uint64_t mKSM = 0;
    uint64_t mLazyFree = 0;
    uint64_t mAnonHugePages = 0;
    uint64_t mShmemPmdMapped = 0;
    uint64_t mShared_Hugetlb = 0;
    uint64_t mPrivate_Hugetlb = 0;
    uint64_t mSwap = 0;
    uint64_t mSwapPss = 0;
    uint64_t mLocked = 0;
    uint64_t mTHPeligible = 0;
    uint64_t mFilePmdMapped = 0;
};

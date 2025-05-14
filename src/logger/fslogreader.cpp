/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <unistd.h>

#include "logger/fslogreader.hpp"

namespace aos::zephyr::logger {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error FSLogReader::GetEntry(LogEntry& entry)
{
    LockGuard lock {mMutex};

    return ReadLine(entry);
}

bool FSLogReader::IsValid()
{
    LockGuard lock {mMutex};

    return mFD != -1 || !mLogFiles.IsEmpty();
}

Error FSLogReader::Reset()
{
    LockGuard lock {mMutex};

    CloseFile();

    mLogFiles.Clear();

    return ReadLogFiles();
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error FSLogReader::OpenNextFile()
{
    mCurrentPos = 0;
    mFD         = open(mLogFiles.begin()->CStr(), O_RDONLY);
    mLogFiles.Erase(mLogFiles.begin());

    if (mFD < 0) {
        return Error(ErrorEnum::eFailed, "failed to open log file");
    }

    return ErrorEnum::eNone;
}

void FSLogReader::CloseFile()
{
    if (mFD != -1) {
        close(mFD);
        mFD = -1;
    }

    mCurrentPos = 0;
}

Error FSLogReader::ReadLine(LogEntry& entry)
{
    if (mFD == -1) {
        if (mLogFiles.IsEmpty()) {
            return ErrorEnum::eNone;
        }

        if (auto err = OpenNextFile(); !err.IsNone()) {
            return err;
        }
    }

    if (auto err = fs::ReadLine(mFD, mCurrentPos, entry.mContent); !err.IsNone()) {
        CloseFile();

        return err;
    }

    mCurrentPos += entry.mContent.Size() + 1;

    if (auto [time, err] = Time::UTC(entry.mContent); err.IsNone()) {
        entry.mTime.SetValue(time);
    }

    return ErrorEnum::eNone;
}

Error FSLogReader::ReadLogFiles()
{
    mLogFiles.Clear();

    fs::DirIterator dirIterator(cLogDir);

    while (dirIterator.Next()) {
        if (dirIterator->mIsDir) {
            continue;
        }

        auto [pos, errFind] = dirIterator->mPath.FindSubstr(0, cLogPrefix);
        if (!errFind.IsNone() || pos != 0) {
            continue;
        }

        if (auto err = mLogFiles.EmplaceBack(); !err.IsNone()) {
            return err;
        }

        fs::AppendPath(mLogFiles.Back(), dirIterator.GetRootPath(), dirIterator->mPath);
    }

    mLogFiles.Sort();

    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::logger

/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGREADERSTUB_HPP_
#define LOGREADERSTUB_HPP_

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "logger/logreader.hpp"

namespace aos::zephyr::logger {

/**
 * Log reader stub.
 */
class LogReaderStub : public LogReaderItf {
public:
    /**
     * Reads log entry.
     *
     * @param entry log entry.
     * @return Error.
     */
    Error GetEntry(LogEntry& entry) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mLogEntries.empty()) {
            mValid = false;

            return ErrorEnum::eNotFound;
        }

        entry = mLogEntries.back();
        mLogEntries.pop_back();

        return ErrorEnum::eNone;
    }

    /**
     * Checks if reader is valid.
     *
     * @return bool.
     */
    bool IsValid() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        return mValid;
    }

    /**
     * Resets reader.
     *
     * @return Error.
     */
    Error Reset() override
    {
        mValid = true;

        return ErrorEnum::eNone;
    }

    /**
     * Sets is valid flag.
     *
     * @param valid valid flag.
     */
    void SetIsValid(bool valid)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mValid = valid;
    }

    /**
     * Sets log entries.
     *
     * @param logEntries log entries.
     */
    void SetLogEntries(const std::vector<LogEntry>& logEntries)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mLogEntries = logEntries;
    }

private:
    std::mutex            mMutex;
    bool                  mValid = false;
    std::vector<LogEntry> mLogEntries;
};

} // namespace aos::zephyr::logger

#endif

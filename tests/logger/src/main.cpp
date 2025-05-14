/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <fstream>
#include <memory>
#include <string>
#include <sys/statvfs.h>
#include <vector>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/fs.hpp>
#include <aos/common/tools/log.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/common/types.hpp>

#include "logger/fsbackend.hpp"
#include "logger/fslogreader.hpp"
#include "utils/log.hpp"
#include "utils/utils.hpp"

namespace aos::zephyr::logger {

namespace {

const auto cLogTime = Time::Unix(1706702400);

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

Error WriteLogToFile(const String& log, const String& path)
{
    auto f = std::ofstream(path.CStr(), std::ios::out | std::ios::app);
    if (!f.is_open()) {
        return ErrorEnum::eFailed;
    }

    f << log.CStr() << std::endl;

    return ErrorEnum::eNone;
}

void* Setup(void)
{
    Log::SetCallback(
        [](const aos::String&, aos::LogLevel, const aos::String& message) { printk("%s\n", message.CStr()); });

    return NULL;
}

void before_test(void* data)
{
    auto err = aos::fs::ClearDir(CONFIG_AOS_LOG_BACKEND_FS_DIR);
    zassert_true(err.IsNone(), "Failed to remove disk mount point: %s", utils::ErrorToCStr(err));
}

} // namespace

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(logger, nullptr, Setup, before_test, nullptr, nullptr);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

struct TestLogEntries {
    StaticString<1024>         mContent;
    Optional<Time>             mTime;
    StaticString<cFilePathLen> mLogFile;
};

ZTEST(logger, test_fslogreader)
{
    auto fsLogReader = std::make_unique<FSLogReader>();
    auto logEntry    = std::make_unique<LogEntry>();

    auto err = fsLogReader->Reset();
    zassert_true(err.IsNone(), "Failed to reset log reader: %s", utils::ErrorToCStr(err));

    // No log entries should be available

    err = fsLogReader->GetEntry(*logEntry);
    zassert_true(err.IsNone(), "Failed to get log entry: %s", utils::ErrorToCStr(err));
    zassert_true(logEntry->mContent.IsEmpty(), "Log entry should be empty");
    zassert_false(logEntry->mTime.HasValue(), "Log time should be empty");

    const std::vector<TestLogEntries> logs = {
        {"2024-01-31T12:00:00Z F1 Test message 1", {cLogTime}, fs::JoinPath(cLogDir, cLogPrefix).Append("1")},
        {"2024-01-31T12:00:00Z F1 Test message 2", {cLogTime}, fs::JoinPath(cLogDir, cLogPrefix).Append("1")},
        {"2024-01-31T12:00:00Z F1 Test message 3", {cLogTime}, fs::JoinPath(cLogDir, cLogPrefix).Append("1")},
        {"2024-01-31T12:00:00Z F1 Test message 4", {cLogTime}, fs::JoinPath(cLogDir, cLogPrefix).Append("1")},
        {"F2 Test message 1", {}, fs::JoinPath(cLogDir, cLogPrefix).Append("2")},
    };

    for (const auto& log : logs) {
        err = WriteLogToFile(log.mContent, log.mLogFile);
        zassert_true(err.IsNone(), "Failed to write log: %s", utils::ErrorToCStr(err));
    }

    err = fsLogReader->Reset();
    zassert_true(err.IsNone(), "Failed to reset log reader: %s", utils::ErrorToCStr(err));

    std::vector<LogEntry> readLogEntries;

    while (fsLogReader->IsValid()) {
        logEntry = std::make_unique<LogEntry>();

        err = fsLogReader->GetEntry(*logEntry);
        if (!err.IsNone()) {
            continue;
        }

        readLogEntries.push_back(*logEntry);
    }

    zassert_equal(readLogEntries.size(), logs.size(), "Log entries count mismatched");

    for (size_t i = 0; i < readLogEntries.size(); ++i) {
        const auto& readLog     = logs[i];
        const auto& expectedLog = readLogEntries[i];

        zassert_equal(readLog.mContent, expectedLog.mContent, "Log entry mismatched");
        zassert_equal(readLog.mTime, expectedLog.mTime, "Log time mismatched");
    }
}

} // namespace aos::zephyr::logger

/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
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
#include "logprovider/logprovider.hpp"
#include "stubs/logobserverstub.hpp"
#include "stubs/logreaderstub.hpp"
#include "utils/log.hpp"
#include "utils/utils.hpp"

namespace aos::zephyr::logprovider {

namespace {

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

const auto cFromTimeFilter = Time::Unix(1706702400); // "2024-01-31T12:00:00Z"
const auto cTillTimeFilter = cFromTimeFilter.Add(Time::cHours); // "2024-01-31T13:00:00Z"

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct logprovider_fixture {
    std::unique_ptr<logger::LogObserverStub> mLogObserver;
    std::unique_ptr<logger::LogReaderStub>   mLogReader;
    std::unique_ptr<LogProvider>             mLogProvider;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

logger::LogEntry CreateLogEntry(const String& content, Optional<Time> time = {})
{
    logger::LogEntry logEntry;

    logEntry.mTime = time;

    if (time.HasValue()) {
        Error err;

        Tie(logEntry.mContent, err) = time.GetValue().ToUTCString();
        zassert_true(err.IsNone(), "Failed to create log entry: %s", utils::ErrorToCStr(err));
    }

    logEntry.mContent.Append(content);

    return logEntry;
}

void* Setup(void)
{
    Log::SetCallback(
        [](const aos::String&, aos::LogLevel, const aos::String& message) { printk("%s\n", message.CStr()); });

    auto fixture = new logprovider_fixture;

    return fixture;
}

} // namespace

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    logprovider, nullptr, Setup,
    [](void* fixture) {
        auto logproviderFixture = static_cast<logprovider_fixture*>(fixture);

        logproviderFixture->mLogObserver = std::make_unique<logger::LogObserverStub>();
        logproviderFixture->mLogReader   = std::make_unique<logger::LogReaderStub>();
        logproviderFixture->mLogProvider = std::make_unique<LogProvider>();

        auto err = logproviderFixture->mLogProvider->Init(*logproviderFixture->mLogReader);
        zassert_true(err.IsNone(), "Failed to initialize log provider: %s", utils::ErrorToCStr(err));

        err = logproviderFixture->mLogProvider->Subscribe(*logproviderFixture->mLogObserver);
        zassert_true(err.IsNone(), "Failed to subscribe log observer: %s", utils::ErrorToCStr(err));

        err = logproviderFixture->mLogProvider->Start();
        zassert_true(err.IsNone(), "Failed to start log provider: %s", utils::ErrorToCStr(err));
    },
    [](void* fixture) {
        auto logproviderFixture = static_cast<logprovider_fixture*>(fixture);

        auto err = logproviderFixture->mLogProvider->Stop();
        zassert_true(err.IsNone(), "Can't stop SM client: %s", utils::ErrorToCStr(err));
    },
    nullptr);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(logprovider, test_start_stop)
{
    auto err = fixture->mLogProvider->Start();
    zassert_true(err.Is(ErrorEnum::eWrongState), "Unexpected error: %s", utils::ErrorToCStr(err));

    err = fixture->mLogProvider->Stop();
    zassert_true(err.IsNone(), "Failed to stop log provider: %s", utils::ErrorToCStr(err));

    err = fixture->mLogProvider->Stop();
    zassert_true(err.Is(ErrorEnum::eWrongState), "Unexpected error: %s", utils::ErrorToCStr(err));

    err = fixture->mLogProvider->Start();
    zassert_true(err.IsNone(), "Failed to start log provider: %s", utils::ErrorToCStr(err));
}

ZTEST_F(logprovider, test_get_instance_log)
{
    auto logRequest = std::make_unique<cloudprotocol::RequestLog>();

    auto err = fixture->mLogProvider->GetInstanceLog(*logRequest);
    zassert_true(err.IsNone(), "Failed to get system log: %s", utils::ErrorToCStr(err));
}

ZTEST_F(logprovider, test_get_instance_crash_log)
{
    auto logRequest = std::make_unique<cloudprotocol::RequestLog>();

    auto err = fixture->mLogProvider->GetInstanceCrashLog(*logRequest);
    zassert_true(err.IsNone(), "Failed to get system log: %s", utils::ErrorToCStr(err));
}

ZTEST_F(logprovider, test_get_empty_system_logs)
{
    const std::vector<logger::LogEntry> logEntries = {
        CreateLogEntry("log_entry_1", cTillTimeFilter),
        CreateLogEntry("log_entry_2", cTillTimeFilter),
        CreateLogEntry("log_entry_3", cTillTimeFilter),
        CreateLogEntry("log_entry_4", cTillTimeFilter.Add(Time::cHours)),
        CreateLogEntry("log_entry_5", cFromTimeFilter.Add(-Time::cHours)),
        CreateLogEntry("time not set", {}),
    };

    fixture->mLogReader->SetLogEntries(logEntries);
    fixture->mLogReader->SetIsValid(true);

    auto logRequest           = std::make_unique<cloudprotocol::RequestLog>();
    logRequest->mLogID        = "log_id";
    logRequest->mFilter.mFrom = cFromTimeFilter;
    logRequest->mFilter.mTill = cTillTimeFilter;

    auto err = fixture->mLogProvider->GetSystemLog(*logRequest);
    zassert_true(err.IsNone(), "Failed to get system log: %s", utils::ErrorToCStr(err));

    auto response = std::make_unique<cloudprotocol::PushLog>();

    err = fixture->mLogObserver->WaitLogReceived(*response);
    zassert_true(err.IsNone(), "Failed to wait log received: %s", utils::ErrorToCStr(err));

    zassert_equal(response->mLogID, logRequest->mLogID, "Log ID mismatch");
    zassert_equal(response->mPartsCount, 1, "Log parts count mismatch");
    zassert_equal(response->mStatus, cloudprotocol::LogStatusEnum::eEmpty, "Log status mismatch");
    zassert_equal(response->mContent.Size(), 0, "Log content size mismatch");
}

ZTEST_F(logprovider, test_get_filtered_system_logs)
{
    const std::vector<logger::LogEntry> logEntries = {
        CreateLogEntry("log_entry_1", cFromTimeFilter),
        CreateLogEntry("log_entry_2", cFromTimeFilter),
        CreateLogEntry("log_entry_3", cFromTimeFilter),
        CreateLogEntry("log_entry_4", cFromTimeFilter.Add(10 * Time::cMinutes)),
        CreateLogEntry("log_entry_5", cFromTimeFilter.Add(11 * Time::cMinutes)),
        CreateLogEntry("time not set", {}),
    };

    fixture->mLogReader->SetLogEntries(logEntries);
    fixture->mLogReader->SetIsValid(true);

    auto logRequest           = std::make_unique<cloudprotocol::RequestLog>();
    logRequest->mLogID        = "log_id";
    logRequest->mFilter.mFrom = cFromTimeFilter;
    logRequest->mFilter.mTill = cTillTimeFilter;

    auto err = fixture->mLogProvider->GetSystemLog(*logRequest);
    zassert_true(err.IsNone(), "Failed to get system log: %s", utils::ErrorToCStr(err));

    auto response = std::make_unique<cloudprotocol::PushLog>();

    err = fixture->mLogObserver->WaitLogReceived(*response);
    zassert_true(err.IsNone(), "Failed to wait log received: %s", utils::ErrorToCStr(err));

    zassert_equal(response->mLogID, logRequest->mLogID, "Log ID mismatch");
    zassert_equal(response->mPartsCount, 1, "Log parts count mismatch");
    zassert_equal(response->mStatus, cloudprotocol::LogStatusEnum::eOk, "Log status mismatch");

    err = fixture->mLogObserver->WaitLogReceived(*response);
    zassert_true(err.IsNone(), "Failed to wait log received: %s", utils::ErrorToCStr(err));

    zassert_equal(response->mLogID, logRequest->mLogID, "Log ID mismatch");
    zassert_equal(response->mPartsCount, 2, "Log parts count mismatch");
    zassert_equal(response->mStatus, cloudprotocol::LogStatusEnum::eEmpty, "Log status mismatch");
    zassert_equal(response->mContent.Size(), 0, "Log content size mismatch");
}

} // namespace aos::zephyr::logprovider

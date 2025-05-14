/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGPROVIDER_HPP_
#define LOGPROVIDER_HPP_

#include <aos/common/tools/allocator.hpp>
#include <aos/common/tools/thread.hpp>
#include <aos/sm/logprovider.hpp>

#include "logger/logreader.hpp"

namespace aos::zephyr::logprovider {

/**
 * Log provider.
 */
class LogProvider : public sm::logprovider::LogProviderItf {
public:
    /**
     * Initializes log provider.
     *
     * @param logReader log reader.
     * @return Error.
     */
    Error Init(logger::LogReaderItf& logReader);

    /**
     * Starts log provider.
     *
     * @return Error.
     */
    Error Start();

    /**
     * Stops log provider.
     *
     * @return Error.
     */
    Error Stop();

    /**
     * Gets instance log.
     *
     * @param request request log.
     * @return Error.
     */
    Error GetInstanceLog(const cloudprotocol::RequestLog& request) override;

    /**
     * Gets instance crash log.
     *
     * @param request request log.
     * @return Error.
     */
    Error GetInstanceCrashLog(const cloudprotocol::RequestLog& request) override;

    /**
     * Gets system log.
     *
     * @param request request log.
     * @return Error.
     */
    Error GetSystemLog(const cloudprotocol::RequestLog& request) override;

    /**
     * Subscribes on logs.
     *
     * @param observer logs observer.
     * @return Error.
     */
    Error Subscribe(sm::logprovider::LogObserverItf& observer) override;

    /**
     * Unsubscribes from logs.
     *
     * @param observer logs observer.
     * @return Error.
     */
    Error Unsubscribe(sm::logprovider::LogObserverItf& observer) override;

private:
    static constexpr auto cMaxNumLogRequests = 4;
    static constexpr auto cMaxLogFiles       = CONFIG_AOS_LOG_BACKEND_FS_FILES_LIMIT;
    static constexpr auto cAllocatorSize     = sizeof(cloudprotocol::PushLog) + sizeof(logger::LogEntry);

    Error SendLogChunk(cloudprotocol::PushLog& log);
    Error SendFinalChunk(cloudprotocol::PushLog& log);
    Error SendErrorLog(const String& logID, const Error& err);
    Error HandleLogRequest(const cloudprotocol::RequestLog& request);
    void  ProcessLogRequests();
    bool  SkipLogEntry(const logger::LogEntry& logEntry, const cloudprotocol::RequestLog& request);

    StaticAllocator<cAllocatorSize>                            mAllocator;
    StaticArray<cloudprotocol::RequestLog, cMaxNumLogRequests> mLogRequests;
    Mutex                                                      mMutex;
    ConditionalVariable                                        mCondVar;
    Thread<>                                                   mThread;
    bool                                                       mStopped     = true;
    sm::logprovider::LogObserverItf*                           mLogObserver = nullptr;
    logger::LogReaderItf*                                      mLogReader   = nullptr;
};

} // namespace aos::zephyr::logprovider

#endif // LOGPROVIDER_HPP_

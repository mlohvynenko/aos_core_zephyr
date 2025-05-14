/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGREADER_HPP_
#define LOGREADER_HPP_

#include <aos/common/tools/noncopyable.hpp>
#include <aos/common/tools/optional.hpp>

#include "logger/types.hpp"

namespace aos::zephyr::logger {

/**
 * Log entry structure.
 */
struct LogEntry {
    StaticString<cLogEntryLen> mContent;
    Optional<Time>             mTime = {};

    /**
     * Resets underlying data.
     */
    void Reset()
    {
        mContent.Clear();
        mTime.Reset();
    }
};

/**
 * Log reader interface.
 */
class LogReaderItf {
public:
    /**
     * Destructor.
     */
    virtual ~LogReaderItf() = default;

    /**
     * Reads log entry.
     *
     * @param entry log entry.
     * @return Error.
     */
    virtual Error GetEntry(LogEntry& entry) = 0;

    /**
     * Checks if reader is valid.
     *
     * @return bool.
     */
    virtual bool IsValid() = 0;

    /**
     * Resets reader.
     *
     * @return Error.
     */
    virtual Error Reset() = 0;
};

} // namespace aos::zephyr::logger

#endif

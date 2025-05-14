/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FSLOGREADER_HPP_
#define FSLOGREADER_HPP_

#include <aos/common/tools/noncopyable.hpp>
#include <aos/common/tools/optional.hpp>
#include <aos/common/tools/thread.hpp>

#include "logger/fsbackend.hpp"
#include "logger/logreader.hpp"

namespace aos::zephyr::logger {

/**
 * File system log reader.
 */
class FSLogReader : public LogReaderItf, public NonCopyable {
public:
    /**
     * Reads log entry.
     *
     * @param entry log entry.
     * @return Error.
     */
    Error GetEntry(LogEntry& entry) override;

    /**
     * Checks if reader is valid.
     *
     * @return bool.
     */
    bool IsValid() override;

    /**
     * Resets reader.
     *
     * @return Error.
     */
    Error Reset() override;

private:
    Error OpenNextFile();
    void  CloseFile();
    Error ReadLine(LogEntry& entry);
    Error ReadLogFiles();

    size_t                                                mCurrentPos = 0;
    int                                                   mFD         = -1;
    StaticArray<StaticString<cFilePathLen>, cMaxLogFiles> mLogFiles;
    Mutex                                                 mMutex;
};

} // namespace aos::zephyr::logger

#endif

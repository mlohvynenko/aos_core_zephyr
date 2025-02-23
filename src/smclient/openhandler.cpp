/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pb_decode.h>

#include "log.hpp"
#include "openhandler.hpp"

#include "communication/pbhandler.cpp"

namespace aos::zephyr::smclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error OpenHandler::Init(communication::ChannelItf& channel, clocksync::ClockSyncItf& clockSync)
{
    mClockSync = &clockSync;

    if (auto err = PBHandler::Init("SM open", channel); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error OpenHandler::SendClockSyncRequest()
{
    mOutgoingMessages.SMOutgoingMessage.clock_sync_request
        = servicemanager_v4_ClockSyncRequest servicemanager_v4_ClockSyncRequest_init_default;
    mOutgoingMessages.which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_clock_sync_request_tag;

    LOG_INF() << "Send SM clock sync request";

    return SendMessage(&mOutgoingMessages, &servicemanager_v4_SMOutgoingMessages_msg);
}

OpenHandler::~OpenHandler()
{
    Stop();
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void OpenHandler::OnConnect()
{
    auto err = mClockSync->Start();
    if (!err.IsNone()) {
        LOG_ERR() << "Failed to start clock sync: err=" << err;
    }
}

void OpenHandler::OnDisconnect()
{
}

Error OpenHandler::ReceiveMessage(const Array<uint8_t>& data)
{
    auto stream = pb_istream_from_buffer(data.Get(), data.Size());

    if (auto status = pb_decode(&stream, &servicemanager_v4_SMIncomingMessages_msg, &mIncomingMessages); !status) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eRuntime, "failed to decode message"));
    }

    Error err;

    switch (mIncomingMessages.which_SMIncomingMessage) {
    case servicemanager_v4_SMIncomingMessages_clock_sync_tag:
        err = ProcessClockSync(mIncomingMessages.SMIncomingMessage.clock_sync);
        break;

    default:
        LOG_WRN() << "Receive unsupported message: tag=" << mIncomingMessages.which_SMIncomingMessage;
        break;
    }

    return err;
}

Error OpenHandler::ProcessClockSync(const servicemanager_v4_ClockSync& pbClockSync)
{
    LOG_INF() << "Process clock sync";

    if (!pbClockSync.has_current_time) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "ClockSync message has no current time"));
    }

    auto err = mClockSync->Sync(aos::Time::Unix(pbClockSync.current_time.seconds, pbClockSync.current_time.nanos));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

} // namespace aos::zephyr::smclient

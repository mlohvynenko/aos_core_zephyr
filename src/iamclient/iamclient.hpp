/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IAMCLIENT_HPP_
#define IAMCLIENT_HPP_

#include <aos/common/tools/thread.hpp>
#include <aos/iam/nodeinfoprovider.hpp>
#include <aos/iam/provisionmanager.hpp>

#include <proto/iamanager/v5/iamanager.pb.h>

#include "clocksync/clocksync.hpp"
#include "communication/channelmanager.hpp"
#include "communication/pbhandler.hpp"

namespace aos::zephyr::iamclient {

/**
 * IAM client instance.
 */
class IAMClient
    : public communication::PBHandler<iamanager_v5_IAMIncomingMessages_size, iamanager_v5_IAMOutgoingMessages_size>,
      public clocksync::ClockSyncSubscriberItf,
      iam::nodeinfoprovider::NodeStatusObserverItf,
      private NonCopyable {
public:
    /**
     * Initializes IAM client instance.
     *
     * @param clockSync clock sync instance.
     * @param nodeInfoProvider node info provider.
     * @param provisionManager provision manager.
     * @param channelManager channel manager instance.
     * @return Error.
     */
    Error Init(clocksync::ClockSyncItf& clockSync, iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider,
        iam::provisionmanager::ProvisionManagerItf& provisionManager, communication::ChannelManagerItf& channelManager);

    /**
     * Notifies subscriber clock is synced.
     */
    void OnClockSynced() override;

    /**
     * Notifies subscriber clock is unsynced.
     */
    void OnClockUnsynced() override;

    /**
     * On node status changed event.
     *
     * @param nodeID node id
     * @param status node status
     * @return Error
     */
    Error OnNodeStatusChanged(const String& nodeID, const NodeStatus& status) override;

    /**
     * Destructor.
     */
    ~IAMClient();

private:
    static constexpr auto cOpenPort   = CONFIG_AOS_IAM_OPEN_PORT;
    static constexpr auto cSecurePort = CONFIG_AOS_IAM_SECURE_PORT;

    void  OnConnect() override;
    void  OnDisconnect() override;
    Error ReceiveMessage(const Array<uint8_t>& data) override;

    template <typename T>
    Error SendError(const void* message, T& pbMessage, const Error& err);
    Error SendNodeInfo();
    Error CheckNodeIDAndStatus(const String& nodeID, const Array<NodeStatus>& expectedStatuses);
    Error ProcessStartProvisioning(const iamanager_v5_StartProvisioningRequest& pbStartProvisioningRequest);
    Error ProcessFinishProvisioning(const iamanager_v5_FinishProvisioningRequest& pbFinishProvisioningRequest);
    Error ProcessDeprovision(const iamanager_v5_DeprovisionRequest& pbDeprovisionRequest);
    Error ProcessPauseNode(const iamanager_v5_PauseNodeRequest& pbPauseNodeRequest);
    Error ProcessResumeNode(const iamanager_v5_ResumeNodeRequest& pbResumeNodeRequest);
    Error ProcessGetCertTypes(const iamanager_v5_GetCertTypesRequest& pbGetCertTypesRequest);
    Error ProcessCreateKey(const iamanager_v5_CreateKeyRequest& pbCreateKeyRequest);
    Error ProcessApplyCert(const iamanager_v5_ApplyCertRequest& pbApplyCertRequest);

    clocksync::ClockSyncItf*                    mClockSync {};
    iam::nodeinfoprovider::NodeInfoProviderItf* mNodeInfoProvider {};
    iam::provisionmanager::ProvisionManagerItf* mProvisionManager {};
    communication::ChannelManagerItf*           mChannelManager {};

    NodeInfo mNodeInfo;
    Mutex    mMutex;

    StaticAllocator<sizeof(iamanager_v5_IAMIncomingMessages) + sizeof(iamanager_v5_IAMOutgoingMessages) * 2> mAllocator;
};

} // namespace aos::zephyr::iamclient

#endif

/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements Thread's Network Diagnostic processing.
 */

#define WPP_NAME "network_diagnostic.tmh"

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/encoding.hpp>
#include <mac/mac_frame.hpp>
#include <net/netif.hpp>
#include <platform/random.h>
#include <thread/mesh_forwarder.hpp>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>
#include <thread/network_diagnostic.hpp>
#include <thread/network_diagnostic_tlvs.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {

namespace NetworkDiagnostic {

NetworkDiagnostic::NetworkDiagnostic(ThreadNetif &aThreadNetif) :
    mDiagnosticGet(OPENTHREAD_URI_DIAGNOSTIC_GET, &NetworkDiagnostic::HandleDiagnosticGet, this),
    mDiagnosticReset(OPENTHREAD_URI_DIAGNOSTIC_RESET, &NetworkDiagnostic::HandleDiagnosticReset, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mCoapClient(aThreadNetif.GetCoapClient()),
    mMle(aThreadNetif.GetMle()),
    mMac(aThreadNetif.GetMac()),
    mNetif(aThreadNetif),
    mReceiveDiagnosticGetCallback(NULL),
    mReceiveDiagnosticGetCallbackContext(NULL)
{
    mCoapServer.AddResource(mDiagnosticGet);
    mCoapServer.AddResource(mDiagnosticReset);
}

void NetworkDiagnostic::SetReceiveDiagnosticGetCallback(otReceiveDiagnosticGetCallback aCallback,
                                                        void *aCallbackContext)
{
    mReceiveDiagnosticGetCallback = aCallback;
    mReceiveDiagnosticGetCallbackContext = aCallbackContext;
}

ThreadError NetworkDiagnostic::SendDiagnosticGet(const Ip6::Address &aDestination, const uint8_t aTlvTypes[],
                                                 uint8_t aCount)
{
    ThreadError error;
    Ip6::SockAddr sockaddr;
    Message *message;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;

    sockaddr.mPort = kCoapUdpPort;

    header.Init(kCoapTypeConfirmable, kCoapRequestGet);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_DIAGNOSTIC_GET);
    header.SetPayloadMarker();

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = message->Append(aTlvTypes, aCount));

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(mMle.GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo,
                                                  &NetworkDiagnostic::HandleDiagnosticGetResponse, this));

    otLogInfoNetDiag("Sent diagnostic get");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkDiagnostic::HandleDiagnosticGetResponse(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                                    const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGetResponse(static_cast<Coap::Header *>(aHeader),
                                                                            static_cast<Message *>(aMessage),
                                                                            static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                                            aResult);
}

void NetworkDiagnostic::HandleDiagnosticGetResponse(Coap::Header *aHeader, Message *aMessage,
                                                    const Ip6::MessageInfo *aMessageInfo, ThreadError aResult)
{
    VerifyOrExit(aResult == kThreadError_None, ;);
    VerifyOrExit(aHeader->GetCode() == kCoapResponseChanged, ;);

    otLogInfoNetDiag("Network Diagnostic get response received");

    VerifyOrExit(mReceiveDiagnosticGetCallback != NULL, ;);

    // Call application defined callback.
    mReceiveDiagnosticGetCallback(aMessage, aMessageInfo, mReceiveDiagnosticGetCallbackContext);

exit:
    return;
}

ThreadError NetworkDiagnostic::SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[],
                                                   uint8_t aCount)
{
    ThreadError error;
    Ip6::SockAddr sockaddr;
    Message *message;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_DIAGNOSTIC_RESET);
    header.SetPayloadMarker();

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = message->Append(aTlvTypes, aCount));

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(mMle.GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo));

    otLogInfoNetDiag("Sent network diagnostic reset");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void NetworkDiagnostic::HandleDiagnosticGet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                            const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

ThreadError NetworkDiagnostic::AppendIPv6AddressList(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    IPv6AddressListTlv tlv;
    uint8_t count = 0;

    tlv.Init();

    for (const Ip6::NetifUnicastAddress *addr = mNetif.GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        count++;
    }

    tlv.SetLength(count * sizeof(Ip6::Address));
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    for (const Ip6::NetifUnicastAddress *addr = mNetif.GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        SuccessOrExit(error = aMessage.Append(&addr->GetAddress(), sizeof(Ip6::Address)));
    }

exit:

    return error;
}

ThreadError NetworkDiagnostic::AppendChildTable(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    uint8_t count = 0;
    uint8_t timeout = 0;
    uint8_t numChildren;
    const Child *children = mMle.GetChildren(&numChildren);
    ChildTableTlv tlv;
    ChildTableEntry entry;

    tlv.Init();

    for (int i = 0; i < numChildren; i++)
    {
        if (children[i].mState == Neighbor::kStateValid)
        {
            count++;
        }
    }

    tlv.SetLength(count * sizeof(ChildTableEntry));

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(ChildTableTlv)));

    for (int i = 0; i < numChildren; i++)
    {
        if (children[i].mState == Neighbor::kStateValid)
        {
            timeout = 0;

            while (static_cast<uint32_t>(1 << timeout) < children[i].mTimeout) { timeout++; }

            entry.SetReserved(0);
            entry.SetTimeout(timeout + 4);
            entry.SetChildId(mMle.GetChildId(children[i].mValid.mRloc16));
            entry.SetMode(children[i].mMode);

            SuccessOrExit(error = aMessage.Append(&entry, sizeof(ChildTableEntry)));
        }
    }

exit:

    return error;
}

void NetworkDiagnostic::HandleDiagnosticGet(Coap::Header &aHeader, Message &aMessage,
                                            const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    uint16_t offset = 0;
    uint8_t type;
    NetworkDiagnosticTlv networkDiagnosticTlv;
    Coap::Header header;
    Ip6::MessageInfo messageInfo(aMessageInfo);

    VerifyOrExit(aHeader.GetType() == kCoapTypeConfirmable &&
                 aHeader.GetCode() == kCoapRequestGet, error = kThreadError_Drop);

    otLogInfoNetDiag("Received diagnostic get request");

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv),
                                &networkDiagnosticTlv) == sizeof(NetworkDiagnosticTlv)), error = kThreadError_Drop);

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList, error = kThreadError_Drop);

    VerifyOrExit((static_cast<TypeListTlv *>(&networkDiagnosticTlv)->IsValid()), error = kThreadError_Drop);

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    header.SetDefaultResponseHeader(aHeader);

    if (networkDiagnosticTlv.GetLength() > 0)
    {
        header.SetPayloadMarker();
    }

    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    offset = aMessage.GetOffset() + sizeof(NetworkDiagnosticTlv);

    for (uint8_t i = 0; i < networkDiagnosticTlv.GetLength(); i++)
    {

        VerifyOrExit(aMessage.Read(offset, sizeof(type), &type) == sizeof(type), error = kThreadError_Drop);

        otLogInfoNetDiag("Received diagnostic get type %d", type);

        switch (type)
        {
        case NetworkDiagnosticTlv::kExtMacAddress:
        {
            ExtMacAddressTlv tlv;
            tlv.Init();
            tlv.SetMacAddr(*mNetif.GetMac().GetExtAddress());
            SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kAddress16:
        {
            Address16Tlv tlv;
            tlv.Init();
            tlv.SetRloc16(mMle.GetRloc16());
            SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kMode:
        {
            ModeTlv tlv;
            tlv.Init();
            tlv.SetMode(mMle.GetDeviceMode());
            SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kTimeout:
        {
            if ((mMle.GetDeviceMode() & ModeTlv::kModeRxOnWhenIdle) == 0)
            {
                TimeoutTlv tlv;
                tlv.Init();
                tlv.SetTimeout(mMle.GetTimeout());
                SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            }

            break;
        }

        case NetworkDiagnosticTlv::kConnectivity:
        {
            ConnectivityTlv tlv;
            tlv.Init();
            mMle.FillConnectivityTlv(*reinterpret_cast<Mle::ConnectivityTlv *>(&tlv));
            SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
            break;
        }

        case NetworkDiagnosticTlv::kRoute:
        {
            RouteTlv tlv;
            tlv.Init();
            mMle.FillRouteTlv(*reinterpret_cast<Mle::RouteTlv *>(&tlv));
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kLeaderData:
        {
            LeaderDataTlv tlv;
            memcpy(&tlv, &mMle.GetLeaderDataTlv(), sizeof(tlv));
            tlv.Init();
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kNetworkData:
        {
            NetworkDataTlv tlv;
            tlv.Init();
            mMle.FillNetworkDataTlv((*reinterpret_cast<Mle::NetworkDataTlv *>(&tlv)), true);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kIPv6AddressList:
        {
            SuccessOrExit(error = AppendIPv6AddressList(*message));
            break;
        }

        case NetworkDiagnosticTlv::kMacCounters:
        {
            MacCountersTlv tlv;
            memset(&tlv, 0, sizeof(tlv));
            tlv.Init();
            mMac.FillMacCountersTlv(tlv);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kBatteryLevel:
        {
            // TODO Need more api from driver
            BatteryLevelTlv tlv;
            tlv.Init();
            tlv.SetBatteryLevel(100);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kSupplyVoltage:
        {
            // TODO Need more api from driver
            SupplyVoltageTlv tlv;
            tlv.Init();
            tlv.SetSupplyVoltage(0);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        case NetworkDiagnosticTlv::kChildTable:
        {
            SuccessOrExit(error = AppendChildTable(*message));
            break;
        }

        case NetworkDiagnosticTlv::kChannelPages:
        {
            ChannelPagesTlv tlv;
            tlv.Init();
            tlv.GetChannelPages()[0] = 0;
            tlv.SetLength(1);
            SuccessOrExit(error = message->Append(&tlv, tlv.GetSize()));
            break;
        }

        default:
            ExitNow();
        }

        offset += sizeof(type);
    }

    SuccessOrExit(error = mCoapServer.SendMessage(*message, messageInfo));

    otLogInfoNetDiag("Sent diagnostic get acknowledgment");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

void NetworkDiagnostic::HandleDiagnosticReset(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                              const otMessageInfo *aMessageInfo)
{
    static_cast<NetworkDiagnostic *>(aContext)->HandleDiagnosticReset(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkDiagnostic::HandleDiagnosticReset(Coap::Header &aHeader, Message &aMessage,
                                              const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    uint16_t offset = 0;
    uint8_t type;
    NetworkDiagnosticTlv networkDiagnosticTlv;
    Coap::Header header;
    Ip6::MessageInfo messageInfo(aMessageInfo);

    otLogInfoNetDiag("Received diagnostic reset request");

    VerifyOrExit(aHeader.GetType() == kCoapTypeConfirmable &&
                 aHeader.GetCode() == kCoapRequestPost, error = kThreadError_Drop);

    VerifyOrExit((aMessage.Read(aMessage.GetOffset(), sizeof(NetworkDiagnosticTlv),
                                &networkDiagnosticTlv) == sizeof(NetworkDiagnosticTlv)), error = kThreadError_Drop);

    VerifyOrExit(networkDiagnosticTlv.GetType() == NetworkDiagnosticTlv::kTypeList, error = kThreadError_Drop);

    VerifyOrExit((static_cast<TypeListTlv *>(&networkDiagnosticTlv)->IsValid()), error = kThreadError_Drop);

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    header.SetDefaultResponseHeader(aHeader);

    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    offset = aMessage.GetOffset() + sizeof(NetworkDiagnosticTlv);

    for (uint8_t i = 0; i < networkDiagnosticTlv.GetLength(); i++)
    {
        VerifyOrExit(aMessage.Read(offset, sizeof(type), &type) == sizeof(type), error = kThreadError_Drop);

        switch (type)
        {
        case NetworkDiagnosticTlv::kMacCounters:
            mMac.ResetCounters();
            otLogInfoNetDiag("Received diagnostic reset type kMacCounters(9)");
            break;

        default:
            otLogInfoNetDiag("Received diagnostic reset other type %d not resetable", type);
            break;
        }
    }

    SuccessOrExit(error = mCoapServer.SendMessage(*message, messageInfo));

    otLogInfoNetDiag("Sent diagnostic reset acknowledgment");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

}  // namespace NetworkDiagnostic

}  // namespace Thread

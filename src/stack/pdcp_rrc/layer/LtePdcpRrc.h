//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEPDCPRRC_H_
#define _LTE_LTEPDCPRRC_H_

#include <omnetpp.h>

#include "corenetwork/binder/LteBinder.h"
#include "common/LteCommon.h"
#include "stack/pdcp_rrc/ConnectionsTable.h"
#include "corenetwork/lteip/LteIp.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"
#include "stack/pdcp_rrc/layer/entity/LtePdcpEntity.h"


#define LTE_PDCP_HEADER_COMPRESSION_DISABLED B(-1)

/**
 * @class LtePdcp
 * @brief PDCP Layer
 *
 * TODO REVIEW COMMENTS
 *
 * This is the PDCP/RRC layer of LTE Stack.
 *
 * PDCP part performs the following tasks:
 * - Header compression/decompression
 * - association of the terminal with its eNodeB, thus storing its MacNodeId.
 *
 * The PDCP layer attaches an header to the packet. The size
 * of this header is fixed to 2 bytes.
 *
 * RRC part performs the following actions:
 * - Binding the Local IP Address of this Terminal with
 *   its module id (MacNodeId) by telling these informations
 *   to the oracle
 * - Assign a Logical Connection IDentifier (LCID)
 *   for each connection request (coming from PDCP)
 *
 * The couple < MacNodeId, LogicalCID > constitute the CID,
 * that uniquely identifies a connection in the whole network.
 *
 */
class SIMULTE_API LtePdcpRrcBase : public omnetpp::cSimpleModule
{
  public:
    /**
     * Initializes the connection table
     */
    LtePdcpRrcBase();

    /**
     * Cleans the connection table
     */
    virtual ~LtePdcpRrcBase();

  protected:

    /**
     * Initialize class structures
     * gates, delay, compression
     * and watches
     */
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * Statistics recording
     */
    virtual void finish() override;

    /*
     * Internal functions
     */

    /**
     * headerCompress(): Performs header compression.
     * At the moment, if header compression is enabled,
     * simply decrements the HEADER size by the configured
     * number of bytes
     *
     * @param Packet packet to compress
     */
    void headerCompress(inet::Packet* pkt);

    /**
     * headerDecompress(): Performs header decompression.
     * At the moment, if header compression is enabled,
     * simply restores original packet size
     *
     * @param Packet packet to decompress
     */
    void headerDecompress(inet::Packet* pkt);

    /*
     * Functions to be implemented from derived classes
     */

    /**
     * handleControlInfo() determines whether the controlInfo must
     * be detached from packet (ENODEB or RELAY) or left unchanged (RELAY)
     *
     * @param pkt packet
     * @param lteInfo Control Info
     */
    virtual void handleControlInfo(omnetpp::cPacket* pkt, FlowControlInfo* lteInfo) = 0;

    /**
     * getDestId() retrieves the id of destination node according
     * to the following rules:
     * - On UE use masterId
     * - On ENODEB:
     *   - Use source Ip for directly attached UEs
     *   - Use relay Id for UEs attahce to relays
     * - On RELAY:
     *   - Use masterId for packets destined to ENODEB
     *   - Use source Ip for packets destined to UEs
     *
     * @param lteInfo Control Info
     */
    virtual MacNodeId getDestId(FlowControlInfo* lteInfo) = 0;

    /**
     * getDirection() is used only on UEs and ENODEBs:
     * - direction is downlink for ENODEB
     * - direction is uplink for UE
     *
     * @return Direction of traffic
     */
    virtual Direction getDirection() = 0;
    void setTrafficInformation(omnetpp::cPacket* pkt, FlowControlInfo* lteInfo);

    bool isCompressionEnabled();

    /*
     * Upper Layer Handlers
     */

    /**
     * handler for data port
     *
     * fromDataPort() receives data packets from applications
     * and performs the following steps:
     * - If compression is enabled, header is compressed
     * - Reads the source port to determine if a
     *   connection for that application was already established
     *    - If it was established, sends the packet with the proper CID
     *    - Otherwise, encapsulates packet in a sap message and sends it
     *      to the RRC layer: it will find a proper CID and send the
     *      packet back to the PDCP layer
     *
     * @param pkt incoming packet
     */
    virtual void fromDataPort(omnetpp::cPacket *pkt);

    /**
     * handler for eutran port
     *
     * fromEutranRrcSap() receives data packets from eutran
     * and sends it on a special LCID, over TM
     *
     * @param pkt incoming packet
     */
    void fromEutranRrcSap(omnetpp::cPacket *pkt);

    /*
     * Lower Layer Handlers
     */

    /**
     * handler for um/am sap
     *
     * toDataPort() performs the following steps:
     * - decompresses the header, restoring original packet
     * - decapsulates the packet
     * - sends the packet to the application layer
     *
     * @param pkt incoming packet
     */
    void toDataPort(omnetpp::cPacket *pkt);

    /**
     * handler for tm sap
     *
     * toEutranRrcSap() decapsulates packet and sends it
     * over the eutran port
     *
     * @param pkt incoming packet
     */
    void toEutranRrcSap(omnetpp::cPacket *pkt);

    /*
     * Data structures
     */

    /// Header size after ROHC (RObust Header Compression)
    inet::B headerCompressedSize_;

    /// Binder reference
    LteBinder *binder_;

    /// Connection Identifier
    LogicalCid lcid_;

    /// Hash Table used for CID <-> Connection mapping
    ConnectionsTable* ht_;

    /// Identifier for this node
    MacNodeId nodeId_;

    omnetpp::cGate* dataPort_[2];
    omnetpp::cGate* eutranRrcSap_[2];
    omnetpp::cGate* tmSap_[2];
    omnetpp::cGate* umSap_[2];
    omnetpp::cGate* amSap_[2];

    /**
     * The entities map associate each LCID with a PDCP Entity, identified by its ID
     */
    typedef std::map<LogicalCid, LtePdcpEntity*> PdcpEntities;
    PdcpEntities entities_;

    /**
     * getEntity() is used to gather the PDCP entity
     * for that LCID. If entity was already present, a reference
     * is returned, otherwise a new entity is created,
     * added to the entities map and a reference is returned as well.
     *
     * @param lcid Logical CID
     * @return pointer to the PDCP entity for the LCID of the flow
     *
     */
    LtePdcpEntity* getEntity(LogicalCid lcid);

    // statistics
    omnetpp::simsignal_t receivedPacketFromUpperLayer;
    omnetpp::simsignal_t receivedPacketFromLowerLayer;
    omnetpp::simsignal_t sentPacketToUpperLayer;
    omnetpp::simsignal_t sentPacketToLowerLayer;
};

class SIMULTE_API LtePdcpRrcUe : public LtePdcpRrcBase
{
  protected:
    void handleControlInfo(omnetpp::cPacket* upPkt, FlowControlInfo* lteInfo)
    {
        delete lteInfo;
    }

    MacNodeId getDestId(FlowControlInfo* lteInfo)
    {
        // UE is subject to handovers: master may change
        return binder_->getNextHop(nodeId_);
    }

    Direction getDirection()
    {
        // Data coming from Dataport on UE are always Uplink
        return UL;
    }

  public:
    virtual void initialize(int stage);
};

class SIMULTE_API LtePdcpRrcEnb : public LtePdcpRrcBase
{
  protected:
    void handleControlInfo(omnetpp::cPacket* upPkt, FlowControlInfo* lteInfo)
    {
        delete lteInfo;
    }

    MacNodeId getDestId(FlowControlInfo* lteInfo)
    {
        // dest id
        MacNodeId destId = binder_->getMacNodeId(inet::Ipv4Address(lteInfo->getDstAddr()));
        // master of this ue (myself or a relay)
        MacNodeId master = binder_->getNextHop(destId);
        std::cout << "destid: " << destId << " master: " << master << "\n";
        if (master != nodeId_)
        { // ue is relayed, dest must be the relay
            destId = master;
        } // else ue is directly attached
        return destId;
    }

    Direction getDirection()
    {
        // Data coming from Dataport on ENB are always Downlink
        return DL;
    }
  public:
    virtual void initialize(int stage);
};

class SIMULTE_API LtePdcpRrcRelayEnb : public LtePdcpRrcBase
{
  protected:
    void handleControlInfo(omnetpp::cPacket* upPkt, FlowControlInfo* lteInfo)
    {
        upPkt->setControlInfo(lteInfo);
    }

    MacNodeId getDestId(FlowControlInfo* lteInfo)
    {
        // packet arriving from eNB, send to UE given the IP address
        return getBinder()->getMacNodeId(inet::Ipv4Address(lteInfo->getDstAddr()));
    }

    // Relay doesn't set Traffic Information
    void setTrafficInformation(FlowControlInfo* lteInfo)
    {
    }

    Direction getDirection()
    {
        // Error: Relay doesn't set direction!
        return UNKNOWN_DIRECTION;
    }
};

class SIMULTE_API LtePdcpRrcRelayUe : public LtePdcpRrcBase
{
  protected:
    /// Node id
    MacNodeId destId_;

    virtual void initialize(int stage)
    {
        LtePdcpRrcBase::initialize(stage);
        destId_ = getAncestorPar("masterId");
        WATCH(destId_);
    }

    void handleControlInfo(omnetpp::cPacket* upPkt, FlowControlInfo* lteInfo)
    {
        upPkt->setControlInfo(lteInfo);
    }

    MacNodeId getDestId(FlowControlInfo* lteInfo)
    {
        // packet arriving from UE, send to master
        return destId_;
    }

    // Relay doesn't set Traffic Information
    void setTrafficInformation(FlowControlInfo* lteInfo)
    {
    }

    Direction getDirection()
    {
        // Error: Relay doesn't set direction!
        return UNKNOWN_DIRECTION;
    }
};

#endif

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013,2014 TELEMATICS LAB, DEI - Politecnico di Bari
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Giuseppe Piro <peppe@giuseppepiro.com>, <g.piro@poliba.it>
 */

#ifndef SIMPLE_NANO_DEVICE_H
#define SIMPLE_NANO_DEVICE_H

#include <string.h>
#include <ns3/node.h>
#include <ns3/address.h>
#include <ns3/net-device.h>
#include <ns3/callback.h>
#include <ns3/packet.h>
#include <ns3/traced-callback.h>
#include <ns3/ptr.h>
#include <list>

namespace ns3 {

class NanoSpectrumPhy;
class NanoMacEntity;
class NanoRoutingEntity;
class MessageProcessUnit;

/**
 * \ingroup nanonetworks
 *
 * This class provides a template implementation
 * of a a generic nano device
 */
class SimpleNanoDevice : public NetDevice
{
public:
  static TypeId GetTypeId (void);

  SimpleNanoDevice ();
  virtual ~SimpleNanoDevice ();

  virtual void DoDispose (void);

  // inherited from NetDevice
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address& dest,
                     uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest,
                         uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  virtual Address GetMulticast (Ipv4Address addr) const;
  virtual Address GetMulticast (Ipv6Address addr) const;
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;


  /**
   * Set the Phy object which is attached to this device.
   * @param phy the Phy object embedded within this device.
   */
  void SetPhy (Ptr<NanoSpectrumPhy> phy);

  /**
   * @return a reference to the PHY object embedded in this NetDevice.
   */
  Ptr<NanoSpectrumPhy> GetPhy () const;

  /**
   * Set the
   * @param mac
   */
  void SetMac (Ptr<NanoMacEntity> mac);

  /**
   * @return a reference to
   */
  Ptr<NanoMacEntity> GetMac () const;

  /**
   * Set the
   * @param routing
   */
  void SetL3 (Ptr<NanoRoutingEntity> l3);

  /**
   * @return a reference to
   */
  Ptr<NanoRoutingEntity> GetL3 () const;


  void SendPacket (Ptr<Packet> p);
  void ReceivePacket (Ptr<Packet> p);
  void SendPacketDst(Ptr<Packet> p, uint32_t dstId);//to send the message to a certain destination
//the q-routing will
  void SetMessageProcessUnit (Ptr<MessageProcessUnit> mpu);
  Ptr<MessageProcessUnit> GetMessageProcessUnit (void);

  enum NodeType
  {
    NanoNode, NanoRouter, NanoInterface
  };
  NodeType m_type;

private:

  Ptr<Node> m_node;
  uint32_t m_ifIndex;
  Ptr<NanoSpectrumPhy> m_phy;
  Ptr<NanoMacEntity> m_mac;
  Ptr<NanoRoutingEntity> m_l3;
  Ptr<MessageProcessUnit> m_mpu;

public:
  int m_seed;
  int m_nodes;
  int m_apps;
  double m_txrange;
  int m_gates;
  int m_routers;
};


} // namespace ns3

#endif /* SIMPLE_NANO_DEVICE_H */

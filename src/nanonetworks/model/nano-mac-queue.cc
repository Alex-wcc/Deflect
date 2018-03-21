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

#include "nano-mac-queue.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("NanoMacQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (NanoMacQueue);

NanoMacQueue::QueueElement::QueueElement (void)
  : m_packet (Create<Packet> ()),
    m_timeStamp (Seconds (0))
{
}


NanoMacQueue::QueueElement::QueueElement (Ptr<Packet> packet, Time timeStamp)
  : m_packet (packet),
    m_timeStamp (timeStamp)
{
}


uint32_t
NanoMacQueue::QueueElement::GetSize (void) const
{
  NS_LOG_FUNCTION (this);
  // XXX: ADD mac/RLC/CRC OVERHEADs ?!?
  uint32_t size = m_packet->GetSize ();
  return size;
}


TypeId
NanoMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NanoMacQueue")
    .SetParent<Object> ()
    .AddAttribute (
      "MaxSize",
      "Maximum size",
      UintegerValue (1024),
      MakeUintegerAccessor (&NanoMacQueue::GetMaxSize,
                            &NanoMacQueue::SetMaxSize),
      MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Enqueue",
                     "Enqueue trace",
                     MakeTraceSourceAccessor (&NanoMacQueue::m_traceEnqueue),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("Dequeue",
                     "Dequeue trace",
                     MakeTraceSourceAccessor (&NanoMacQueue::m_traceDequeue),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("Drop",
                     "Drop trace",
                     MakeTraceSourceAccessor (&NanoMacQueue::m_traceDrop),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


NanoMacQueue::NanoMacQueue (void)
  : m_maxSize (0),
    m_bytes (0),
    m_nrDataPackets (0)
{
}


NanoMacQueue::NanoMacQueue (uint32_t maxSize)
  : m_maxSize (maxSize),
    m_bytes (0),
    m_nrDataPackets (0)
{
}


NanoMacQueue::~NanoMacQueue (void)
{
  m_queue.clear ();
}


void
NanoMacQueue::SetMaxSize (uint32_t maxSize)
{
  NS_LOG_FUNCTION (this);
  m_maxSize = maxSize;
}


uint32_t
NanoMacQueue::GetMaxSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_maxSize;
}


bool
NanoMacQueue::Enqueue (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << "queue size: " << m_queue.size ());
  NS_LOG_FUNCTION (this << "packet size: " << packet->GetSize ());
  if (m_queue.size () == m_maxSize)
    {
      m_traceDrop (packet);
      return false;
    }

  m_traceEnqueue (packet);
  QueueElement element (packet, Simulator::Now ());
  m_queue.push_back (element);

  m_nrDataPackets++;
  m_bytes += element.GetSize ();

  NS_LOG_FUNCTION (this << "queue size: " << m_queue.size ());
  return true;
}


Ptr<Packet>
NanoMacQueue::Dequeue (void)
{
  NS_LOG_FUNCTION (this);
  if (!IsEmpty ())
    {
      QueueElement element = Front ();
      Pop ();

      m_nrDataPackets--;

      Ptr<Packet> packet = element.m_packet;

      return packet;
    }

  return 0;
}


Ptr<Packet>
NanoMacQueue::Dequeue (uint32_t availableByte)
{
  NS_LOG_FUNCTION (this << availableByte);
  /*
   This functiopn can be called when the UM of AM RLC mode are abilited.
   */
  return 0;
}


Ptr<Packet>
NanoMacQueue::Peek (void) const
{
  NS_LOG_FUNCTION (this);
  if (!IsEmpty ())
    {
      QueueElement element = m_queue.front ();
      Ptr<Packet> packet = element.m_packet->Copy ();

      return packet;
    }

  return 0;
}


uint32_t
NanoMacQueue::GetSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_queue.size ();
}

uint32_t
NanoMacQueue::GetNBytes (void) const
{
  NS_LOG_FUNCTION (this);
  return m_bytes;
}

uint32_t
NanoMacQueue::GetQueueLengthWithMACOverhead (void)
{
  NS_LOG_FUNCTION (this);
  uint32_t queueSize = GetNBytes ();
  // Add MAC/RLC/CRC Overhead
  queueSize += GetSize () * 0; // XXX

  return queueSize;
}

NanoMacQueue::QueueElement
NanoMacQueue::Front (void) const
{
  NS_LOG_FUNCTION (this);
  QueueElement element = m_queue.front ();
  return element;
}

void
NanoMacQueue::Pop (void)
{
  NS_LOG_FUNCTION (this);
  m_queue.pop_front ();
}

bool
NanoMacQueue::IsEmpty (void) const
{
  NS_LOG_FUNCTION (this);
  return m_queue.empty ();
}


const NanoMacQueue::PacketQueue &
NanoMacQueue::GetPacketQueue (void) const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}


} // namespace ns3

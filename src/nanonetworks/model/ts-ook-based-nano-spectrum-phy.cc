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
//#define NS_LOG_APPEND_CONTEXT std::clog << Simulator::Now ().GetSeconds () << " [node " << GetDevice ()->GetNode ()->GetId () << "]";
#define NS_LOG_APPEND_CONTEXT                                   \
  if (GetDevice () && GetDevice ()->GetNode ()) { \
      std::clog << Simulator::Now ().GetSeconds () \
                << " [node " << GetDevice ()->GetNode ()->GetId () << "] "; }


#include <ns3/waveform-generator.h>
#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <math.h>
#include <ns3/simulator.h>
#include "ns3/spectrum-error-model.h"
#include "ts-ook-based-nano-spectrum-phy.h"
#include "simple-nano-device.h"
#include "nano-spectrum-channel.h"
#include <math.h>
#include "nano-spectrum-signal-parameters.h"


NS_LOG_COMPONENT_DEFINE ("TsOokBasedNanoSpectrumPhy");

namespace ns3 {


NS_OBJECT_ENSURE_REGISTERED (TsOokBasedNanoSpectrumPhy);

TsOokBasedNanoSpectrumPhy::TsOokBasedNanoSpectrumPhy ()
{
  Initialize ();
  SetState (TsOokBasedNanoSpectrumPhy::IDLE);
  m_receivingpackets = new std::list<receivingpacket>;
}

TypeId
TsOokBasedNanoSpectrumPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TsOokBasedNanoSpectrumPhy")
    .SetParent<NanoSpectrumPhy> ()
    .AddTraceSource ("outPHYTX",  "outPHYTX",  MakeTraceSourceAccessor (&TsOokBasedNanoSpectrumPhy::m_outPHYTX),
                     "ns3::TsOokBasedNanoSpectrumPhy::OutPhyTxCallback")
    .AddTraceSource ("outPHYCOLL",  "outPHYCOLL",  MakeTraceSourceAccessor (&TsOokBasedNanoSpectrumPhy::m_outPHYCOLL),
                     "ns3::TsOokBasedNanoSpectrumPhy::OutPhyCollCallback");
  return tid;
}

std::ostream& operator<< (std::ostream& os, TsOokBasedNanoSpectrumPhy::State s)
{
  switch (s)
    {
    case TsOokBasedNanoSpectrumPhy::IDLE:
      os << "IDLE";
      break;
    case TsOokBasedNanoSpectrumPhy::RX:
      os << "RX";
      break;
    case TsOokBasedNanoSpectrumPhy::TX:
      os << "TX";
      break;
    case TsOokBasedNanoSpectrumPhy::TX_RX:
      os << "TX_RX";
      break;
    default:
      os << "UNKNOWN";
      break;
    }
  return os;
}

TsOokBasedNanoSpectrumPhy::~TsOokBasedNanoSpectrumPhy ()
{
}

void
TsOokBasedNanoSpectrumPhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_receivingpackets->clear ();
  delete m_receivingpackets;
  NanoSpectrumPhy::DoDispose ();
}


bool
TsOokBasedNanoSpectrumPhy::StartTx (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  Ptr<NanoSpectrumSignalParameters> txParams = Create<NanoSpectrumSignalParameters> ();
  txParams->duration = m_pulseDuration;
  txParams->psd = GetTxPowerSpectralDensity ();
  txParams->txPhy = GetObject<SpectrumPhy> ();

  double nbPulseIntervals = p->GetSize () * 8 + PHY_BITS_HOVERHEAD - 1;

  double duration = (nbPulseIntervals * m_pulseInterval.GetFemtoSeconds ()) + m_pulseDuration.GetFemtoSeconds ();

  txParams->m_packet = p;
  txParams->m_duration = FemtoSeconds (duration);
  txParams->m_pulseDuration = m_pulseDuration;
  txParams->m_pulseInterval = m_pulseInterval;
  txParams->m_startTime = Simulator::Now ();

  Ptr<NanoSpectrumChannel> c = GetChannel ()->GetObject<NanoSpectrumChannel> ();

  Simulator::Schedule (FemtoSeconds (duration), &TsOokBasedNanoSpectrumPhy::EndTx, this);

  if (GetState () == IDLE)
    {
	  SetState (NanoSpectrumPhy::TX);
    }
  else if (GetState () == RX)
    {
	  SetState (NanoSpectrumPhy::TX_RX);
    }

  Ptr<SimpleNanoDevice> dev = GetDevice ()->GetObject <SimpleNanoDevice>();

  m_outPHYTX ((int)p->GetUid (), (int)GetDevice ()->GetNode ()->GetId ());

  c->StartTx (txParams);
  return true;
}

void
TsOokBasedNanoSpectrumPhy::EndTx ()
{
  NS_LOG_FUNCTION (this);
  if (m_receivingpackets->size () == 0)
    {
      SetState (NanoSpectrumPhy::IDLE);
    }
  else
    {
	  SetState (NanoSpectrumPhy::RX);
    }
}

void
TsOokBasedNanoSpectrumPhy::SetPulseDuration (Time d)
{
  NS_LOG_FUNCTION (this << d);
  m_pulseDuration = d;
}

void
TsOokBasedNanoSpectrumPhy::SetPulseInterval (Time d)
{
  NS_LOG_FUNCTION (this << d);
  m_pulseInterval = d;
}

Time
TsOokBasedNanoSpectrumPhy::GetPulseDuration (void)
{
  NS_LOG_FUNCTION (this);
  return m_pulseDuration;
}

Time
TsOokBasedNanoSpectrumPhy::GetPulseInterval (void)
{
  NS_LOG_FUNCTION (this);
  return m_pulseInterval;
}

void
TsOokBasedNanoSpectrumPhy::StartRx (Ptr<SpectrumSignalParameters> spectrumParams)
{
  NS_LOG_FUNCTION (this << spectrumParams << "from " << spectrumParams->txPhy);
  NS_LOG_LOGIC (this << "state: " << GetState () << m_receivingpackets->size ());

  Ptr<NanoSpectrumSignalParameters> params = DynamicCast<NanoSpectrumSignalParameters> (spectrumParams);

  if (params != 0)
	{

	  Time duration = params->m_duration;

	  NS_LOG_LOGIC (this << " duration " << duration.GetPicoSeconds ());

	  receivingpacket rp;
	  rp.params = DynamicCast<NanoSpectrumSignalParameters> (params->Copy ());
	  rp.correct = true;
	  m_receivingpackets->push_back (rp);

	  if (GetState () == NanoSpectrumPhy::IDLE)
	    {
		  SetState (NanoSpectrumPhy::RX);
	    }
	  else if (GetState () == NanoSpectrumPhy::TX)
	    {
		  SetState (NanoSpectrumPhy::TX_RX);
	    }

	  NS_LOG_LOGIC (this << "state: " << GetState () << m_receivingpackets->size ());

	  Simulator::Schedule (duration, &TsOokBasedNanoSpectrumPhy::EndRx, this, params);
	}
}

void
TsOokBasedNanoSpectrumPhy::EndRx (Ptr<SpectrumSignalParameters> spectrumParams)
{
  NS_LOG_FUNCTION (this << spectrumParams << "state: " << GetState () << m_receivingpackets->size ());

  Ptr<NanoSpectrumSignalParameters> params = DynamicCast<NanoSpectrumSignalParameters> (spectrumParams);
  Ptr<Packet> p = params->m_packet;

  if (!CheckCollision (params))
    {
	  NS_LOG_FUNCTION (this << " receiving the packet at PHY");
	  GetDevice ()->GetObject<SimpleNanoDevice> ()->ReceivePacket (p);
    }
  else
    {
	  NS_LOG_FUNCTION (this << "PHY collision");
	  m_outPHYCOLL ((int)p->GetUid (), (int)GetDevice ()->GetNode ()->GetId ());
    }

  if (GetState () == NanoSpectrumPhy::TX_RX && m_receivingpackets->size () == 0)
    {
	  SetState (NanoSpectrumPhy::TX);
    }
  else if (m_receivingpackets->size () == 0)
    {
	  SetState (NanoSpectrumPhy::IDLE);
    }

  NS_LOG_FUNCTION (this << "state: " << GetState () << m_receivingpackets->size ());
}

bool
TsOokBasedNanoSpectrumPhy::CheckCollision (Ptr<NanoSpectrumSignalParameters> params)
{
  NS_LOG_FUNCTION (this << "tot pkts: " << m_receivingpackets->size ());

  bool collision = false;
  receivingpacket rp;

  if (m_receivingpackets->size () == 1)
    {
	  NS_LOG_FUNCTION (this << "only 1 pkt" << m_receivingpackets->size ());
	  receivingpacket rp = *m_receivingpackets->begin ();
	  collision = !rp.correct;
	  m_receivingpackets->clear ();
	  NS_LOG_FUNCTION (this << collision);
	  return collision;
    }
  else
    {
	  NS_LOG_FUNCTION (this << "more pkts" << m_receivingpackets->size ());
	  std::list<receivingpacket>::iterator it;
	  std::list<receivingpacket> *newvet = new std::list<receivingpacket>;
	  for (it = m_receivingpackets->begin (); it != m_receivingpackets->end (); it++)
	    {
		  receivingpacket rp = *it;
		  NS_LOG_FUNCTION (this << "compare" << rp.params->txPhy << "and" << params->txPhy);
		  if (rp.params->txPhy != params->txPhy)
		    {
			  NS_LOG_FUNCTION (this << "add");
			  newvet->push_back (rp);
		    }
	    }

	  NS_LOG_FUNCTION (this << "new" << newvet->size () << "old" << m_receivingpackets->size ());
	  m_receivingpackets->clear ();
	  delete m_receivingpackets;
	  m_receivingpackets = newvet;
	  NS_LOG_FUNCTION (this << m_receivingpackets->size ());

	  for (it = m_receivingpackets->begin (); it != m_receivingpackets->end (); it++)
		{
		  receivingpacket rp = *it;
		  NS_LOG_FUNCTION (this << "check among" <<
				  rp.params->txPhy <<
				  rp.params->m_pulseInterval <<
				  rp.params->m_startTime
				  << "and" <<
				  params->txPhy <<
				  params->m_pulseInterval <<
				  params->m_startTime);

		  if (rp.params->m_startTime == params->m_startTime)
			{
			  NS_LOG_FUNCTION (this << "collision");
			  collision = true;
			  rp.correct = false;
			}
		  else
			{
			  Time timeDistance = rp.params->m_startTime - params->m_startTime;
			  NS_LOG_FUNCTION (this << timeDistance);
			  if (timeDistance.GetFemtoSeconds () % rp.params->m_pulseInterval.GetFemtoSeconds () == 0)
				{
				  NS_LOG_FUNCTION (this << "collision");
				  collision = true;
				  rp.correct = false;
				}

			}
		}
	  return collision;
    }
}


double
TsOokBasedNanoSpectrumPhy::GetTxDuration (Ptr<Packet> p)
{
  double nbPulseIntervals = p->GetSize () * 8 + PHY_BITS_HOVERHEAD - 1;
  double duration = (nbPulseIntervals * m_pulseInterval.GetFemtoSeconds ()) + m_pulseDuration.GetFemtoSeconds ();
  return duration;
}

} // namespace ns3

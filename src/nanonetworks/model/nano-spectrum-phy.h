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


#ifndef NANO_PHY_H
#define NANO_PHY_H

#include <ns3/spectrum-value.h>
#include <ns3/mobility-model.h>
#include <ns3/packet.h>
#include <ns3/nstime.h>
#include <ns3/net-device.h>
#include <ns3/spectrum-phy.h>
#include <ns3/spectrum-channel.h>
#include <ns3/spectrum-interference.h>
#include <ns3/data-rate.h>
#include <ns3/generic-phy.h>
#include <ns3/event-id.h>
#include <ns3/spectrum-signal-parameters.h>

namespace ns3 {

class SimpleNanoDevice;
class AntennaModel;

/**
 * \ingroup nanonetworks
 *
 * The NanoSpectrumPhy provides a template class for the nanonetwork
 * PHY layer
 */
class NanoSpectrumPhy : public SpectrumPhy
{

public:
  NanoSpectrumPhy ();
  virtual ~NanoSpectrumPhy ();
  void Initialize (void);

  static TypeId GetTypeId (void);
  virtual void DoDispose (void);

  // inherited from SpectrumPhy
  void SetChannel (Ptr<SpectrumChannel> c);
  void SetMobility (Ptr<MobilityModel> m);
  void SetDevice (Ptr<NetDevice> d);
  Ptr<MobilityModel> GetMobility ();
  virtual Ptr<NetDevice> GetDevice () const;
  Ptr<const SpectrumModel> GetRxSpectrumModel () const;
  virtual void StartRx (Ptr<SpectrumSignalParameters> params) = 0;

  void SetTxPower (double p);

  /**
   * set the Power Spectral Density of outgoing signals in power units
   * (Watt, Pascal...) per Hz.
   *
   * @param txPsd
   */
  void SetTxPowerSpectralDensity (Ptr<SpectrumValue> txPsd);

  /**
   * get the Power Spectral Density of outgoing signals in power units
   * (Watt, Pascal...) per Hz.
   *
   * @return txPsd
   */
  Ptr<SpectrumValue> GetTxPowerSpectralDensity (void);

  Ptr<SpectrumChannel> GetChannel (void);

  /**
   *
   * @param noisePsd the Noise Power Spectral Density in power units
   * (Watt, Pascal...) per Hz.
   */
  void SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd);


  /**
   * Start a transmission
   *
   *
   * @param p the packet to be transmitted
   *
   * @return true if an error occurred and the transmission was not
   * started, false otherwise.
   */
  virtual bool StartTx (Ptr<Packet> p) = 0;



  /**
   * set the callback for the end of a TX, as part of the
   * interconnections betweenthe PHY and the MAC
   *
   * @param c the callback
   */
  void SetGenericPhyTxEndCallback (GenericPhyTxEndCallback c);

  /**
   * set the callback for the start of RX, as part of the
   * interconnections betweenthe PHY and the MAC
   *
   * @param c the callback
   */
  void SetGenericPhyRxStartCallback (GenericPhyRxStartCallback c);

  /**
   * set the callback for the end of a RX in error, as part of the
   * interconnections betweenthe PHY and the MAC
   *
   * @param c the callback
   */
  void SetGenericPhyRxEndErrorCallback (GenericPhyRxEndErrorCallback c);

  /**
   * set the callback for the successful end of a RX, as part of the
   * interconnections betweenthe PHY and the MAC
   *
   * @param c the callback
   */
  void SetGenericPhyRxEndOkCallback (GenericPhyRxEndOkCallback c);

  /**
   *  PHY states
   */
  enum State
  {
    IDLE, TX, RX, TX_RX
  };

  /**
   * \brief Set the state of the phy layer
   * \param newState the state
   */
  void SetState (State newState);
  State GetState ();

  virtual double GetTxDuration (Ptr<Packet> p) = 0;

  void SetTransmissionRange (double r);
  double GetTransmissionRange (void);


  virtual Ptr<AntennaModel> GetRxAntenna ();

private:

  State m_state;
  EventId m_endRxEventId;
  Ptr<MobilityModel> m_mobility;
  Ptr<NetDevice> m_netDevice;
  Ptr<SpectrumChannel> m_channel;
  double m_txPower;
  Ptr<SpectrumValue> m_txPsd;
  Ptr<const SpectrumValue> m_rxPsd;

  TracedCallback<Ptr<const Packet> > m_phyTxStartTrace;
  TracedCallback<Ptr<const Packet> > m_phyTxEndTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxStartTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxAbortTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxEndOkTrace;
  TracedCallback<Ptr<const Packet> > m_phyRxEndErrorTrace;

  GenericPhyTxEndCallback        m_phyMacTxEndCallback;
  GenericPhyRxStartCallback      m_phyMacRxStartCallback;
  GenericPhyRxEndErrorCallback   m_phyMacRxEndErrorCallback;
  GenericPhyRxEndOkCallback      m_phyMacRxEndOkCallback;

  SpectrumInterference m_interference;

  double m_transmissionRange;
};






}


#endif /* NANO_PHY_H */

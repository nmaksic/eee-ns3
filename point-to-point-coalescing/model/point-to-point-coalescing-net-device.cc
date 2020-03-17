/*
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
 * Extended from implementation of point to point net device included in ns-3.30.1 available at
 * https://www.nsnam.org/releases/ns-3-30/
 * 
 * by Natasa Maksic, maksicn@etf.rs
*/


#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "point-to-point-coalescing-net-device.h"
#include "point-to-point-coalescing-channel.h"
#include "ppp-header-coalescing.h"

#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointCoalescingNetDevice");

NS_OBJECT_ENSURE_REGISTERED (PointToPointCoalescingNetDevice);

TypeId 
PointToPointCoalescingNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointCoalescingNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("PointToPointCoalescing")
    .AddConstructor<PointToPointCoalescingNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (DEFAULT_MTU),
                   MakeUintegerAccessor (&PointToPointCoalescingNetDevice::SetMtu,
                                         &PointToPointCoalescingNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Address", 
                   "The MAC address of this device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&PointToPointCoalescingNetDevice::m_address),
                   MakeMac48AddressChecker ())
    .AddAttribute ("DataRate", 
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("32768b/s")),
                   MakeDataRateAccessor (&PointToPointCoalescingNetDevice::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("ReceiveErrorModel", 
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointCoalescingNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("InterframeGap", 
                   "The time to wait between packet (frame) transmissions",
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&PointToPointCoalescingNetDevice::m_tInterframeGap),
                   MakeTimeChecker ())

    //
    // Transmit queueing discipline for the device which includes its own set
    // of trace hooks.
    //
    .AddAttribute ("TxQueue", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointCoalescingNetDevice::m_queue),
                   MakePointerChecker<Queue<Packet> > ())

    //
    // Energy Efficient Ethernet attributes
    //   
	.AddAttribute ("EeeCoalescingTimeout", "EEE coalescing timeout in microseconds",
					   DoubleValue (800),
 					   MakeDoubleAccessor (&PointToPointCoalescingNetDevice::m_eeeTimeout),
					   MakeDoubleChecker<double> ())
	.AddAttribute ("EeeByteLimit", "EEE coalescing byte limit",
					   DoubleValue (24000),
 					   MakeDoubleAccessor (&PointToPointCoalescingNetDevice::m_eeeByteLimit),
					   MakeDoubleChecker<double> ())
	.AddAttribute ("EeeSleepTime", "Duration of transition to low-power state in microseconds",
					   DoubleValue (2.88),
 					   MakeDoubleAccessor (&PointToPointCoalescingNetDevice::m_eeeSleepTime),
					   MakeDoubleChecker<double> ())
	.AddAttribute ("EeeWakeUpTime", "Duration of transition to active state in microseconds",
					   DoubleValue (4.48),
 					   MakeDoubleAccessor (&PointToPointCoalescingNetDevice::m_eeeWakeupTime),
					   MakeDoubleChecker<double> ())

    //
    // Trace sources at the "top" of the net device, where packets transition
    // to/from higher layers.
    //
    .AddTraceSource ("MacTx", 
                     "Trace source indicating a packet has arrived "
                     "for transmission by this device",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop", 
                     "Trace source indicating a packet has been dropped "
                     "by the device before transmission",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacPromiscRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_macPromiscRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("MacRxDrop", 
                     "Trace source indicating a packet was dropped "
                     "before being forwarded up the stack",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
#endif
    //
    // Trace sources at the "bottom" of the net device, where packets transition
    // to/from the channel.
    //
    .AddTraceSource ("PhyTxBegin", 
                     "Trace source indicating a packet has begun "
                     "transmitting over the channel",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_phyTxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxEnd", 
                     "Trace source indicating a packet has been "
                     "completely transmitted over the channel",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during transmission",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_phyTxDropTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("PhyRxBegin", 
                     "Trace source indicating a packet has begun "
                     "being received by the device",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
#endif
    .AddTraceSource ("PhyRxEnd", 
                     "Trace source indicating a packet has been "
                     "completely received by the device",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_phyRxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during reception",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")

    //
    // Trace sources designed to simulate a packet sniffer facility (tcpdump).
    // Note that there is really no difference between promiscuous and 
    // non-promiscuous traces in a point-to-point link.
    //
    .AddTraceSource ("Sniffer", 
                    "Trace source simulating a non-promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_snifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PromiscSniffer", 
                     "Trace source simulating a promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&PointToPointCoalescingNetDevice::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


PointToPointCoalescingNetDevice::PointToPointCoalescingNetDevice () 
  :
    m_txMachineState (READY),
    m_channel (0),
    m_linkUp (false),
    m_currentPkt (0),
    m_coalescingTimerCycle (0),
    m_coalescingState (COALESCING_LOWPOWER),
    m_coalescingTimerState (false), 
    m_lpTimeNs(0),
    m_lpIntervals(0),
    m_packetCount(0),
    m_sumInterarrivalNs (0),
    m_lastPacketArrivalNs(0),
    m_packetBytes(0)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (Simulator::Now() << ": m_coalescingState = COALESCING_LOWPOWER initialize 1"); 
  m_lowPowerStart = Simulator::Now();
}

PointToPointCoalescingNetDevice::~PointToPointCoalescingNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
PointToPointCoalescingNetDevice::AddHeader (Ptr<Packet> p, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << protocolNumber);
  PppHeaderCoalescing ppp;
  ppp.SetProtocol (EtherToPpp (protocolNumber));
  p->AddHeader (ppp);
}

bool
PointToPointCoalescingNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t& param)
{
  NS_LOG_FUNCTION (this << p << param);
  PppHeaderCoalescing ppp;
  p->RemoveHeader (ppp);
  param = PppToEther (ppp.GetProtocol ());
  return true;
}

void
PointToPointCoalescingNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_channel = 0;
  m_receiveErrorModel = 0;
  m_currentPkt = 0;
  m_queue = 0;
  NetDevice::DoDispose ();
}

void
PointToPointCoalescingNetDevice::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this);
  m_bps = bps;
}

void
PointToPointCoalescingNetDevice::SetInterframeGap (Time t)
{
  NS_LOG_FUNCTION (this << t.GetSeconds ());
  m_tInterframeGap = t;
}

bool
PointToPointCoalescingNetDevice::TransmitStart (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  //
  // This function is called to start the process of transmitting a packet.
  // We need to tell the channel that we've started wiggling the wire and
  // schedule an event that will be executed when the transmission is complete.
  //
  NS_ASSERT_MSG (m_txMachineState == READY, "Must be READY to transmit");
  m_txMachineState = BUSY;
  m_currentPkt = p;
  m_phyTxBeginTrace (m_currentPkt);

  Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
  Time txCompleteTime = txTime + m_tInterframeGap;

  NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.GetNanoSeconds () << "ns");
  Simulator::Schedule (txCompleteTime, &PointToPointCoalescingNetDevice::TransmitComplete, this);

  bool result = m_channel->TransmitStart (p, this, txTime);
  if (result == false)
    {
      m_phyTxDropTrace (p);
    }

  m_packetCount++;
  m_packetBytes+=p->GetSize();

  return result;
}

void
PointToPointCoalescingNetDevice::TransmitComplete (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_LOGIC (Simulator::Now() << " switch: " << GetNode()->GetId() << " " << GetIfIndex() << ": TransmitComplete");

  //
  // This function is called to when we're all done transmitting a packet.
  // We try and pull another packet off of the transmit queue.  If the queue
  // is empty, we are done, otherwise we need to start transmitting the
  // next packet.
  //
  NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");
  m_txMachineState = READY;

  NS_ASSERT_MSG (m_currentPkt != 0, "PointToPointCoalescingNetDevice::TransmitComplete(): m_currentPkt zero");

  m_phyTxEndTrace (m_currentPkt);
  m_currentPkt = 0;

  Ptr<Packet> p = m_queue->Dequeue ();
  if (p == 0)
    {
      CoalescingQueueEmptied();
      NS_LOG_LOGIC ("No pending packets in device queue after tx complete");
      return;
    }

  //
  // Got another packet off of the queue, so start the transmit process again.
  //
  m_snifferTrace (p);
  m_promiscSnifferTrace (p);
  TransmitStart (p);
}

bool
PointToPointCoalescingNetDevice::Attach (Ptr<PointToPointCoalescingChannel> ch)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch;

  m_channel->Attach (this);

  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
  return true;
}

void
PointToPointCoalescingNetDevice::SetQueue (Ptr<Queue<Packet> > q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
PointToPointCoalescingNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
PointToPointCoalescingNetDevice::Receive (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) ) 
    {
      // 
      // If we have an error model and it indicates that it is time to lose a
      // corrupted packet, don't forward this packet up, let it go.
      //
      m_phyRxDropTrace (packet);
    }
  else 
    {
      // 
      // Hit the trace hooks.  All of these hooks are in the same place in this 
      // device because it is so simple, but this is not usually the case in
      // more complicated devices.
      //
      m_snifferTrace (packet);
      m_promiscSnifferTrace (packet);
      m_phyRxEndTrace (packet);

      //
      // Trace sinks will expect complete packets, not packets without some of the
      // headers.
      //
      Ptr<Packet> originalPacket = packet->Copy ();

      //
      // Strip off the point-to-point protocol header and forward this packet
      // up the protocol stack.  Since this is a simple point-to-point link,
      // there is no difference in what the promisc callback sees and what the
      // normal receive callback sees.
      //
      ProcessHeader (packet, protocol);

      if (!m_promiscCallback.IsNull ())
        {
          m_macPromiscRxTrace (originalPacket);
          m_promiscCallback (this, packet, protocol, GetRemote (), GetAddress (), NetDevice::PACKET_HOST);
        }

      m_macRxTrace (originalPacket);
      m_rxCallback (this, packet, protocol, GetRemote ());
    }
}

Ptr<Queue<Packet> >
PointToPointCoalescingNetDevice::GetQueue (void) const
{ 
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
PointToPointCoalescingNetDevice::NotifyLinkUp (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

void
PointToPointCoalescingNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this);
  m_ifIndex = index;
}

uint32_t
PointToPointCoalescingNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
PointToPointCoalescingNetDevice::GetChannel (void) const
{
  return m_channel;
}

//
// This is a point-to-point device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
PointToPointCoalescingNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
PointToPointCoalescingNetDevice::GetAddress (void) const
{
  return m_address;
}

bool
PointToPointCoalescingNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

void
PointToPointCoalescingNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
PointToPointCoalescingNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

//
// We don't really need any addressing information since this is a 
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
PointToPointCoalescingNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
PointToPointCoalescingNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
PointToPointCoalescingNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("01:00:5e:00:00:00");
}

Address
PointToPointCoalescingNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address ("33:33:00:00:00:00");
}

bool
PointToPointCoalescingNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
PointToPointCoalescingNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
PointToPointCoalescingNetDevice::Send (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());

  //
  // If IsLinkUp() is false it means there is no channel to send any packet 
  // over so we just hit the drop trace on the packet and return an error.
  //
  if (IsLinkUp () == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  //
  // Stick a point to point protocol header on the packet in preparation for
  // shoving it out the door.
  //
  AddHeader (packet, protocolNumber);

   

  m_macTxTrace (packet);

  //
  // We should enqueue and dequeue the packet to hit the tracing hooks.
  //
  CoalescingCheckTimer(); 
  if (m_queue->Enqueue (packet))
    {
      //
      // If the channel is ready for transition we send the packet right now
      //
      double timeNs = Simulator::Now().GetNanoSeconds();
      if (m_lastPacketArrivalNs > 0) 
         m_sumInterarrivalNs += timeNs - m_lastPacketArrivalNs;

      m_lastPacketArrivalNs = timeNs;
      
      CoalescingQueueLimit();
      if (m_coalescingState == COALESCING_SEND)
      if (m_txMachineState == READY)
        {
          packet = m_queue->Dequeue ();
          m_snifferTrace (packet);
          m_promiscSnifferTrace (packet);
          bool ret = TransmitStart (packet);
          return ret;
        }
      return true;
    }

  // Enqueue may fail (overflow)

  m_macTxDropTrace (packet);
  return false;
}

bool
PointToPointCoalescingNetDevice::SendFrom (Ptr<Packet> packet, 
                                 const Address &source, 
                                 const Address &dest, 
                                 uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  return false;
}

Ptr<Node>
PointToPointCoalescingNetDevice::GetNode (void) const
{
  return m_node;
}

void
PointToPointCoalescingNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;
}

bool
PointToPointCoalescingNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
PointToPointCoalescingNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  m_rxCallback = cb;
}

void
PointToPointCoalescingNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  m_promiscCallback = cb;
}

bool
PointToPointCoalescingNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
PointToPointCoalescingNetDevice::DoMpiReceive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  Receive (p);
}

Address 
PointToPointCoalescingNetDevice::GetRemote (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_channel->GetNDevices () == 2);
  for (std::size_t i = 0; i < m_channel->GetNDevices (); ++i)
    {
      Ptr<NetDevice> tmp = m_channel->GetDevice (i);
      if (tmp != this)
        {
          return tmp->GetAddress ();
        }
    }
  NS_ASSERT (false);
  // quiet compiler.
  return Address ();
}

bool
PointToPointCoalescingNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
PointToPointCoalescingNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

uint16_t
PointToPointCoalescingNetDevice::PppToEther (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  switch(proto)
    {
    case 0x0021: return 0x0800;   //IPv4
    case 0x0057: return 0x86DD;   //IPv6
    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  return 0;
}

uint16_t
PointToPointCoalescingNetDevice::EtherToPpp (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  switch(proto)
    {
    case 0x0800: return 0x0021;   //IPv4
    case 0x86DD: return 0x0057;   //IPv6
    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  return 0;
}


void
PointToPointCoalescingNetDevice::CoalescingTimeOut(uint64_t coalescingTimerCycle) {

   //NS_LOG_LOGIC ("CoalescingTimeOut");
   
   if (coalescingTimerCycle == m_coalescingTimerCycle && m_coalescingState == COALESCING_LOWPOWER) {
      m_coalescingTimerState = false;
      m_coalescingState = COALESCING_WAKEUP;
      Simulator::Schedule (MicroSeconds (m_eeeWakeupTime), &PointToPointCoalescingNetDevice::CoalescingWakeUp, this);
      NS_LOG_LOGIC (Simulator::Now() << " switch: " << GetNode()->GetId() << " " << GetIfIndex() << ": m_coalescingState = COALESCINGWAKEUP on timeout");
   }
}

void
PointToPointCoalescingNetDevice::CoalescingSleep() {

   //NS_LOG_LOGIC ("CoalescingTimeOut");
   
   if (m_coalescingState == COALESCING_SLEEP) {
      m_coalescingState = COALESCING_LOWPOWER;
      NS_LOG_LOGIC (Simulator::Now() << " switch: " << GetNode()->GetId() << " " << GetIfIndex() << ": m_coalescingState = COALESCING_LOWPOWER");
      m_lowPowerStart = Simulator::Now();
   }
}


void
PointToPointCoalescingNetDevice::CoalescingQueueEmptied() {

   m_coalescingTimerCycle++;
   m_coalescingState = COALESCING_SLEEP;
   Simulator::Schedule (MicroSeconds (m_eeeWakeupTime), &PointToPointCoalescingNetDevice::CoalescingSleep, this);
   NS_LOG_LOGIC (Simulator::Now() << " switch: " << GetNode()->GetId() << " " << GetIfIndex() << ": m_coalescingState = COALESCING_SLEEP");
   
}

void
PointToPointCoalescingNetDevice::CoalescingWakeUp() {

   //NS_LOG_LOGIC ("CoalescingTimeOut");
   
   if (m_coalescingState == COALESCING_WAKEUP) {
      m_coalescingState = COALESCING_SEND;
      NS_LOG_LOGIC (Simulator::Now() << " switch: " << GetNode()->GetId() << " " << GetIfIndex() << ": m_coalescingState = COALESCING_SEND " << m_lpIntervals);

      // update counters
      if (m_lpIntervals > 0) {
         Time t = Simulator::Now() - m_lowPowerStart;
         m_lpTimeNs+=t.GetNanoSeconds () ;
      }

      m_lpIntervals++;
   
      // start sending it there are packets
      Ptr<Packet> p = m_queue->Dequeue ();
      if (p == 0)
    {
      CoalescingQueueEmptied();
      NS_LOG_LOGIC ("Error:No pending packets in device queue after wakeup");
      return;
    }

  //
  // Got another packet off of the queue, so start the transmit process again.
  //
  m_snifferTrace (p);
  m_promiscSnifferTrace (p);
  TransmitStart (p);

   }
}

void
PointToPointCoalescingNetDevice::CoalescingQueueLimit() {

   uint32_t queueBytes = m_queue->GetNBytes();

   if (m_coalescingState == COALESCING_LOWPOWER && queueBytes >= m_eeeByteLimit) {
      m_coalescingState = COALESCING_WAKEUP;
      m_coalescingTimerState = false;
      Simulator::Schedule (MicroSeconds (m_eeeWakeupTime), &PointToPointCoalescingNetDevice::CoalescingWakeUp, this);
      NS_LOG_LOGIC (Simulator::Now() << " switch: " << GetNode()->GetId() << " " << GetIfIndex() << ": m_coalescingState = COALESCING_WAKEUP on byte limit");
   }

}

void
PointToPointCoalescingNetDevice::CoalescingCheckTimer() {

   uint32_t queueBytes = m_queue->GetNBytes();
   
   NS_LOG_LOGIC (Simulator::Now() << " switch: " << GetNode()->GetId() << " " << GetIfIndex() << ": queueBytes: " << queueBytes);

   // Checks m_coalescingTimerState = false in order not to repeat 
   if (queueBytes == 0 && m_coalescingTimerState == false) {
      
      Simulator::Schedule (MicroSeconds (m_eeeTimeout), &PointToPointCoalescingNetDevice::CoalescingTimeOut, this, m_coalescingTimerCycle);
      m_coalescingTimerState = true;
      NS_LOG_LOGIC (Simulator::Now() << " switch: " << GetNode()->GetId() << " " << GetIfIndex() << ": m_coalescingTimerState = true");
   }
}

void 
PointToPointCoalescingNetDevice::WriteMeasurementsData (std::string s) {

  std::ofstream outfile;
  outfile.open("data.txt", std::ios_base::app); // append instead of overwrite
  uint64_t linkspeed = 0;
  if (s.compare("10Gbps") == 0) 
      linkspeed = 10000000000;
  if (s.compare("1Gbps") == 0) 
      linkspeed = 1000000000;
  if (s.compare("5Gbps") == 0) 
      linkspeed = 5000000000;
  if (m_lpIntervals > 0)
      m_lpIntervals--; // first interval is not added to m_lpTimeNs because flows may start later in the simulation
  outfile << GetNode()->GetId() << " " << GetIfIndex() << " " << m_lpTimeNs << " " << m_lpIntervals << " " << m_packetCount << " " << m_packetBytes << " " << m_sumInterarrivalNs / 1e9 / (m_packetCount-1) << " " << linkspeed << std::endl; 




}

} // namespace ns3

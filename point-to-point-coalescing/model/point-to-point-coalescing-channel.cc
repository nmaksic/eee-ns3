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

#include "point-to-point-coalescing-channel.h"
#include "point-to-point-coalescing-net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointCoalescingChannel");

NS_OBJECT_ENSURE_REGISTERED (PointToPointCoalescingChannel);

TypeId 
PointToPointCoalescingChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointCoalescingChannel")
    .SetParent<Channel> ()
    .SetGroupName ("PointToPointCoalescing")
    .AddConstructor<PointToPointCoalescingChannel> ()
    .AddAttribute ("Delay", "Propagation delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PointToPointCoalescingChannel::m_delay),
                   MakeTimeChecker ())
    .AddTraceSource ("TxRxPointToPointCoalescing",
                     "Trace source indicating transmission of packet "
                     "from the PointToPointCoalescingChannel, used by the Animation "
                     "interface.",
                     MakeTraceSourceAccessor (&PointToPointCoalescingChannel::m_txrxPointToPointCoalescing),
                     "ns3::PointToPointCoalescingChannel::TxRxAnimationCallback")
  ;
  return tid;
}

//
// By default, you get a channel that 
// has an "infitely" fast transmission speed and zero delay.
PointToPointCoalescingChannel::PointToPointCoalescingChannel()
  :
    Channel (),
    m_delay (Seconds (0.)),
    m_nDevices (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
PointToPointCoalescingChannel::Attach (Ptr<PointToPointCoalescingNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG (m_nDevices < N_DEVICES, "Only two devices permitted");
  NS_ASSERT (device != 0);

  m_link[m_nDevices++].m_src = device;
//
// If we have both devices connected to the channel, then finish introducing
// the two halves and set the links to IDLE.
//
  if (m_nDevices == N_DEVICES)
    {
      m_link[0].m_dst = m_link[1].m_src;
      m_link[1].m_dst = m_link[0].m_src;
      m_link[0].m_state = IDLE;
      m_link[1].m_state = IDLE;
    }
}

bool
PointToPointCoalescingChannel::TransmitStart (
  Ptr<const Packet> p,
  Ptr<PointToPointCoalescingNetDevice> src,
  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);

  uint32_t wire = src == m_link[0].m_src ? 0 : 1;

  Simulator::ScheduleWithContext (m_link[wire].m_dst->GetNode ()->GetId (),
                                  txTime + m_delay, &PointToPointCoalescingNetDevice::Receive,
                                  m_link[wire].m_dst, p->Copy ());

  // Call the tx anim callback on the net device
  m_txrxPointToPointCoalescing (p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
  return true;
}

std::size_t
PointToPointCoalescingChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<PointToPointCoalescingNetDevice>
PointToPointCoalescingChannel::GetPointToPointCoalescingDevice (std::size_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (i < 2);
  return m_link[i].m_src;
}

Ptr<NetDevice>
PointToPointCoalescingChannel::GetDevice (std::size_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return GetPointToPointCoalescingDevice (i);
}

Time
PointToPointCoalescingChannel::GetDelay (void) const
{
  return m_delay;
}

Ptr<PointToPointCoalescingNetDevice>
PointToPointCoalescingChannel::GetSource (uint32_t i) const
{
  return m_link[i].m_src;
}

Ptr<PointToPointCoalescingNetDevice>
PointToPointCoalescingChannel::GetDestination (uint32_t i) const
{
  return m_link[i].m_dst;
}

bool
PointToPointCoalescingChannel::IsInitialized (void) const
{
  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);
  return true;
}

} // namespace ns3

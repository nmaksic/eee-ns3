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

#include <iostream>

#include "point-to-point-coalescing-remote-channel.h"
#include "point-to-point-coalescing-net-device.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/mpi-interface.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointCoalescingRemoteChannel");

NS_OBJECT_ENSURE_REGISTERED (PointToPointCoalescingRemoteChannel);

TypeId
PointToPointCoalescingRemoteChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointCoalescingRemoteChannel")
    .SetParent<PointToPointCoalescingChannel> ()
    .SetGroupName ("PointToPointCoalescing")
    .AddConstructor<PointToPointCoalescingRemoteChannel> ()
  ;
  return tid;
}

PointToPointCoalescingRemoteChannel::PointToPointCoalescingRemoteChannel ()
  : PointToPointCoalescingChannel ()
{
}

PointToPointCoalescingRemoteChannel::~PointToPointCoalescingRemoteChannel ()
{
}

bool
PointToPointCoalescingRemoteChannel::TransmitStart (
  Ptr<const Packet> p,
  Ptr<PointToPointCoalescingNetDevice> src,
  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  IsInitialized ();

  uint32_t wire = src == GetSource (0) ? 0 : 1;
  Ptr<PointToPointCoalescingNetDevice> dst = GetDestination (wire);

#ifdef NS3_MPI
  // Calculate the rxTime (absolute)
  Time rxTime = Simulator::Now () + txTime + GetDelay ();
  MpiInterface::SendPacket (p->Copy (), rxTime, dst->GetNode ()->GetId (), dst->GetIfIndex ());
#else
  NS_FATAL_ERROR ("Can't use distributed simulator without MPI compiled in");
#endif
  return true;
}

} // namespace ns3

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

// This object connects two point-to-point net devices where at least one
// is not local to this simulator object.  It simply over-rides the transmit
// method and uses an MPI Send operation instead.

#ifndef POINT_TO_POINT_COALESCING_REMOTE_CHANNEL_H
#define POINT_TO_POINT_COALESCING_REMOTE_CHANNEL_H

#include "point-to-point-coalescing-channel.h"

namespace ns3 {

/**
 * \ingroup point-to-point
 *
 * \brief A Remote Point-To-Point Channel
 * 
 * This object connects two point-to-point net devices where at least one
 * is not local to this simulator object. It simply override the transmit
 * method and uses an MPI Send operation instead.
 */
class PointToPointCoalescingRemoteChannel : public PointToPointCoalescingChannel
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /** 
   * \brief Constructor
   */
  PointToPointCoalescingRemoteChannel ();

  /** 
   * \brief Deconstructor
   */
  ~PointToPointCoalescingRemoteChannel ();

  /**
   * \brief Transmit the packet
   *
   * \param p Packet to transmit
   * \param src Source PointToPointCoalescingNetDevice
   * \param txTime Transmit time to apply
   * \returns true if successful (currently always true)
   */
  virtual bool TransmitStart (Ptr<const Packet> p, Ptr<PointToPointCoalescingNetDevice> src,
                              Time txTime);
};

} // namespace ns3

#endif



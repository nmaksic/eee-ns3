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
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ppp-header-coalescing.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PppHeaderCoalescing");

NS_OBJECT_ENSURE_REGISTERED (PppHeaderCoalescing);

PppHeaderCoalescing::PppHeaderCoalescing ()
{
}

PppHeaderCoalescing::~PppHeaderCoalescing ()
{
}

TypeId
PppHeaderCoalescing::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PppHeaderCoalescing")
    .SetParent<Header> ()
    .SetGroupName ("PointToPointCoalescing")
    .AddConstructor<PppHeaderCoalescing> ()
  ;
  return tid;
}

TypeId
PppHeaderCoalescing::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
PppHeaderCoalescing::Print (std::ostream &os) const
{
  std::string proto;

  switch(m_protocol)
    {
    case 0x0021: /* IPv4 */
      proto = "IP (0x0021)";
      break;
    case 0x0057: /* IPv6 */
      proto = "IPv6 (0x0057)";
      break;
    default:
      NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  os << "Point-to-Point Protocol: " << proto; 
}

uint32_t
PppHeaderCoalescing::GetSerializedSize (void) const
{
  return 2;
}

void
PppHeaderCoalescing::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU16 (m_protocol);
}

uint32_t
PppHeaderCoalescing::Deserialize (Buffer::Iterator start)
{
  m_protocol = start.ReadNtohU16 ();
  return GetSerializedSize ();
}

void
PppHeaderCoalescing::SetProtocol (uint16_t protocol)
{
  m_protocol=protocol;
}

uint16_t
PppHeaderCoalescing::GetProtocol (void)
{
  return m_protocol;
}


} // namespace ns3

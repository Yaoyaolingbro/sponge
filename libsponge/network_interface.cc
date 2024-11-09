#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

EthernetFrame NetworkInterface::broadcast_frame(uint32_t ip) {
    ARPMessage arp;
    arp.opcode = ARPMessage::OPCODE_REQUEST;
    arp.sender_ethernet_address = _ethernet_address;
    arp.sender_ip_address = _ip_address.ipv4_numeric();
    arp.target_ethernet_address = {};
    arp.target_ip_address = ip;

    EthernetFrame frame;
    frame.header() = EthernetHeader{ETHERNET_BROADCAST, _ethernet_address, EthernetHeader::TYPE_ARP};
    frame.payload() = arp.serialize();

    return frame;
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    auto it = _arp_cache.find(next_hop_ip);
    if (it == _arp_cache.end()) {
        // save datagram in cache
        _cache.emplace_back(next_hop, dgram);
        if (_waiting_arps.find(next_hop_ip) == _waiting_arps.end()) {
            // Broadcast ARP request
            auto frame = broadcast_frame(next_hop_ip);
            _frames_out.push(frame);
            _waiting_arps[next_hop_ip] = _time;
        }
    } else {
        // send Ethernet frame
        EthernetFrame frame;
        frame.header() = EthernetHeader{it->second.first, _ethernet_address, EthernetHeader::TYPE_IPv4};
        frame.payload() = dgram.serialize();
        _frames_out.push(frame);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    auto &header = frame.header();

    if (header.dst != _ethernet_address && header.dst != ETHERNET_BROADCAST) {
        return nullopt;
    }

    if (header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError) {
            return dgram;
        }
        return nullopt;
    } else if (header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp;
        if (arp.parse(frame.payload()) != ParseResult::NoError) {
            return nullopt;
        }

        // ARP request
        auto sender_ethernet_address = arp.sender_ethernet_address;
        auto sender_ip_address = arp.sender_ip_address;
        if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == _ip_address.ipv4_numeric()) {
            ARPMessage reply;
            reply.opcode = ARPMessage::OPCODE_REPLY;
            reply.sender_ethernet_address = _ethernet_address;
            reply.sender_ip_address = _ip_address.ipv4_numeric();
            reply.target_ethernet_address = sender_ethernet_address;
            reply.target_ip_address = sender_ip_address;

            EthernetFrame reply_frame;
            reply_frame.header() = EthernetHeader{sender_ethernet_address, _ethernet_address, EthernetHeader::TYPE_ARP};
            reply_frame.payload() = reply.serialize();
            _frames_out.push(reply_frame);
        }

        // ARP update cache
        _arp_cache[sender_ip_address] = {sender_ethernet_address, _time};
        _waiting_arps.erase(sender_ip_address);

        // send some waiting datagrams
        for (auto it = _cache.begin(); it != _cache.end();) {
            if (it->first.ipv4_numeric() == sender_ip_address) {
                send_datagram(it->second, it->first);
                it = _cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    return nullopt;
}


//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    constexpr size_t ARP_TIMEOUT = 30 * 1000;
    constexpr size_t ARP_REQUEST_INTERVAL = 5 * 1000;
    _time += ms_since_last_tick;

    // check ARP cache
    for (auto it = _arp_cache.begin(); it != _arp_cache.end();) {
        if (_time - it->second.second >= ARP_TIMEOUT) {
            it = _arp_cache.erase(it);
        } else {
            ++it;
        }
    }

    // check ARP waitlist
    for (auto it = _waiting_arps.begin(); it != _waiting_arps.end();) {
        if (_time - it->second >= ARP_REQUEST_INTERVAL) {
            // Broadcast ARP request
            auto frame = broadcast_frame(it->first);
            _frames_out.push(frame);
            it->second = _time;
        }
        ++it;
    }
}

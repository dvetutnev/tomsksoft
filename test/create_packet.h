#pragma once

#include <uvw.hpp>
#include <algorithm>


inline uvw::DataEvent createPacket(const std::string& data) {
    std::uint32_t header = ::htonl(data.size());
    std::size_t packetLength = sizeof(header) + data.size();

    uvw::DataEvent packet{std::make_unique<char[]>(packetLength), packetLength};

    std::copy_n(reinterpret_cast<const char*>(&header), sizeof(header), packet.data.get());
    std::copy_n(data.data(), data.size(), packet.data.get() + sizeof(header));

    return packet;
};


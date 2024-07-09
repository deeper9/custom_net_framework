#include "address.h"
#include <sstream>
#include <string.h>

namespace sylar
{

int Address::getFamily() const
{
    return getAddr()->sa_family;
}

std::string Address::toString()
{
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const
{
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);

    if (result < 0) {
        return true;
    } else if (result > 0) {
        return false;
    } else if (getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

bool Address::operator==(const Address& rhs) const
{
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const
{
    return !(*this == rhs);
}

IPv4Address::IPv4Address(uint32_t address, uint32_t port)
{

}

const sockaddr* IPv4Address::getAddr() const
{
    return nullptr;
}

socklen_t IPv4Address::getAddrLen() const
{
    return 0;
}

std::ostream& IPv4Address::insert(std::ostream& os) const
{
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)
{
    return nullptr;
}    

IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len)
{
    return nullptr;
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len)
{
    return nullptr;
}

uint32_t IPv4Address::getPort() const
{
    return 0;
}

void IPv4Address::setPort(uint32_t v)
{

}

IPv6Address::IPv6Address(uint32_t address, uint32_t port)
{

}

const sockaddr* IPv6Address::getAddr() const
{
    return nullptr;
}

socklen_t IPv6Address::getAddrLen() const
{
    return 0;
}

std::ostream& IPv6Address::insert(std::ostream& os) const
{
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
{
    return nullptr;
}    

IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len)
{
    return nullptr;
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
{
    return nullptr;
}

uint32_t IPv6Address::getPort() const
{
    return 0;
}

void IPv6Address::setPort(uint32_t v)
{
    
}

 UnixAddress::UnixAddress(const std::string& path)
 {

 }

const sockaddr* UnixAddress::getAddr() const
{
    return nullptr;
}

socklen_t UnixAddress::getAddrLen() const
{
    return 0;
}

std::ostream& UnixAddress::insert(std::ostream& os) const
{
    return os;
}

const sockaddr* UnknowAddress::getAddr() const
{
    return nullptr;
}

socklen_t UnknowAddress::getAddrLen() const
{
    return 0;
}

std::ostream& UnknowAddress::insert(std::ostream& os) const
{
    return os;
}

}
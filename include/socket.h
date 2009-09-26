/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2009 InspIRCd Development Team
 * See: http://wiki.inspircd.org/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#ifndef INSPIRCD_SOCKET_H
#define INSPIRCD_SOCKET_H

#ifndef WIN32

#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#else

#include "inspircd_win32wrapper.h"

#endif

#include <cerrno>
#include "socketengine.h"

/* Contains irc-specific definitions */
namespace irc
{
	/** This namespace contains various protocol-independent helper classes.
	 * It also contains some types which are often used by the core and modules
	 * in place of inet_* functions and types.
	 */
	namespace sockets
	{

		typedef union {
			struct sockaddr sa;
			struct sockaddr_in in4;
			struct sockaddr_in6 in6;
		} sockaddrs;

		/** Match raw binary data using CIDR rules.
		 *
		 * This function will use binary comparison to compare the
		 * two bit sequences, address and mask, up to mask_bits
		 * bits in size. If they match, it will return true.
		 * @param address The whole address, of 4 or 16 bytes in length
		 * @param mask The mask, from 1 to 16 bytes in length, anything
		 * from 1 to 128 bits of which is significant
		 * @param mask_Bits How many bits of the mask parameter are significant
		 * for this comparison.
		 * @returns True if the first mask_bits of address matches the first
		 * mask_bits of mask.
		 */
		CoreExport bool MatchCIDRBits(const unsigned char* address, const unsigned char* mask, unsigned int mask_bits);

		/** Match CIDR, without matching username/nickname parts.
		 *
		 * This function will compare a human-readable address against a human-
		 * readable CIDR mask, for example 1.2.3.4 against 1.2.0.0/16. This
		 * method supports both IPV4 and IPV6 addresses.
		 * @param address The human readable address, e.g. 1.2.3.4
		 * @param cidr_mask The human readable mask, e.g. 1.2.0.0/16
		 * @return True if the mask matches the address
		 */
		CoreExport bool MatchCIDR(const std::string &address, const std::string &cidr_mask);

		/** Match CIDR, including an optional username/nickname part.
		 *
		 * This function will compare a human-readable address (plus
		 * optional username and nickname) against a human-readable
		 * CIDR mask, for example joe!bloggs\@1.2.3.4 against
		 * *!bloggs\@1.2.0.0/16. This method supports both IPV4 and
		 * IPV6 addresses.
		 * @param address The human readable address, e.g. fred\@1.2.3.4
		 * @param cidr_mask The human readable mask, e.g. *\@1.2.0.0/16
		 * @return True if the mask matches the address
		 */
		CoreExport bool MatchCIDR(const std::string &address, const std::string &cidr_mask, bool match_with_username);

		/** Create a new valid file descriptor using socket()
		 * @return On return this function will return a value >= 0 for success,
		 * or a negative value upon failure (negative values are invalid file
		 * descriptors)
		 */
		CoreExport int OpenTCPSocket(const char* addr, int socktype = SOCK_STREAM);

		/** Return the size of the structure for syscall passing */
		CoreExport int sa_size(const irc::sockets::sockaddrs& sa);

		/** Convert an address-port pair into a binary sockaddr
		 * @param addr The IP address, IPv4 or IPv6
		 * @param port The port, 0 for unspecified
		 * @param sa The structure to place the result in. Will be zeroed prior to conversion
		 * @return true if the conversion was successful, false if not.
		 */
		CoreExport bool aptosa(const char* addr, int port, irc::sockets::sockaddrs* sa);
		/** Convert a binary sockaddr to an address-port pair
		 * @param sa The structure to convert
		 * @param addr the IP address
		 * @param port the port
		 * @return true if the conversion was successful, false if unknown address family
		 */
		CoreExport bool satoap(const irc::sockets::sockaddrs* sa, std::string& addr, int &port);
		/** Convert a binary sockaddr to a user-readable string.
		 * This means IPv6 addresses are written as [::1]:6667, and *:6668 is used for 0.0.0.0:6668
		 * @param sa The structure to convert
		 * @return The string; "<unknown>" if not a valid address
		 */
		CoreExport std::string satouser(const irc::sockets::sockaddrs* sa);
	}
}



/** This class handles incoming connections on client ports.
 * It will create a new User for every valid connection
 * and assign it a file descriptor.
 */
class CoreExport ListenSocketBase : public EventHandler
{
 protected:
	/** Socket description (shown in stats p) */
	std::string desc;

	/** Raw address socket is bound to */
	std::string bind_addr;
	/** Port socket is bound to */
	int bind_port;
	/** Human-readable address/port socket is bound to */
	std::string bind_desc;

	/** The client address if the most recently connected client.
	 * Should only be used when accepting a new client.
	 */
	static irc::sockets::sockaddrs client;
	/** The server address used by the most recently connected client.
	 * This may differ from the bind address by having a nonzero address,
	 * if the port is wildcard bound, or being IPv4 on a 6to4 IPv6 port.
	 * The address family will always match that of "client"
	 */
	static irc::sockets::sockaddrs server;

 public:
	/** Create a new listening socket
	 */
	ListenSocketBase(int port, const std::string &addr);
	/** Handle an I/O event
	 */
	void HandleEvent(EventType et, int errornum = 0);
	/** Close the socket
	 */
	~ListenSocketBase();
	/** Set descriptive text
	 */
	void SetDescription(const std::string &description)
	{
		desc = description;
	}
	/** Get description for socket
	 */
	const std::string& GetDescription() { return desc; }
	/** Get port number for socket
	 */
	int GetPort() const { return bind_port; }

	/** Get IP address socket is bound to
	 */
	const std::string &GetIP() { return bind_addr; }

	const std::string &GetBindDesc() { return bind_desc; }

	/** Handles sockets internals crap of a connection, convenience wrapper really
	 */
	void AcceptInternal();

	/** Called when a new connection has successfully been accepted on this listener.
	 * @param fd The file descriptor of the new connection
	 */
	virtual void OnAcceptReady(int fd) = 0;
};

class CoreExport ClientListenSocket : public ListenSocketBase
{
	virtual void OnAcceptReady(int fd);
 public:
	ClientListenSocket(int port, const std::string &addr) : ListenSocketBase(port, addr) { }
};

#endif


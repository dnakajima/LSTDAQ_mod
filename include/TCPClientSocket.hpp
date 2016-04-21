



#ifndef __TCP_CLIENT_SOCKET_H
#define __TCP_CLIENT_SOCKET_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
/*! \file structcmd.h
 \brief A Documented file.
 
 Details.
 */

namespace LSTDAQ{
  namespace LIB{
/**
 * The class to take care of TCP/IP connection.
 * 
 * This class requires the knowledge of socket programming to maintain.
 * It obtains all the information for TCP/IP connection as member variables and establishes/disconnects the TCP/IP connection by member functions.
 *
 * @param m_sockTcp : int type which stores socket ID 
 * @param m_addrTcp : sockaddr_in type struct which stores connection information destination
 * @param m_sockNo  : USELESS VALUE. I HAVE FORGOT TO REMOVE IT. WILL BE REMOVED.
 *
 */
class TCPClientSocket
{
public:
  //constructor & destructor
  /**
   * Constructor
   */
  TCPClientSocket         (                        ) throw();
  /**
   * Copy constructor
   */
  TCPClientSocket         ( const TCPClientSocket& ) throw();
  /**
   * Destructor
   */
  virtual ~TCPClientSocket(                        ) throw();
  //operator=
  /**
   * "=" Operator.
   */
  TCPClientSocket &operator = (const TCPClientSocket&) throw();
  
  //setter & getter
  /** 
   * Getter.
   * @return m_sockTcp(
   */
  int getSock() throw();

  //method
  /** 
   * Establishes TCP/IP connection.
   *
   *
   * Procedures:
   *   - Get a socket to receive data in client side (DAQ program) by socket() function and store the socket number in m_sockTcp.
   *   - Set values in m_addrTcp, including the destination address (IP address and port number).
   *   - Establish connection with the server (FEB) by connect() function.
   *   - After connection is established, data which FEB sends arrives at the socket and will be stored 
   *       
   *
   */
  bool connectTcp(const char *pszHost,
                    unsigned short shPort,
                    unsigned long &lConnectedIP);

  /**
   * Reads data from socket.
   * 
   * read() function reads data from receiver socket. 
   *
   *
   *
   */
  ssize_t readSock(void *buffer, size_t nbytes) throw();

  /**
   * Close socket.
   *
   *
   *
   *
   *
   */
  bool closeSock();
  
private:
  int m_sockTcp;
  sockaddr_in m_addrTcp;
  int m_sockNo;
};
    
  }
}

#endif

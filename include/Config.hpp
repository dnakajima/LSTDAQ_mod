//
//! \file  Config.hpp
//  
//
//  Created by Kazuma Ishio on 2014/12/19.
//  Modified by Daisuke Nakajima
//

#ifndef _Config_hpp
#define _Config_hpp

/*--- Size and structure of event data ---*/
/** @def READDEPTH
 * @brief Region of interest
 */
#define READDEPTH 100


/** @def EVENTSIZE
 * @brief Size of one event from one module.

 * (READDEPTH * 2(high/lowgain) + 2 lines of header +flag +stopcell) * 16 byte
 */
#define EVENTSIZE (READDEPTH*2 + 2 +1 +1)*16


/*FakeFEB sends data begins with header 2 bytes and followed by ipaddress 4 bytes */
#define HEADERLEN 2   //for FakeFEB only
#define IPADDRLEN 4   //for FakeFEB only
/*Data from Both has evNo,trigNo, and clk(FakeFEB doesn't) in last 16 bytes*/


/** @def AAAALEN
 * @brief Length of the Header AAAA 2 byte
 */
#define AAAALEN  2

/** @def PPSLEN
 * @brief Length of the PPS counter 2 byte
 */
#define PPSLEN  2

/** @def TENMLEN
 * @brief Length of the 10 MHz counter 4 byte
 */
#define TENMLEN  4


/** @def EVTNOLEN
 * @brief Length of the EventCounter 4 byte
 */
#define EVTNOLEN  4


/** @def TRGNOLEN
 * @brief Length of the TriggerCounter 4 byte
 */
#define TRGNOLEN  4


/** @def CLKLEN
 * @brief Length of the local 133MHz counter 8 byte
 */
#define CLKLEN    8

/** @def DDDDLEN
 * @brief Length of the Footer of the header DDDD_DDDD_DDDD_DDDD 8 byte
 */
#define DDDDLEN  8



/** @def POSEVTNO
 * @brief Position of EventCounter 

  AAAALEN+PPSLEN+TENMLEN = (2+2+4) = 8
 */
#define POSEVTNO  AAAALEN+PPSLEN+TENMLEN



/** @def POSTRGNO
 * @brief Position of TriggerCounter 

POSEVTNO + EVTNOLEN  = (8 + 4) = 12
 */
#define POSTRGNO  POSEVTNO+EVTNOLEN

/** @def POSTRGNO
 * @brief Position of TriggerCounter 

POSTRGNO+TRGNOLEN = (12+4) = 16
 */
#define POSCLK    POSTRGNO+TRGNOLEN

/** @def MAX_CONNECTION
 * @brief max number of connections
 */
#define MAX_CONNECTION 48

/** @def MAX_RINGBUF
 * @brief max number of ring buffer
 */
#define MAX_RINGBUF 49

//error exit threshold 
#define ERR_NDROPPED 100000

//interval to measure throughput
#define THRUPUTMES_STARTSEC 1
#define THRUPUTMES_INTERVALSEC 0
#define THRUPUTMES_INTERVALNSEC 1000000 
//1000000 for 1msec
#define MAX_NWMES 10000

//outputfile by DAQtimer
#define MESFILE "LSTDAQmeasure.dat"
#define ERRMESFILE "LSTDAQerrmeasure.dat"

#define LOGPATH "./log"

#endif

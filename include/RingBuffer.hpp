#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

// #ifndef EVENTSIZE 
//   #define EVENTSIZE 
// #endif

#define RINGBUFSIZE 50000
#define TIMETOWAIT  0
#define TIMETOWAIT_USEC  1000
#include <pthread.h>
#include "Config.hpp"
namespace LSTDAQ{

  /** 
   * The class to organize ringbuffer.
   *
   * Ringbuffer is a buffer to temporally store data which arrive at socket through TCP/IP.  
   *
   * @param -Name-   ---Type : Description---

   
   * @param *****For_access_controll*****
     @param m_mutex  pthread_mutex_t  : ticket to access RingBuffer object
     @param m_cond   pthread_cond_t   : to controll pthread_mutex_t so that a thread who has ticket can wait for and allow another thread access RingBuffer object.
     @param m_tsWait  struct timespec : time to wait (on write function)

   * @param *****Buffer*****
     @param m_buffer[EVENTSIZE*RINGBUFSIZE]        unsigned char : buffer memory. RINGBUFSIZE and EVENTSIZE are defined in RingBuffer.hpp and Config.hpp respectively.
     @param RINGBUFSIZE  #define : size of the buffer[events].
     @param EVENTSIZE    #define : size of one event[bytes]. More precisely, the length of data which comes from a FEB kicked by a trigger.

     @param m_Nm           const unsigned int : NOT USED. TO BE REMOVED. (confession: this was used to check initialization by constructor in developing this program)




   * @param *****For_position_controll*****
     @param m_bufSizeByte     unsigned int : buffer size[byte] to be used as reference to the position in RingBuffer. It is initialized as m_bufSizeByte = RINGBUFSIZE*EVENTSIZE.
     @param m_Nmw          unsigned int :position[events] to write on the memory 
     @param m_Nmr   	   unsigned int :position[events] to read from  the memory
     @param m_woffset 	   unsigned int :pointer position[bytes] to write on the memory
     @param m_roffset 	   unsigned int :pointer position[bytes] to read from the memory
     @param m_remain 	   unsigned int :
     @param m_wbytes       unsigned int :The data size of current event, which have been written to the memory. 0 =< m_wbytes < EVENTSIZE.

     @param *****Total_history*****		                 
     @param m_Nw    unsigned long  : Total number of events written to the memory 
     @param m_Nr    unsigned long  : Total number of events read from  the memory	   

   *
   */
  class RingBuffer
  {
  public:
    /**
     * Constructor
     */
    RingBuffer() throw();
    /**
     * Destructor
     */
    virtual ~RingBuffer() throw();
    /**
     * "=" operator
     */
    RingBuffer &operator = (const RingBuffer &)throw();

    
    //methods
    /**
     *
     */
    bool open();
    /**
     *
     */
    bool init();
    
    //******************************
    //*  write() method
    //******************************
    /**
     * The function to write data on RingBuffer in which wbytes[bytes] of data from *buf will be written.
     *
     * Data with any length up to EVENTSIZE can be written if there is enough empty space.
     * "empty" space means the buffer memory which is not written yet or already read by read() function. 
     * Data doesn't need to be exact within one event. Data which contains parts of two events can be treated too.
     *
     *
     * Procedure:
     *   - Lock mutex so that no other threads can access RingBuffer. 
     *   - Makes sure there is enough space to write \n
     *     m_Nw should not be larger than m_Nr + RINGBUFSIZE, otherwise the unread data in buffer is overwritten.
     *     If the condition will be violated by writing data, this function stops and waits for one more data to be read.
     *
     *   - Writes data in m_buffer buffer memory\n
     *     The data buf with wbytes of lengh is written to m_buffer from m_woffset position. After writing, m_woffset and m_wbytes will be increased by wbytes.
         　If the data to write doesn't include the end of a event (m_wbytes + wbytes< EVENTSIZE), data will just be written. Otherwise, data will be written in following way.\n
     *     m_Nw and m_Nmw will be counted up as well as data is written in buffer and m_woffset will be shifted by wbytes.m_wbytes also be shifted so that it indicates to which position in the dataformat has been written. If the position in the memory to write data includes the end of buffer memory, the process includes reset of the offset position. This means m_Nmw=0 and m_woffset = m_remain. m_remain is the residure to write after writing data until the end position of buffer.
     *  
     *   - Unlock mutex to access. 
     *
     *
     *
     */
    int write( char *buf,unsigned int wbytes);

    //******************************
    //*  read() method
    //******************************
    /**
     * The function to read data from RingBuffer.
     *
     * Whole size of one event data is read from m_buffer. If there is no new data in m_buffer, the function ends without reading.
     * 
     * Procedure : 
     * - If there is no new unread data, the function ends.
     * - Lock mutex 
     * - read one event starts from m_roffset in m_buffer. 
     * - increment values (m_Nr, m_Nmr and m_roffset).
     * - If offset in m_roffset reaches at the end of m_buffer, resets m_Nmr and m_roffset.
     * 
     */
    int read(char *buf);

    //getter methods
    unsigned long getNw() throw();
    unsigned long getNr() throw();
    
  private:
    pthread_mutex_t *m_mutex;
    pthread_cond_t *m_cond;
    
    //time to wait (on write function)
    struct timespec m_tsWait;
    
    //buffer
    const unsigned int m_Nm;
    unsigned char m_buffer[EVENTSIZE*RINGBUFSIZE];
    unsigned int m_bufSizeByte;
    unsigned int m_woffset;
    unsigned int m_roffset;
    unsigned int m_remain;
    unsigned int m_wbytes;//written to the memory
    //total history
    unsigned long m_Nw;  //events written to the memory
    unsigned long m_Nr;  //read from  the memory
    //the position on memory
    unsigned int  m_Nmw;  //written to the memory
    unsigned int  m_Nmr;  //read from  the memory
  };
}


#endif

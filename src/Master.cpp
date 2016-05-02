
/*! \def DAQ_NEVENT
 @brief The number of events to acquire when NOT specified by user. */
/*!
 *  Ndaq is set to this value as default value.
 *  And if specified -
 */
#define DAQ_NEVENT  100000


/**
   \mainpage LSTDAQ ver3.0
   \author Kazuma Ishio,Daisuke Nakajima Univ. of Tokyo
   \date  Last modified on 2016/05/02
   
   *********************************************************************
   \section INTRO Introduction
   *********************************************************************
   %LSTDAQ is a program to perform data acquisition from LST camera.\n
   The camera in a telescope consists of 265 modules called FEB.
   %LSTDAQ acquires data from them through TCP/IP connection.\n
   At this moment minimum features are implemented.
 
   ************************************************
   \subsection FUNCTIONALITY_OVERVIEW Functionality Overview
   ************************************************
   
   - TCP/IP connection to FEBs (configurable without compilation)
   - Multi threaded data acquisition through TCP/IP (configurable without compilation)
   - Event building identifying trigger number
   - data pooling in ring buffers, to absorb inequality of sending data between FEBs.
   
   
   ************************************************
   \subsection PROC_OVERVIEW Procedure Overview
   ************************************************
   - main() function
       - Sets parameters from args and sysconf.
       - Reads configuration file.
       - Creates structs for connetions.
       - Creates Builder thread.
       - Creates Collector threads.
       - Creates Monitor thread.
       - Waits for the end of the threads.
  
   - Collector_thread() function
       - Casts the input
       - Sets CPU.
       - Searches the charged sRingBuffer structs
       - Establishes TCP/IP connections with FEBs.
       - Synchronizes threads before starting DAQ.
       - Reads data from socket\n
        and writes it to RingBuffer.
       - Goes out the loop.
       - Thread ends.

   - Builder_thread() function
       - Receives the first struct for connection.
       - Searches all the structs for connections
       - Sets CPU.
       - Creates output file.
       - Waits for start synchronization.
       - Reads data written in Ring Buffers by Collector threads, 
           combining the data with identical trigger number embedded in the data.
       - Goes out the loop when read amount of one of the connections exceeded the requested amount.
       - Thread ends.
       
   
   *********************************************************************
   \section PROCEDURE_CLOSEVIEW Common procedure closeview
   *********************************************************************
   In this section, the explanation for the procedures which are commonly used are provided.
 

   

   ************************************************
   \subsection CPU_ID CPU ID specification
   ************************************************
   To adopt parallel computing, assigned are CPU IDs to the threads, which are divided in a unit of procedue which can run in parallel.

   \subsubsection CPU_ID_OVERVIEW Overview   
   main function creates multi threads. The threads consist of one ThruPutMes_thread, one Builder_thread and several Collector_thread.
   Collector_thread can run as multiple threads and the configuration of multiplicity is established by reading Connection.conf. The number of threads can be increased without limit. But the number of CPUs is limited, therefore too many threads cause assignment of the CPUs shared by multiple threads. Unless the number of threads does not exceed the number of CPUs, assignment of threads to CPUs is done avoiding biased assignment.
   
  \subsubsection CPU_ID_PROC Procedues
  There are `nColl + 2` threads to be assigned, which consist of one `Builder_thread`, one `ThruPutMes_thread`, and  `nColl` threads of `Collector_thread`. 
  Using residual number, CPUs are assigned to the threads evenly. Here is the relation between thread and `CPU_ID`.

  Thread name     | Number of threads |  CPU ID  
 -----------------|-------------------|--------------
 Collector_thread | `nColl`           |  `Cid` \% `Ncpu` (where `Cid` = 0,1,2...`nColl`-1)
 Builder_thread   | 1                 |  `nColl` \% `Ncpu`
 ThruPutMes_thread| 1                 |  `nColl`+1 \% `Ncpu`


  The number of CPUs is inspected using `sysconf()` function in `main()` function, and stored as `Ncpu` value. 
  When a thread starts, which CPU to use is set, specifying `CPU_ID` in `__CPU_SET()` function.


   ************************************************
   \subsection START_SYNC Start synchronization 
   ************************************************
   When DAQ starts, all the Collector_thread and Builder_thread synchronize, not to waste Ring Buffer memory in a way that Collector_thread starts filling data in Ring Buffer before Builder_thread gets prepared to extract it.
   

   \subsubsection START_SYNC_DETAILS Procedures
   - When Builder_thread gets prepared, it waits all the other threads untill they are also prepared, checking if `initEnd` become nColl (\ref BLD_START_SYNC process).

   - On the other hand, Collector_threads undergo the preparation process too. In a Collector_thread, when it has established connection with all the charged FEBs, it increases `initEnd` value by 1, stops itself, and waits for Builder_thread to call it. 

   - When all Collector_threads get prepared (`initEnd` = `nColl`), Builder_thread broadcasts the allowance of starting DAQ to all Collector_threads. 

   - Builder_thread starts reading data from RingBuffers immediately after the announcement, while a Collector_thread starts reading data from socket and writing data to RingBuffer after it notices the announcement.

   \subsubsection START_SYNC_PTHREAD Implementation
   This functionality is implemented using `mutex_initLock` as a `pthread_mutex_t` and `cond_allend` as a `pthread_cond_t`. 

   - Collector_thread\n
   When a Collector_thread gets prepared for DAQ, it locks `mutex_initLock` ticket by `pthread_mutex_lock()` and immediately it releases it by `pthread_cond_wait()`. But this function makes it stopped until `pthread_cond_broadcast()` is issued to `cond_allend`. After receiving the signal, it again tries to lock `mutex_initLock` which enables it to leave `pthread_cond_wait()`  function. As the last step, now having the `mutex_initLock` ticket, it unlocks it by `pthread_mutex_unlock()` so that the other Collector_threads can do the same step.

   - Builder_thread\n
   When `initEnd` = `nColl`, Builder_thread announces the allowance of starting DAQ as follows.
   First, it locks `mutex_initLock` which is already free from any other threads, with pthread_mutex_lock() method. Second, it sends signal to all the threads which have issued `pthread_cond_wait()` function with `cond_allend` variable. This procedure means that it announces the start of DAQ to all the Collector_threads. At last, it unlocks `mutex_initLock` with `pthread_mutex_unlock()` function for the Collector_threads to be able to leave `pthread_cond_wait()` function.

   - ThruPutMes_thread\n
   When preparation for the measurement is completed, it does the same steps as Builder_thread. But the steps neither do nor suffer from any harm. Here is the inspection of two possible case:
       - Earlier than Builder_thread\n
       If ThruPutMes_thread precedes Builder_thread, ThruPutMes_thread serves the `cond_allend` signal to Collector_threads. Builder_thread can also take `mutex_initLock` ticket right after some other thread releases it. This doesn't take much time because all the threads releases it (by `pthread_mutex_unlock()`) immediately after it locks (as awaking `pthread_cond_wait()`). When Builder_thread takes `mutex_initLock` and sends signal by `pthread_cond_broadcast()`, nothing happens, because the signal was already sent and all the threads have gone to next step. The moment Builder_thread releases `mutex_initLock` as the next step and starts reading RingBuffers is not so much later than 




   - A thread which will be added in the future
      - Monitor thread 



 */



//
//  Output Directory is set to Raid System on this computer.
//  The directory needs "su" when submit this to write data on disk.
//
//   ThruputMes is modified to output Nr too.
//
//  \date Based on these versions
//  \date 2014/12/27_1 MergeTest (Event Merge with throwing events with skipped Ntrig)
//  \date 2014/12/26_3 For RingBuf Estimation
//  \date 2014/12/25_2 LongTermTest without merge
//  \date 2014/12/23 without merge version 2

/******************************/
/** \file Master.cpp
 * This file contains main function.
 *
 *   main function reads configuration file and creates the threads.
 *  \param Ndaq will be aaa
 */
/********************************/

/******************************/
// added:
//  THRUPUTMES_INTERVAL in Config.hpp
//  ThruPutMes_thread in Master.cpp
//  getNw in RingBuffer.cpp, RingBuffer.hpp
//  evtNo,trgNo SkipCounter is implemented.
//    result is output as ERRMESFILE(see Config.hpp)
//
// modified:
//  cpuid
//NOTE:
//  how many times to monitor is specified in Config.h
//  
/******************************/



/* basic */
#include <iostream> //cout
#include <string.h> //strcpy
//#include <string>
#include <stdio.h>  //sprintf
#include <unistd.h> //for sleep()andusleep()
#include <fstream>  //filestream(file I/O)
#include <sstream>  //stringstream
#include <iomanip>  //padding cout
#include <stdlib.h> //exit(1)

/* for DAQ functionality  */
#include <pthread.h>
#include "RingBuffer.hpp"
#include "TCPClientSocket.hpp"
#include "DAQtimer.hpp"
#include "Config.hpp"

/* for measurement  */
#include <time.h>//itimespec
#include <sys/timerfd.h>//timer in ThruPutMes_thread
#include <assert.h>

#include "termcolor.h"
#include <getopt.h>

struct option options[] =
  {
    {"help"     ,no_argument       ,NULL ,'h'},
    {"infreq"   ,required_argument ,NULL ,'i'},
    {"ndata"    ,required_argument ,NULL ,'n'},
    {"output"   ,required_argument ,NULL ,'o'},
    {"readdepth",required_argument ,NULL ,'r'},
    {"save"     ,no_argument       ,NULL ,'s'},
    {"version"  ,required_argument ,NULL ,'v'},
    {"closeinspect" ,no_argument   ,NULL ,'c'},
    {"configfile" ,required_argument   ,NULL ,'f'},
    {0,0,0,0}
  };


int infreq;
unsigned long  Ndaq;
bool datacreate;
std::string fileNameHeader;

//variables for start synchronizer
pthread_mutex_t mutex_initLock  =PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_allend      =PTHREAD_COND_INITIALIZER;
int initEnd;
//variable of the number of CPU in the system on which this runs
int Ncpu;

using namespace std;


/****************************/
// inverse Byte Order
/****************************/
//! \brief Flips the byte order of given array
/*!
 *  
 * The order of each value inside data sent from Dragon is inversed.<br>
 * For event building, trigNo and evtNo are needed.<br>
 * This function is used to flip byte orders of them after extraction of data from socket.<br>
 */
void inverseByteOrder(char *buf,int bufsize)
{
  // if(bufsize>32)exit 1;
  unsigned char tempbuf[32];
  for(int i=0; i<bufsize; i++)
  {
    tempbuf[i]=buf[bufsize-i-1];
  }
  memcpy(buf,tempbuf,bufsize);
}

/****************************/
// print usage
/****************************/
void usage(char *argv)
{
  printf("usage: %s <infreq[Hz]> <Ndaq[events]> <datacreate> \n",argv);
  printf("<Ndaq>       -- optional.default value is %d\n",DAQ_NEVENT);
  printf("<datacreate> -- 1:create data. 0 or no specification: don't create data.\n"); 
  exit(0);
}

/****************************/
// struct definition
/****************************/
/**
\param sRBid 
RingBufferID.

\param Cid
Collector ID. This is used to specify which collector thread controls the %sRingBuffer object. 
\param szAddr
Destination IP address of connection to FEB.

\param shPort
Destination Port number of connection to FEB.

\section SRB_OVERVIEW Overview
 This struct stores all information about an connection to FEB. 
 They are created in main() function, based on Connection.conf, and used in Collector_thread() and Builder_thread(). 

 sRingBuffer has one-to-one correspondence to RingBuffer object and connection to FEB. Therefore one sRingBuffer struct represents a connection to a FEB, and as many sRingBuffer structs as the connections to FEB are prepared. And it has one RingBuffer object, which store all the data from a FEB temporally in a unit of event length. The purpose of this is to absorb timing inequality between connections, because the data-flows from FEBs are not physically parallel. 

A sRingBuffer struct belongs to a Collector_thread, while a Collector_thread can take care of multiple sRBs. The relation between them is specified in Connection.conf by users and set in main() function. 

A Collector_thread performs data acquisition from FEBs, reading data from sockets arrived by TCP connection and writing it on RingBuffers.

The data stored in RingBuffer is extracted one by one by Builder_thread.
 
 
\section SRB_PROC Procedues
A sRingBuffer struct is used as follows.
 
\subsection SRB_CREATION main() --- Object creation
 In main() function, sRBs are created and the primal information to control them are set with 
 sRBinit(), sRBcreate() and sRBsetaddr().
 When threads are created, the addresses of these sRBs are given to the Collector_threads and the Builder_thread.

\subsection SRB_COLL Collector_thread() --- Data acquisition
To identify Collector_thread, Cid is set from the sRingBuffer struct given to the Collector_thread. Then the thread decides which sRingBuffer to take care of, searching for the same Cid number.
TCP/IP connections to FEBs for sRingBuffer structs are also established, using the connection information stored in sRingBuffer struct. 

In endless loop, data is extracted from socket and written on RingBuffer object in sRingBuffer struct.

\subsection SRB_BLD Builder_thread() --- Event building
Builder_thread() performs event building in which the data from all FEBs is combined together to make a image of whole camera. Therefore it collects addresses of all sRingBuffer structs in the beginning. Then using read() function in RingBuffer object, it collects data.



 */
struct sRingBuffer{
  int sRBid;            //!< RingBufferID
  int Cid;              //!< Collector ID
  char szAddr[18];      //!< IP address of target FEB.
                        //!< for example, "192.168.10.1".
                        //!< The value in Connection.conf will be read and set.
  unsigned short shPort;//!< port number of target FEB.
                        //!< for example, 24.
                        //!< The value in Connection.conf will be read and set.
  LSTDAQ::RingBuffer* rb ;
  sRingBuffer* next;
};
sRingBuffer sRB[MAX_RINGBUF];

//void sRBinit();
//void sRBcreate(int nServ);
//void sRBsetaddr(int sRBid, char *szAddr, unsigned short shPort);
void sRBinit()
{
  for(int i=0; i<MAX_RINGBUF; i++)
  {
    sRB[i].Cid = -1;
    sRB[i].sRBid = -1;
    sRB[i].next = 0;
    // cout <<sRB[i].next<<endl;
  }
}

void sRBcreate(int nServ)
{
  for(int i=0; i<nServ; i++)
  {
    sRB[i].sRBid = i;
    sRB[i].rb = new LSTDAQ::RingBuffer();
    sRB[i].next=&sRB[i+1];
    // cout <<sRB[i].next<<endl;
  }
}

void sRBsetaddr(int sRBid, unsigned short shCid, char *szAddr, unsigned short shPort)
{
  if(sRB[sRBid].sRBid==sRBid)
  {
    sRB[sRBid].Cid=(int)shCid;
    strcpy(sRB[sRBid].szAddr, szAddr);
    sRB[sRBid].shPort=shPort;
  }
}

int getMaxCid()
{
  int maxCid=0;
  for(int i=0;i<MAX_RINGBUF;i++)
  {
    if(maxCid<sRB[i].Cid)maxCid=sRB[i].Cid;
  }
  return maxCid;
  
}

void sRBdelete(int nServ)
{
  for(int i=0; i<nServ; i++)
  {
    delete sRB[i].rb;
  }
}

/********************************/
//! ThruPutMes thread
/********************************/
void *ThruPutMes_thread(void *arg)
{
  //basic preparation
  sRingBuffer *srb[MAX_CONNECTION];
  srb[0]= (sRingBuffer*)arg;
//  double readfreqC[MAX_CONNECTION];//Collector
//  double readrateC[MAX_CONNECTION];//Collector
//  double readfreqB[MAX_CONNECTION];//Builder
//  double readrateB[MAX_CONNECTION];//Builder
  
//  unsigned long Nw[MAX_CONNECTION]={0};
  unsigned long Nr[MAX_NWMES][MAX_CONNECTION]={0};
//  unsigned long prevNw[MAX_CONNECTION]={0};
//  unsigned long prevNr[MAX_CONNECTION]={0};
  unsigned long Nw[MAX_NWMES][MAX_CONNECTION]={0};

  
  
  //cout<<"*** ThruPutMes_thread initialization ***"<<endl;
  int nRB =0;
  while(1)
  {
    // cout<<"srb["<<nRB<<"] :"<<srb[nRB]->next<<endl;
    if(srb[nRB]->next==0||nRB==MAX_RINGBUF)break;
    srb[nRB+1]=srb[nRB]->next;
    nRB++;
  }
  
  int nColl = 1+ getMaxCid();
  
  //*********** CPU Specification    *************//
  int cpuid;
  cpuid = (nColl+1)%Ncpu;
#ifdef __CPU_ZERO
  cpu_set_t mask;
  __CPU_ZERO(&mask);
  __CPU_SET(Ncpu-1,&mask);
  if(sched_setaffinity(0,sizeof(mask), &mask) == -1)
    printf("WARNING: failed to set CPU affinity... (cpuid=%d)\n",cpuid);
#endif
  
  //*********** Output File Creation *************//
 char buf[128];
// sprintf(buf,"ThruPutMes_infreq%d_%dto%02d.dat"
//         ,infreq
//         ,nColl
//         ,nRB);
// FILE *fp_ms;
// fp_ms = fopen(buf,"w");
// fprintf(fp_ms,"InFreq=%d\n",infreq);
// fprintf(fp_ms,"readrate by Coll[Mbps], readrate by Bld[Mbps]\n");
  sprintf(buf,"RingBufMes_infreq%d_%dto%02d.dat"
          ,infreq
          ,nColl
          ,nRB);
  FILE *fp_ms;
  fp_ms = fopen(buf,"w");
  fprintf(fp_ms,"InFreq=%d\n",infreq);
  fprintf(fp_ms,"Nw in RingBuffer　　　　　　Nr in RingBuffer\n");
  
 fprintf(fp_ms, "count");
 for(int i =0;i<nRB;i++)
 {
   fprintf(fp_ms, "   RB%02d   ",i);
 }
  for(int i =0;i<nRB;i++)
  {
    fprintf(fp_ms, "   RB%02d   ",i);
  }
 // for(int i =0;i<nRB;i++)
 // {
 //   fprintf(fp_ms, "  RB%02d_B[Mbps]  ",i);
 // }
 fprintf(fp_ms,"\n");

  //*********** Timer Initialization *************//
  int interval;
  struct itimerspec its;
  int timerfd;
  int ret;
  // interval = THRUPUTMES_INTERVAL;
  
  //time to begin;
  its.it_value.tv_sec = THRUPUTMES_STARTSEC;
  its.it_value.tv_nsec = 0;
  //interval
  its.it_interval.tv_sec = THRUPUTMES_INTERVALSEC;
  its.it_interval.tv_nsec = THRUPUTMES_INTERVALNSEC;
  

  timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
  ret = timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &its, NULL);

  //*********** Start Synchronization *************//
  while(initEnd<nColl);
  // cout<<"Bld Confirmed all"<<endl;
  cout<<"*** ThruPutMes_thread initend ***"<<endl;
  pthread_mutex_lock(&mutex_initLock);
  pthread_cond_broadcast(&cond_allend);
  pthread_mutex_unlock(&mutex_initLock);

  
  //*********** RingBuffer Measuremet *************//
//  fprintf(stderr, "  Throughput in Mbps   \n");
 // for(int i =0;i<nRB;i++)
 // {
 //   fprintf(stderr, "   RB%02d    ",i);
 // }
 //  fprintf(stderr, "\n");

  unsigned long Nread=0;
    int ReadEnd=0;
    bool bReadEnd[MAX_CONNECTION]={false};
  while (1)
  {
    uint64_t v;
    ret = read(timerfd, &v, sizeof(v));
    assert(ret == sizeof(v));
    //    if (v > 1) {
    //      fputc('o', stderr);
    //    } else if (!quite) {
    //      fputc('.', stderr);
    //    }
    
    for(int i =0;i<nRB;i++)
    {
      Nw[Nread][i]=srb[i]->rb->getNw();
      Nr[Nread][i]=srb[i]->rb->getNr();
    }
    for(int i =0;i<nRB;i++)
    {
//      readfreqC[i]=(double)(Nw[i]-prevNw[i])/(double)interval;
//      readrateC[i]=readfreqC[i]*(double)EVENTSIZE*8./1000./1000.;
//      prevNw[i]=Nw[i];
//      readfreqB[i]=(double)(Nr[i]-prevNr[i])/(double)interval;
//      readrateB[i]=readfreqB[i]*(double)EVENTSIZE*8./1000./1000.;
//      prevNr[i]=Nr[i];
      
      if(Nr[Nread][i]>=Ndaq && !bReadEnd[i])
      {
        ReadEnd++;
        bReadEnd[i]=true;
      }
    }
//    for(int i =0;i<nRB;i++)
//    {
//      fprintf(stderr, "%8.3f  ",readrateC[i]);
//    }
//    fprintf(stderr, "\n");

//    for(int i =0;i<nRB;i++)
//    {
//      fprintf(fp_ms,"%8.3f  ",readrateC[i]);
//    }
    // for(int i =0;i<nRB;i++)
    // {
    //   fprintf(fp_ms,"%8.3f  ",readrateB[i]);
    // }
//    fprintf(fp_ms, "\n");
    Nread++;
    if (Nread==MAX_NWMES)break;
    // if (ReadEnd==nRB)break;
    if (ReadEnd>0)break;
  }

  
  for(int i=0; i<Nread;i++)
  {
    fprintf(fp_ms,"%5d ",i);
    for(int j=0; j<nRB; j++)
    {
      fprintf(fp_ms,"%9d ",Nw[i][j]);
    }
    for(int j=0; j<nRB; j++)
    {
      fprintf(fp_ms,"%9d ",Nr[i][j]);
    }
    
    fprintf(fp_ms, "\n");
  }
  fclose(fp_ms);
  
}




/********************************/
// Collector thread
/********************************/
/**
   \func Collector_thread
   Collects data from FEBs by TCP/IP connection and stores it to RingBuffer.
   
   *********************************************************************
   \section COLL_PROC Procedures
   *********************************************************************
    -# Sets the first charged sRingBuffer .
    -# Sets CPU.
    -# Searches the charged sRingBuffer structs
    -# Establishes TCP/IP connections with FEBs.
    -# Synchronizes with all threads before starting DAQ.
    -# Reads data from socket\n
        and writes it to RingBuffer.
    -# Goes out the loop.
    -# Thread ends.
  ************************************************
  \subsection COLL_SETFIRST Sets the first charged sRingBuffer .
  ************************************************
    This function receives reference to an sRingBuffer struct as void* type, 
    and casts it as "sRingBuffer" struct. \n
    From Cid in it, this thread knows its Collector ID.
    This struct is the first one among which this thread is charged for. 
    All the struct to take charge is to be searched in \ref COLL_SEARCHRB .

  ************************************************
  \subsection COLL_SETCPU Sets CPU
  ************************************************
    Sets CPU. See \ref CPU_ID .

  ************************************************
  \subsection COLL_SEARCHRB Search the charged sRingBuffer structs
  ************************************************
    Searches all the structs for connections for which this thread is assigned.
    
  ************************************************
  \subsection COLL_TCPCON Establishes TCP/IP connections with FEBs.
  ************************************************
    Establishes TCP/IP connections with FEBs. LSTDAQ::LIB::TCPClientSocket() takes main role.

  ************************************************
  \subsection COLL_STARTSYNC Synchronizes threads before starting DAQ.
  ************************************************
    Waits for the other threads to be prepared as well. See \ref START_SYNC.

  ************************************************
  \subsection COLL_READSOCK Reads data from socket and writes it to RingBuffer.
  ************************************************
    Reads data arrived at the sockets and writes on Ring Buffers (in the structs).
    Reading from sockets is performed by LSTDAQ::LIB::TCPClientSocket() and writing to Ring Buffers is performed by 
    LSTDAQ::RingBuffer::write().

  ************************************************
  \subsection COLL_OUTLOOP Goes out the loop.
  ************************************************
    Goes out the loop when read amount of all the connections exceeded the requested amount.

  ************************************************
  \subsection COLL_THREADEND Thread ends.
  ************************************************
   
   

 */


void *Collector_thread(void *arg)
{
  
  //receive buffer(From socket to ringbuffer)
  char tempbuf[EVENTSIZE];
  
  // cout<<"*** Collector_thread initialization ***"<<endl;
  /******************************************/
  //  First RB and Collector ID
  /******************************************/
  //first RingBuffer
  sRingBuffer *srb[MAX_CONNECTION];
  srb[0]= (sRingBuffer*)arg;
  //CollectorID
  int Cid=srb[0]->Cid;
  
  
  /******************************************/
  //  CPU Specification
  /******************************************/
  int cpuid;
  cpuid = Cid%Ncpu;
#ifdef __CPU_ZERO
  cpu_set_t mask;
  __CPU_ZERO(&mask);
  __CPU_SET(cpuid,&mask);
  if(sched_setaffinity(0,sizeof(mask), &mask) == -1)
    printf("WARNING: failed to set CPU affinity... (cpuid=%d)\n",cpuid);
#endif
  
  
  /******************************************/
  //  Search RBs
  /******************************************/
  //next RingBuffers
  // cout<<"next RBs for Coll "<<Cid<<endl;
  sRingBuffer *srb_temp=srb[0]->next;
  int nServ=1;
  while(1)
  {
    if(srb_temp->sRBid==-1)break;
    // cout<<"RB"<<srb_temp->sRBid<<": Cid is "<<srb_temp->Cid;
    if(srb_temp->Cid == Cid)
    {
      srb[nServ]=srb_temp;
      // cout<<" matched."<<endl;
      nServ++;
    }
    // else
    // {
    //   cout<<endl;
    // }
    srb_temp=srb_temp->next;
    //cout<<"srb["<<nServ<<"]->next :"<<srb_temp->next<<endl;
  }
  // cout<< "*** RingBufferList owned by Collector"<<srb[0]->Cid<<endl;
  // for(int i=0;i<nServ;i++)
  // {
  //   cout<<"RB"<<srb[i]->sRBid<<endl;
  // }
  // cout<<" :nServ is "<< nServ<<endl;
  
  /******************************************/
  // Connection Initialization
  /******************************************/
  cout<<"*** connection initialization ***"<<endl;
  unsigned long lConnected = 0;
  LSTDAQ::LIB::TCPClientSocket *tcps[MAX_CONNECTION];
  int sock[MAX_CONNECTION];
  for(int i=0;i<nServ;i++)
  {
    tcps[i] = new LSTDAQ::LIB::TCPClientSocket();
    if((tcps[i]->connectTcp(srb[i]->szAddr,
                            srb[i]->shPort,
                            lConnected)
        )<0)
    {
      exit(1);
    }
    sock[i] = tcps[i]->getSock();
  }
  int maxfd=sock[0];
  fd_set fds, readfds;
  FD_ZERO(&readfds);
  for(int i=0;i<nServ;i++)FD_SET(sock[i], &readfds);
  for(int i=1;i<nServ;i++)
  {
    if(sock[i]>maxfd)maxfd=sock[i];
  }
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 10000;
  
  /******************************************/
  //  Start Synchronization
  /******************************************/
  // cout<<"*** CollInit end ***"<<endl;
  initEnd++;
  pthread_mutex_lock(&mutex_initLock);
  pthread_cond_wait(&cond_allend,&mutex_initLock);
  pthread_mutex_unlock(&mutex_initLock);
  
  /******************************************/
  //  Read From sock
  //         and Write on RB
  /******************************************/
  unsigned long long daqsize=Ndaq * EVENTSIZE;
  cout<<"*** Collector_thread starts to read ***"<<endl;
  // cout<<daqsize<<" will be read"<<endl;
  unsigned long long llReadBytes[MAX_CONNECTION]={0};
  int nRdBytes;
  bool bReadEnd[MAX_CONNECTION]={false};
  int ReadEnd=0;
  size_t reqsize;
  for(;;)
  {
    memcpy(&fds,&readfds,sizeof(fd_set));
    select(maxfd+1, &fds, NULL, NULL,&tv);
    for(int i=0;i<nServ;i++)
    {
      if( FD_ISSET(sock[i], &fds) )
      {
	nRdBytes=0;
	reqsize=EVENTSIZE;
	while(1)
	{
	  nRdBytes+=(tcps[i]->readSock(&tempbuf[nRdBytes],reqsize));
	  if(nRdBytes==EVENTSIZE)
	  {
	    // cout<<"Y"<<nRdBytes<<endl;
	    break;
	  }
	  else if(nRdBytes<EVENTSIZE)
	  {
	    reqsize=EVENTSIZE-nRdBytes;
	  }
	  else
	  {
	    exit(1);
	  }
	  // cout<<"N"<<nRdBytes<<endl;
	}
        //if(nRdBytes != EVENTSIZE) is usual
        // cout<<"RB"<<srb[i]->sRBid<<"read"<<nRdBytes<<endl;
        // cout<<i<<"  "<<nRdBytes<<"  "<<llReadBytes[i]<<endl;
        // // cout<<tempbuf<<endl;
        
        if((srb[i]->rb->write(tempbuf,nRdBytes))==-1)
        {
          cout<<"RB"<<srb[i]->sRBid<<":W wait exceeded"<<endl;
	  //exit(1);
        }
	else
	{
	  llReadBytes[i]+=nRdBytes;
	}
        //cout<<"connection"<<i<<bReadEnd[i]<<endl;
        if(llReadBytes[i]>=daqsize && !bReadEnd[i])
        {
          ReadEnd++;
          bReadEnd[i]=true;
          // cout<<"RB"<<i<<"read end"<<endl;
        }
      }
      //usleep(1000000);
      //cout << "Coll"<<srb[j]->sRBid <<" wrote :"<< tempbuf <<endl;
    }
    if (ReadEnd==nServ)break;
  }
  //sleep(3);
  cout << "Coll"<<srb[0]->Cid <<" thread end"<<endl;
}

/********************************/
// Builder thread
/********************************/
/** 
 \func Builder_thread 
 Performs event building in which data is collected from all RingBuffers by event wise and is combined.
 
 *********************************************************************
 \section BLD_PROC The procedures
 *********************************************************************
  -# Make lists of Ring Buffers.
  -# sets CPU.
  -# Output File Creation
  -# Synchronizes with all threads before starting DAQ.
  -# Read Data From RingBuffer with event building
  -# Goes out the loop.
  -# Writes summary report of DAQ.
  -# Thread ends.
 
 ************************************************
 \subsection BLD_RBLIST Make lists of Ring Buffers.
 ************************************************
 Arg is interpreted as the first sRingBuffer struct.
 Using sRingBuffer.next, all sRingBuffers which will be used are collected.

  ************************************************
  \subsection BLD_SETCPU Sets CPU
  ************************************************
  Sets CPU. See \ref CPU_ID .

  ************************************************
  \subsection BLD_SEARCHRB Search the charged sRingBuffer structs
  ************************************************
  Searches all the structs for connections for which this thread is assigned.
    
  ************************************************
  \subsection BLD_OUTFILE_OPEN Output File Creation
  ************************************************
  Prepares the outputfile.

  ************************************************
  \subsection BLD_STARTSYNC Synchronizes threads before starting DAQ.
  ************************************************
  Waits for the other threads to be prepared as well. See \ref START_SYNC.
  Just after this step, LSTDAQ::DAQtimer object is created to measure DAQ performance.

  ************************************************ 
  \subsection BLD_READ_DATA Read Data From RingBuffer with build
  ************************************************
  The loop procedue of reading data and performing event building. 
  - reading data\n
  Data from all the FEBs are collected by reading data stored in all RingBuffers. 
  This procedure is performed by LSTDAQ::RingBuffer::read() function, which reads data from each RingBuffer by event wise. 
  - event building\n
  The data from all RingBuffer is combined to one data array as one event data for whole camera. The data to be combined must have the result of identical trigger.
  This process makes sure the trigger is identical, investigating if trigger number is the same.
  Trigger number is extracted from data and are checked. If it is not the same number, data in incomplete set will be discarded.

  NOTE: Currently, the procedure of judging trigger is adjusted to DAQ sequence of LST. But it may change. A change of trigger sequence may require modification of event building procedure.
  At this moment, the trigger number check procedure takes into account the discrepancy of trigger number following the readout sequence of FEB as below.

    -# There are two kinds of numbers, trigger number and event number, which are counted up in a FEB. Trigger number counts up when a trigger is accepted. And Event number counts up when data is extracted. Event number cannot be larger than trigger number, because of this order of counting up.

    -# There is a case that FEB accepts trigger but fails to extract data because DRS4 is busy.\n
       This case can happen when the time from previous trigger hasn't passed enough long and is still within the dead time.
       As a result, until the dead time ends the next data cannot be extracted and  trigger number will keep counting up.
       Therefore the data from the FEB will show skip of trigger number.
       When event building, this can be seen as larger number than ones from other FEBs.
    
     -# There can be a case that a FEB drops a trigger.\n
       The event building will result in trouble because all the subsequent data from corresponding FEB will have shifted trigger number.
       To avoid this incident, artificial dead time is set in TIB(Trigger Interface Board) which distributes trigger signal to FEB.
       And additionally, a set of time stamps, 1PPS(1 pulse per second) and 10 MHz signal will be embedded in data to confirm that the time FEB accepted the trigger is the same.
       The check sequence is not implemented now.

  ************************************************ 
  \subsection BLD_SUBMITSUMMARY Submit summary of DAQ
  ************************************************
  Tell LSTDAQ::DAQtimer object to make summary of DAQ measurement.
  
  ************************************************ 
  \subsection BLD_RB_DATAUNLOAD Unload remaining data from RingBuffer
  ************************************************
  Continue LSTDAQ::RingBuffer::read() until all the data in RingBuffer is regarded as read.

  ************************************************
  \subsection COLL_THREADEND Thread ends.
  ************************************************
 
 */
void *Builder_thread(void *arg)
{
  /******************************************/
  //     basic preparation
  /******************************************/
  sRingBuffer *srb[MAX_CONNECTION];
  int offset;
  srb[0]= (sRingBuffer*)arg;
  char tempbuf[EVENTSIZE*MAX_CONNECTION];

  

  //cout<<"*** Builder_thread initialization ***"<<endl;
  int nRB =0;
  while(1)
  {
    // cout<<"srb["<<nRB<<"] :"<<srb[nRB]->next<<endl;
    if(srb[nRB]->next==0||nRB==MAX_RINGBUF)break;
    srb[nRB+1]=srb[nRB]->next;
    nRB++;
  }
  
  int nColl = 1+ getMaxCid();
  
  /******************************************/
  //  CPU Specification
  /******************************************/
  int cpuid;
  cpuid = nColl%Ncpu;
#ifdef __CPU_ZERO
  cpu_set_t mask;
  __CPU_ZERO(&mask);
  __CPU_SET(Ncpu,&mask);
  if(sched_setaffinity(0,sizeof(mask), &mask) == -1)
    printf("WARNING: failed to set CPU affinity... (cpuid=%d)\n",cpuid);
#endif
  
  /******************************************/
  //    Output File Creation
  /******************************************/
  char buf[128];
  // sprintf(buf,"/media/RAID0_Intel/150224/infreq%d_nColl%d_nRB%d.dat"
  sprintf(buf,"infreq%d_nColl%d_nRB%d.dat"
          ,infreq
          ,nColl
          ,nRB);
  FILE *fp_data;
  if((fp_data = fopen(buf,"w"))==NULL){
    cout<<"output file open error!!"<<endl;
    exit(1);
  }
  
  int dataLength = EVENTSIZE*nRB;
  
  /******************************************/
  //    Start Synchronization
  /******************************************/
  while(initEnd<nColl);
  // cout<<"Bld Confirmed all"<<endl;
  cout<<"*** Builder_thread starts to read ***"<<endl;
  cout<<Ndaq<<"events from "<<nRB<<"RBs"<<endl;
  pthread_mutex_lock(&mutex_initLock);
  pthread_cond_broadcast(&cond_allend);
  pthread_mutex_unlock(&mutex_initLock);
  
//  *********** Read Data From RingBuffer without build *************//
//  bool bReadEnd[MAX_CONNECTION]={false};
//  int ReadEnd=0;
//  int ReadCount=0;
//  unsigned long long NreadAll=0;
//  unsigned long Nread[MAX_CONNECTION]={0};
//    unsigned int *p_Ntrg[MAX_CONNECTION];
//    unsigned int *p_Nevt[MAX_CONNECTION];
//    unsigned long int Ntrg[MAX_CONNECTION]={0};
//    unsigned long int Nevt[MAX_CONNECTION]={0};
//    unsigned long int NtrgPrev[MAX_CONNECTION]={0};
//    unsigned long int NevtPrev[MAX_CONNECTION]={0};
//    unsigned long int NtrgSkip[MAX_CONNECTION]={0};
//    unsigned long int NevtSkip[MAX_CONNECTION]={0};
//  
//  for(int i=0;i<nRB;i++)
//  {
//    NevtSkip[i]=0;
//    NtrgSkip[i]=0;
//  }
//
//
//  LSTDAQ::DAQtimer *dt=new LSTDAQ::DAQtimer(nRB);
//  dt->DAQstart();
//  while(1)
//  {
//    //cout<<"@@"<<ReadEnd<<endl;
//    offset = 0;
//    for(int i =0;i<nRB;i++)
//    {
//      /*modified on 1227 due to modification of ret val in RingBuffer.cpp*/
//      if(srb[i]->rb->read(&tempbuf[offset])==0)
//      {
//        Nread[i]++;
//        NreadAll++;
//        ReadCount++;
//        p_Nevt[i]=(unsigned int*)&tempbuf[offset+HEADERLEN+IPADDRLEN];
//        p_Ntrg[i]=(unsigned int*)&tempbuf[offset+HEADERLEN+IPADDRLEN+EVTNOLEN];
//        Nevt[i]=(unsigned long)*p_Nevt[i];
//        Ntrg[i]=(unsigned long)*p_Ntrg[i];
//	// cout<<Nevt[i] - NevtPrev[i] - 1<<endl;
//	// cout<<Ntrg[i] - NtrgPrev[i] - 1<<endl;
//        NevtSkip[i]= (unsigned long )
//	  ((double)NevtSkip[i]+((double)Nevt[i]-(double)NevtPrev[i] -1.));
//        NtrgSkip[i]= (unsigned long)
//	  ((double)NtrgSkip[i]+((double)Ntrg[i]-(double)NtrgPrev[i]-1.));
//	NevtPrev[i]=Nevt[i];
//	NtrgPrev[i]=Ntrg[i];
//      }
//      // cout << "Bld read from Coll"<<srb[i]->sRBid <<" :"<< tempbuf <<endl;
//      //cout<<i<<":"<<Nread[i];//<<endl;
//      if(Nread[i]>=Ndaq && !bReadEnd[i])
//      {
//        ReadEnd++;
//        bReadEnd[i]=true;
//      }
//      if (ReadCount==nRB) {
//        dt->readend();
//        ReadCount=0;
//      }
//      offset+=EVENTSIZE;
//    }
//
//    if (ReadEnd==nRB)break;
//  }
//  
//  dt->DAQend();
//  dt->DAQsummary(infreq,NreadAll,nRB,nColl,Ntrg,Nevt);
//  dt->DAQerrsummary(infreq,NreadAll,nRB,Ntrg,Nevt,NtrgSkip,NevtSkip);
//  fclose(fp_data);
//  cout << "Builder thread(without Merge) end."<< Nread[0]<<"data was read."<<endl;
//

  /******************************************/
  //     Read Data From RingBuffer with build
  /******************************************/
  bool bReadEnd[MAX_CONNECTION]={false};
  // bool bReadStart=false;
  int ReadEnd=0;
  
  unsigned long NreadAll=0;
  unsigned long Nread[MAX_CONNECTION]={0};
  unsigned long cNtrg;  //current Ntrg to collect.
  unsigned long rNtrg;  //read Ntrg. first set to rNtrg=cNtrg, when larger Ntrg is come, stored.
  unsigned int *p_Ntrg[MAX_CONNECTION];
  unsigned int *p_Nevt[MAX_CONNECTION];
  unsigned long Ntrg[MAX_CONNECTION]={0};
  unsigned long Nevt[MAX_CONNECTION]={0}; 
  LSTDAQ::DAQtimer *dt=new LSTDAQ::DAQtimer(nRB);
  dt->DAQstart();
  
  cNtrg=0;
  int SkipRB=-1;

  cout<<"nRB"<<nRB<<endl;
  while(1)
  {
    offset=0;
    for(int i=0;i<nRB;i++)
    {
      // cout<<"SkipRB"<<SkipRB<<endl;
      if(i==SkipRB||bReadEnd[i])
      {
        offset+=EVENTSIZE;
      }
      else
      {
	// cout<<i<<" "<<SkipRB<<endl;
        while(srb[i]->rb->read(&tempbuf[offset])==-1)continue;
	// if(!bReadStart)bReadStart=true;
        p_Nevt[i]=(unsigned int*)&tempbuf[offset+POSEVTNO];
        p_Ntrg[i]=(unsigned int*)&tempbuf[offset+POSTRGNO];
        Nevt[i]=(unsigned long)*p_Nevt[i];
        Ntrg[i]=(unsigned long)*p_Ntrg[i];
	inverseByteOrder((char *)&Nevt[i],sizeof(unsigned long));
	inverseByteOrder((char *)&Ntrg[i],sizeof(unsigned long));
	// cout<<i<<" "<<Nevt[i]<<endl;
        Nread[i]++;
        if(Nread[i]>=Ndaq)
        {
	  printf("RB%d end",i);
          bReadEnd[i]=true;
          ReadEnd++;
        }
        
        if(Ntrg[i]==cNtrg)
        {
          offset+=EVENTSIZE;
        }
        else if(Ntrg[i]<cNtrg)
        {
          while(1)
          {
            while(srb[i]->rb->read(&tempbuf[offset])==-1)continue;
            p_Nevt[i]=(unsigned int*)&tempbuf[offset+POSEVTNO];
            p_Ntrg[i]=(unsigned int*)&tempbuf[offset+POSTRGNO];
            Nevt[i]=(unsigned long)*p_Nevt[i];
            Ntrg[i]=(unsigned long)*p_Ntrg[i];
	    inverseByteOrder((char *)&Nevt[i],sizeof(unsigned long));
	    inverseByteOrder((char *)&Ntrg[i],sizeof(unsigned long));
	    // cout<<i<<" "<<Nevt[i]<<endl;
            Nread[i]++;
            if(Nevt[i]>=Ndaq)
            {
	      printf("RB%d end",i);
              bReadEnd[i]=true;
              ReadEnd++;
              break;
            }
            if(Ntrg[i]==cNtrg)
            {
              offset+=EVENTSIZE;
              break;
            }
            else if(Ntrg[i]<cNtrg)
            {
              continue;
            }
            else //if(Ntrg[i]>cNtrg)
            {
	      // printf("A");
              rNtrg=Ntrg[i];
              SkipRB=i;
              break;
            }
          }//while(1)
        }//if(skip or end)
        else //if(Ntrg[i]>cNtrg)
        {
          rNtrg=Ntrg[i];
          SkipRB=i;
        }
      }//for(i<nRB)
      
      // cout<<"rNtrg"<<rNtrg<<"cNtrg"<<cNtrg<<endl;
      if(rNtrg>cNtrg)
      {
	// printf("B");
        cNtrg=rNtrg;
        break;
      }
      // if(bReadStart && cNtrg==rNtrg && i==(nRB-1))
      if(cNtrg==rNtrg && i==(nRB-1))
      {
	dt->readend();
	NreadAll++;
        cNtrg++;
        rNtrg++;
	SkipRB=-1;
	//fwrite;
	if(datacreate==true)
	  fwrite(&tempbuf,dataLength,1,fp_data);

      }

    }
    // cout<<"RE"<<ReadEnd<<endl;
    // if (ReadEnd==nRB)break;
    if (ReadEnd>0)break;
  }
  
  dt->DAQend();
  dt->DAQsummary(infreq,NreadAll,nRB,nColl,Ntrg,Nevt);
  fclose(fp_data);
  for(int i=0;i<nRB;i++)
  {
    while(srb[i]->rb->read(&tempbuf[offset])!=-1)continue;
  }
  cout << "Builder thread end."<< NreadAll<<"data was read."<<endl;
  //sleep(1);
}


/****************************/
// main
/****************************/
/** 
 \func main
 \param infreq
 \brief Input frequency.
  (i.e.the frequency of triggers which are given to FEB.)\n
  specified by argv[1].\n
  This is used only for making report.
 \param Ndaq
 \brief The number of events to acquire.\n
 specified by argv[2], otherwise DAQ_NEVENT is set.

 \param datacreate
 \brief Whether data will be saved.
 specified by argv[3].
 If yes, data is saved to disk.
 

 *********************************************************************
 \section MAIN_PROC The procedures
 *********************************************************************
  -# Set parameters from args
  -# Inspect # of CPUs
  -# Read configuration file and check its validity.
  -# Set configurations for thread and connection in sRIngBuffer structs.
  -# submit Multi-threaded processes
  -# wait all the Multi-threaded processes to finish

 ************************************************
 \subsection MAIN_SET_PARAM Set parameters from args
 ************************************************
 At submission, main function can receive 1 to 3 arguments.
 The arg[] are set as follows.\n
 Ndaq = arg[1]\n
 infreq = arg[2]\n
 datacreate = arg[3]\n
 
 ************************************************
 \subsection MAIN_INS_CPU Inspect # of CPUs
 ************************************************
 Ncpu is set from sysconf().
 see \ref CPU_ID . 
 

 ************************************************
 \subsection MAIN_READCONF Read configuration file and check its validity.
 ************************************************
 Read "Connection.conf" in which empty lines and lines start with "#" are neglected.
 - Parameters to be set\n
     - Below are used to store the values read from "Connection.conf" temporally .
         - shCid  : Collector id. 
         - szAddr : IP address of FEB.
         - shPort : Port number of FEB.
     - Below are set from the values above.
         - maxCid : The maximum value of Collector ID.
         - firstRB: The first connection ID for each collector thread.
     
 - Restrictions\n
 There is a rule below for writing configuration files. If the values above violates it, the program exits. 
     - Limit of # of connections\n
     If more connections than MAX_CONNECTION are written in "Connection.conf", program exits.
     - No skip of Cid\n
     If there is interval in Cid specification, program exits.
 
 ************************************************
 \subsection MAIN_SETSRB Set configurations for thread and connection in sRIngBuffer structs.
 ************************************************

 - After validation,
 The parameters below are set to sRingBuffer structs from the parameters above.
     - sRB.Cid
     - sRB.szAddr
     - sRB.shPort
 

 ************************************************
 \subsection MAIN_SUBMIT  Submit Multi-threaded processes
 ************************************************
 By pthread_create() function, threads are created. 
 handle[]  is an array of pthread_t type variable, which is used to handle threads.
 By their numbers the array, threads are numbered as these.\n

 Thread name     | Number of threads |  handle[]
-----------------|-------------------|--------------
Collector_thread | nColl             |  0 - nColl-1
Builder_thread   | 1                 |  nColl
ThruPutMes_thread| 1                 |  nColl+1

Note:\n
Which sRingBuffer a collector thread take charge of is defined by creating the tread giving the first sRingBuffer struct to be treated by it.


 ************************************************
 \subsection MAIN_JOIN wait all the Multi-threaded processes to finish
 ************************************************
 pthread_join() function waits for a thread to end. When all the threads finished, this function ends.
 
 */



int main(int argc, char** argv)
{
  /******************************************/
  //   Set parameters from args
  /******************************************/
  infreq=0;
  Ndaq=DAQ_NEVENT;
  datacreate=false;
  // if(argc >4||argc<2)
  //   usage(argv[0]);
  // if(argc >= 2)
  //   infreq = atoi(argv[1]);
  // if(argc >= 3)
  //   Ndaq = atoi(argv[2]);
  // if(argc == 4)
  // {
  //   switch (atoi(argv[3]))
  //   {
  //   case 1:
  //     cout<<"NOTE:data will be created."<<endl;
  //     datacreate = true;
  //     break;
  //   case 0:
  //     datacreate = false;
  //     break;
  //   default:
  //     usage(argv[0]);
  //   }
  // }

  int opt;
  int index;
  while((opt=getopt_long(argc,argv,"hi:n:o:r:sv:cf:",options,&index)) !=-1){
    switch(opt)
      {
	TERM_COLOR_RED;
	printf("Usage:\n");
	printf("you run this program with some options like\n");
	printf("%s -o MyFileNameHeader -s -r 50 -n 10000\n",argv[0]);
	printf("***** LIST OF OPTIONS *****\n");
	printf("-i|--infreq <Input Frequency[Hz]>    : Trigger frequency. \n");
	printf("-o|--output <FileNameHeader>         : Header of data file .\n");
	printf("-s|--save                            : Datasave. Default is false.\n");
	printf("-r|--readdepth <ReadDepth>           : Default is 30.\n");
	printf("-n|--Ndaq <#of data to acquire>      : Default is 1000.\n");
	printf("-v|--version   <Dragon Version>      : Default is 5.\n");
	printf("-c|--closeinspect                    : Default is false.\n");
	printf("-f|--configfile                      : .\n");
	printf("********* CAUTION ********\n");
	printf("Make sure to specify readdepth to Dragon through rpcp command.\n");
	printf("If RD=1024,limit is 3kHz at 1Gbps. so 10000events will take 10s. \n");
	printf("If RD=30,limit is 120kHz at 1Gbps. so 1000000events will take 10s. \n");
	printf("Close inspection mode will store\n");
	printf("  time differences between events.\n");
	printf("  as a file named RDXXinfreqXX_MMDD_HHMMSS.dat\n");
	printf("  in DragonDaqMes directory which will be made automatically.\n");
	printf("\n");
	TERM_COLOR_RESET;
	exit(0);
      
      case 'i':
	infreq=atoi(optarg);
	break;
      case 'o':
	//sprintf(fileNameHeader,"%s",optarg);
	//if(sizeof(optarg)==0);
	//      printf("%d\n",sizeof(optarg));
	fileNameHeader=optarg;
	//outputfile=optarg;
	break;
      
      case 's':
	datacreate=true;
	break;
      case 'n':
	Ndaq=(unsigned int)atoi(optarg);
	printf("Ndaq %d\n",Ndaq);
	break;
      
      // case 'r':
      // 	rddepth=atoi(optarg);
      // 	break;
      // case 'v':
      // 	dragonVer=atoi(optarg);
      // 	break;
      // case 'c':
      // 	closeinspect=true;
      // 	break;
      // case 'f' :
      // 	configfile=optarg;
      // 	break;
      default:
	printf("%s -h for usage\n",argv[0]);
      } 
  }

  cout << "***LSTDAQ starts***" <<endl;
  
  /******************************************/
  //  Inspect # of CPUs
  /******************************************/
  Ncpu = sysconf(_SC_NPROCESSORS_CONF);
  cout<<"This machine has"<<Ncpu<<" cpus"<<endl;
  /******************************************/
  //   Read Connection Configuration and
  //     prepare the assignment
  /******************************************/
  unsigned short shCid[MAX_CONNECTION]={0};
  char szAddr[MAX_CONNECTION][16];
  unsigned short shPort[MAX_CONNECTION]={0};
  unsigned long lConnected=0;
  const char *ConfFile = "Connection.conf";
  std::ifstream ifs(ConfFile);
  std::string str;
  int nServ=0;
  while (std::getline(ifs,str)){
    if(str[0]== '#' || str.length()==0)continue;
    std::istringstream iss(str);
    iss >> shCid[nServ]>> szAddr[nServ] >> shPort[nServ];
    nServ++;
  }
  if(nServ>MAX_CONNECTION){
    printf("The number of connections excessed limit.");
    exit(1);
  }
  
  //Cid validation
  int maxCid=0;
  for(int i=0;i<nServ;i++)
  {
    if(maxCid<shCid[i])maxCid=shCid[i];
  }
  for(int i=0;i<maxCid;i++)
  {
    bool Cid_exist=false;
    for(int j=0;j<nServ;j++)
    {
      if(shCid[j]==i)
      {
        Cid_exist=true;
        break;
      }
    }
    if(!Cid_exist)
    {
      cout<<"error: Cid "<<i<<"is skipped."<<endl;
      exit(1);
    }
  }
  int nColl=maxCid+1;
  int firstRB[MAX_CONNECTION];
  //substitute the first connection(sRBid)
  for(int i=0;i<nColl;i++)
  {
    for(int j=0;j<nServ;j++)
    {
      if(shCid[j]==i)//if given CollID matches Cid
      {
        firstRB[i]=j;//Coll starts to seek from it
        break;
      }
    }
  }
  sRBinit();
  sRBcreate(nServ);
  for(int i=0;i<nServ;i++)
    sRBsetaddr(i,shCid[i],szAddr[i],shPort[i]);
  
  cout<<"****** Configuration of RinbBuffers are set ******"<<endl;
  cout<<nColl<<" Collectors will be created for "<<nServ<<" connections."<<endl;
  cout<<"RBid "<<"shCid "<<"     IP address     "<<" port "<<endl;
  for(int i=0;i<nServ;i++)
    cout<<"RB"<< setw(2) << setfill(' ')<<i<<"|"<<
    setw(6) << setfill(' ')<<sRB[i].Cid<<"|"<<
    setw(18) << setfill(' ')<<sRB[i].szAddr<<"|"<<
    setw(5) << setfill(' ')<<sRB[i].shPort<<endl;
  
  /******************************************/
  //    submit Multi-threaded processes
  /******************************************/
  // cout << "***  Thread create  ***"<<endl;
  pthread_t handle[nColl+2];
  for(int i=0;i<nColl;i++)
  {
    pthread_create(&handle[i],
                   NULL,
                   &Collector_thread,
                   &sRB[firstRB[i]]);
    // sleep(1);
  }
  pthread_create(&handle[nColl],
                 NULL,
                 &Builder_thread,
                 &sRB[0]);
  pthread_create(&handle[nColl+1],
                 NULL,
                 &ThruPutMes_thread,
                 &sRB[0]);

  // cout <<"Threads created. "<<
  // Ndaq << "events will be transferred each"<<endl;
    /******************************************/
    //    wait all the Multi-threaded processes to finish
    /******************************************/
    for(int i=0;i<nColl+2;i++)
    pthread_join(handle[i],NULL);
  
    cout << "LSTDAQ end" <<endl;
    return 0;
  }




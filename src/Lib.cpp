#include "Lib.hpp"


/*!
 * \fn void usage(char *argv)
 * print usage
 */
void usage(char **argv)
{
	TERM_COLOR_RED;
	printf("Usage:\n");
	printf("Execute %s with some options like\n",argv[0]);
	printf("%s -o MyFileNameHeader -s -r 50 -n 10000\n",argv[0]);
	printf("***** LIST OF OPTIONS *****\n");
	printf("-i|--infreq <Input Frequency[Hz]>    : Trigger frequency. \n");
	printf("-o|--output <FileNameHeader>         : Header of data file .\n");
	printf("-s|--save                            : Datasave. Default is false.\n");
	//	printf("-r|--readdepth <ReadDepth>           : Default is 30.\n");
	printf("-n|--Ndaq <#of data to acquire>      : Default is 1000.\n");
	//	printf("-v|--version   <Dragon Version>      : Default is 5.\n");
	//	printf("-c|--closeinspect                    : Default is false.\n");
	//	printf("-f|--configfile                      : .\n");
	printf("-l|--logringbuffer                   : .\n");
	// printf("********* CAUTION ********\n");
	// printf("Make sure to specify readdepth to Dragon through rpcp command.\n");
	// printf("If RD=1024,limit is 3kHz at 1Gbps. so 10000events will take 10s. \n");
	// printf("If RD=30,limit is 120kHz at 1Gbps. so 1000000events will take 10s. \n");
	// printf("Close inspection mode will store\n");
	// printf("  time differences between events.\n");
	// printf("  as a file named RDXXinfreqXX_MMDD_HHMMSS.dat\n");
	// printf("  in DragonDaqMes directory which will be made automatically.\n");
	printf("Goodbye\n");
	printf("\n");
	TERM_COLOR_RESET;
	exit(0);
	// printf("usage: %s <infreq[Hz]> <Ndaq[events]> <datacreate> \n",argv);
	// printf("<Ndaq>       -- optional.default value is %d\n",DAQ_NEVENT);
	// printf("<datacreate> -- 1:create data. 0 or no specification: don't create data.\n"); 
	// exit(0);
}


/****************************/
// inverse Byte Order
/****************************/

/*!
 * \fn void inverseByteOrder(char *buf,int bufsize)
 * \brief Flips the byte order of given array
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

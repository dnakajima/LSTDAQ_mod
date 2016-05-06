#ifndef __LIB_H
#define __LIB_H

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
#include "termcolor.h"

void usage(char **argv);
void inverseByteOrder(char *buf,int bufsize);


#endif

/********************************************************************************/
/*										*/
/*		Socket Interface to a TPM Simulator    				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TcpServerPosix.c 1528 2019-11-20 20:31:43Z kgoldman $	*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2012 - 2019				*/
/*										*/
/********************************************************************************/

// D.3	TcpServer.c
// D.3.1. Description
// This file contains the socket interface to a TPM simulator.
// D.3.2. Includes, Locals, Defines and Function Prototypes

#include <stdio.h>
/* FIXME need Posix TCP socket code */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "string.h"
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "TpmTcpProtocol.h"
#include "Manufacture_fp.h"
#include "TcpServerPosix_fp.h"
#include "Simulator_fp.h"
#include "TpmProfile.h"		/* kgold */
#include "Platform_fp.h"	/* kgold */
#include "PlatformACT_fp.h"	/* added kgold */

BOOL				/* kgold */
ReadUINT32(
	   SOCKET           s,
	   UINT32          *val
	   );


#define IPVER(len) ((len) == sizeof(struct sockaddr_in6) ? 6 :		\
		    ((len) == sizeof(struct sockaddr_in) ? 4 : 0))

#ifndef __IGNORE_STATE__
static UINT32 ServerVersion = 1;
#define MAX_BUFFER 1048576
char InputBuffer[MAX_BUFFER];       //The input data buffer for the simulator.
char OutputBuffer[MAX_BUFFER];      //The output data buffer for the simulator.

struct {
    UINT32      largestCommandSize;
    UINT32      largestCommand;
    UINT32      largestResponseSize;
    UINT32      largestResponse;
} CommandResponseSizes = {0};

#endif // __IGNORE_STATE___

// D.3.3.	Functions
// D.3.3.1.	CreateSocket()
// This function creates a socket listening on PortNumber.

static int
CreateSocket(
	     int                  PortNumber,
	     SOCKET              *listenSocket,
	     socklen_t           *addr_len,
	     int                  domain 	// AF_INET or AF_INET6
	     )
{
    struct		sockaddr_in MyAddress4;
    struct		sockaddr_in6 MyAddress6;
    int			opt;
    
    int res;
    
    // create listening socket
    *listenSocket = socket(domain, SOCK_STREAM, 0);
    if(*listenSocket == -1)
	{
            printf("Warning: Cannot create server listen %s socket\nWarning is %d %s\n",
                   domain == AF_INET6 ? "IPv6" :
                   (domain == AF_INET) ? "IPv4" : "?", errno, strerror(errno));
	    printf("Ignore the IPv6 warning if the platform doesn't support IPv6\n");
	    return -1;
	}

    opt = 1;
    /* Set SO_REUSEADDR before calling bind() for servers that bind to a fixed port number. */
    res = setsockopt(*listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (res != 0) {
	printf("setsockopt error. Error is %d %s\n", errno, strerror(errno));
	return -1;
    }
    // bind the listening socket to the specified port, any address (0s)
    switch (domain) {
      case AF_INET:
	memset((char *)&MyAddress4, 0, sizeof(MyAddress4));
	MyAddress4.sin_family = domain;
	MyAddress4.sin_port = htons((short) PortNumber);
	*addr_len = sizeof(struct sockaddr_in);
	res= bind(*listenSocket,(struct sockaddr*) &MyAddress4, *addr_len);
	break;
      case AF_INET6:
	memset((char *)&MyAddress6, 0, sizeof(MyAddress6));
	MyAddress6.sin6_family = domain;
        MyAddress6.sin6_port = htons((short) PortNumber);
	*addr_len = sizeof(struct sockaddr_in6);

        opt = 1;
        // Set IPPROTO_IPV6 so that it's just for IPv6 and not both IPv4/IPv6.
        res = setsockopt(*listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, &opt,
			 sizeof(opt));
        if (res != 0) {
	    printf("setsockopt IPV6_V6ONLY error. Error is %d %s\n",
		   errno, strerror(errno));
	    return -1;
        }
	res= bind(*listenSocket,(struct sockaddr*) &MyAddress6, *addr_len);
      break;
      default:
        printf("Address family %d not supported\n", domain);
        return -1;
    }
    if(res != 0)
	{
            close(*listenSocket);
	    *listenSocket = -1;
	    printf("Bind error.  Error is  %d %s\n", errno, strerror(errno));
	    return -1;
	}
    
    // listen/wait for server connections
    res= listen(*listenSocket,3);
    if(res != 0)
	{
	    close(*listenSocket);
	    *listenSocket = -1;
            printf("Listen error.  Error is %d %s\n", errno, strerror(errno));
	    return -1;
	}
    
    return 0;
}

// D.3.3.2.	PlatformServer()
// This function processes incoming platform requests.

BOOL
PlatformServer(
	       SOCKET           s
	       )
{
    BOOL                 ok = TRUE;
    UINT32               Command;
    
    for(;;)
	{
	    ok = ReadBytes(s, (char*) &Command, 4);
	    // client disconnected (or other error).  We stop processing this client
	    // and return to our caller who can stop the server or listen for another
	    // connection.
	    if(!ok) return TRUE;
	    Command = ntohl(Command);
	    switch(Command)
		{
		  case TPM_SIGNAL_POWER_ON:
		    _rpc__Signal_PowerOn(FALSE);
		    break;
		    
		  case TPM_SIGNAL_POWER_OFF:
		    _rpc__Signal_PowerOff();
		    break;
		    
		  case TPM_SIGNAL_RESET:
		    _rpc__Signal_PowerOn(TRUE);
		    break;
		    
		  case TPM_SIGNAL_PHYS_PRES_ON:
		    _rpc__Signal_PhysicalPresenceOn();
		    break;
		    
		  case TPM_SIGNAL_PHYS_PRES_OFF:
		    _rpc__Signal_PhysicalPresenceOff();
		    break;
		    
		  case TPM_SIGNAL_CANCEL_ON:
		    _rpc__Signal_CancelOn();
		    break;
		    
		  case TPM_SIGNAL_CANCEL_OFF:
		    _rpc__Signal_CancelOff();
		    break;
		    
		  case TPM_SIGNAL_NV_ON:
		    _rpc__Signal_NvOn();
		    break;
		    
		  case TPM_SIGNAL_NV_OFF:
		    _rpc__Signal_NvOff();
		    break;
		    
		  case TPM_SESSION_END:
		    // Client signaled end-of-session
		    TpmEndSimulation();
		    return TRUE;
		    
		  case TPM_STOP:
		    // Client requested the simulator to exit
		    return FALSE;
		    
		  case TPM_TEST_FAILURE_MODE:
		    _rpc__ForceFailureMode();
		    break;
		    
		  case TPM_GET_COMMAND_RESPONSE_SIZES:
		    ok = WriteVarBytes(s, (char *)&CommandResponseSizes,
				       sizeof(CommandResponseSizes));
		    memset(&CommandResponseSizes, 0, sizeof(CommandResponseSizes));
		    if(!ok)
			return TRUE;
		    break;
		  case TPM_ACT_GET_SIGNALED:
		      {
			  UINT32 actHandle = 0;
			  ok = ReadUINT32(s, &actHandle);
			  WriteUINT32(s, _rpc__ACT_GetSignaled(actHandle));
			  break;
		      }
		  default:
		    printf("Unrecognized platform interface command %08x\n", Command);
		    WriteUINT32(s, 1);
		    return TRUE;
		}
	    WriteUINT32(s,0);
	}
    return FALSE;
}

// D.3.3.3.	PlatformSvcRoutine()
// This function is called to set up the socket interfaces to listen for commands.

int
PlatformSvcRoutine(
		   void *port
		   )
{
    int                  PortNumber = *(int *)port;

    SOCKET               listenSocket[2], maxListenSocket, serverSocket;
    struct               sockaddr_storage HerAddress;
    fd_set               sockSet;
    int                  res, i;
    int                  nSock = 0;
    socklen_t            length[2];
    BOOL                 continueServing = TRUE;	/* kgold initialized */

    if (CreateSocket(PortNumber, &listenSocket[nSock], &length[nSock],
		     AF_INET) == 0) {
        nSock++;

    }
    if (CreateSocket(PortNumber, &listenSocket[nSock], &length[nSock],
		     AF_INET6) == 0) {
        nSock++;
    }
    if (nSock == 0) {
	printf("Create platform service socket fail\n");
	return -1;
    }

    maxListenSocket = listenSocket[0];
    if ((nSock == 2) && (listenSocket[1] > maxListenSocket)) {
        maxListenSocket = listenSocket[1];
    }
   
    // Loop accepting connections one-by-one until we are killed or asked to stop
    // Note the platform service is single-threaded so we don't listen for a new
    // connection until the prior connection drops.
    do {
        printf("Platform server listening on port %d\n", PortNumber);

	// Select both IPv4 and IPv6 sockets or whatever is available
	FD_ZERO(&sockSet);
	FD_SET(listenSocket[0], &sockSet);
	if (nSock == 2)
	    FD_SET(listenSocket[1], &sockSet);
	do {
	    res = select(maxListenSocket + 1, &sockSet, NULL, NULL, NULL);
	}
	while ((res == -1) && (errno == EINTR));
	if (res == -1) {
	    printf("Platform server select error.  Error is %d %s\n",
		   errno, strerror(errno));
	    return -1;
	}

	for (i = 0; i < nSock; i++) {
	    int ipver = IPVER(length[i]);

	    if (!FD_ISSET(listenSocket[i], &sockSet))
		{
		    continue;
		}
	    // blocking accept
	    serverSocket = accept(listenSocket[i],
				  (struct sockaddr*) &HerAddress,
				  &length[i]);
	    if(serverSocket < 0)
		{
		    printf("Platform server IPv%d Accept error.  Error is %d %s\n",
			   ipver, errno, strerror(errno));
		    return -1;
		};
	    printf("Platform IPv%d client accepted\n", ipver);
	    
	    // normal behavior on client disconnection is to wait for a new
	    // client to connect
	    continueServing = PlatformServer(serverSocket);
	    close(serverSocket);
	    serverSocket = -1;
	}
    }
    while(continueServing);
    
    return 0;
}

// D.3.3.4.	PlatformSignalService()

// This function starts a new thread waiting for platform signals. Platform signals are processed
// one at a time in the order in which they are received.

int
PlatformSignalService(
		      int              *PortNumberPlatform
		      )
{
    unsigned long       thread;
    int                 irc = 0;
    pthread_t           *pthread = (pthread_t *)&thread;

    irc = pthread_create(pthread,
                         NULL,
                         (void * (*)(void *))PlatformSvcRoutine,      /* thread entry function */
                         (void *)PortNumberPlatform); 	          /* thread function parameters */
    if (irc != 0) {
	printf("Thread Creation failed\n");
	return -1;
    }
    return 0;
}

// D.3.3.5.	RegularCommandService()
// This funciton services regular commands.

int
RegularCommandService(
		      int              *PortNumber
		      )
{
    SOCKET               listenSocket[2], maxListenSocket;
    SOCKET               serverSocket;
    struct               sockaddr_storage HerAddress;
    fd_set               sockSet;
    int                  res, i;
    int                  nSock = 0;
    socklen_t            length[2];
    BOOL 		continueServing = TRUE;	/* kgold - initialized */
    
    if (CreateSocket(*PortNumber, &listenSocket[nSock], &length[nSock],
		     AF_INET) == 0) {
        nSock++;

    }
    if (CreateSocket(*PortNumber, &listenSocket[nSock], &length[nSock],
		     AF_INET6) == 0) {
        nSock++;
    }
    if (nSock == 0) {
        printf("Create TPM command service socket fail\n");
        return -1;
    }
    maxListenSocket = listenSocket[0];
    if (nSock == 2 && listenSocket[1] > maxListenSocket) {
        maxListenSocket = listenSocket[1];
    }
    
    // Loop accepting connections one-by-one until we are killed or asked to stop
    // Note the TPM command service is single-threaded so we don't listen for
    // a new connection until the prior connection drops.
    do
	{
	    printf("TPM command server listening on port %d\n", *PortNumber);

	    // Select both IPv4 and IPv6 sockets or whatever is available
	    FD_ZERO(&sockSet);
	    FD_SET(listenSocket[0], &sockSet);
	    if (nSock == 2)
		FD_SET(listenSocket[1], &sockSet);
	    do {
	        res = select(maxListenSocket + 1, &sockSet, NULL, NULL, NULL);
	    } while ((res == -1) && (errno == EINTR));
	    if (res == -1) {                                      
	        printf("TPM command server select error.  Error is %d %s\n", errno,
		       strerror(errno));                             
	        return -1;                                        
	    }                                                     

	    for (i = 0; i < nSock; i++) {
	        int ipver = IPVER(length[i]);

	        if (!FD_ISSET(listenSocket[i], &sockSet))
	            continue;
	        // blocking accept
		serverSocket = accept(listenSocket[i],
				      (struct sockaddr*) &HerAddress,
				      &length[i]);
		if(serverSocket < 0)
		    {
			printf("TPM server IPv%d Accept error.  Error is %d %s\n",
			       ipver, errno, strerror(errno));
			return -1;
		    };
	        printf("Command IPv%d client accepted\n", ipver);
	    
	        // normal behavior on client disconnection is to wait for a new
	        // client to connect
	        continueServing = TpmServer(serverSocket);
	        close(serverSocket);
	        serverSocket = -1;
	    }
	}
    while(continueServing);
    
    return 0;
}

/* D.3.2.5.	SimulatorTimeServiceRoutine() */
/* This function is called to service the time ticks. */
static int
SimulatorTimeServiceRoutine(
			    void
			    )
{
    // All time is in ms
    const INT64   tick = 1000;
    UINT64  prevTime = _plat__RealTime();
    INT64   timeout = tick;
    
    while (TRUE)
	{
	    UINT64  curTime;
	    
	    struct timespec req = {timeout / 1000, (timeout % 1000) * 1000};
	    struct timespec rem;
	    nanosleep(&req, &rem);

	    curTime = _plat__RealTime();
	    
	    // May need to issue several ticks if the Sleep() took longer than asked,
	    // or no ticks at all, it Sleep() was interrupted prematurely.
	    while (prevTime < curTime - tick / 2)
	        {
	            //printf("%05lld | %05lld\n",
	            //      prevTime % 100000, (curTime - tick / 2) % 100000);
	            _plat__ACT_Tick();
	            prevTime += (UINT64)tick;
	        }
	    // Adjust the next timeout to keep the average interval of one second
	    timeout = tick + (prevTime - curTime);
	    //prevTime = curTime;
	    //printf("%04lld | c:%05lld | p:%05llu\n",
	    //          timeout, curTime % 100000, prevTime);
	}
    return 0;
}

#if RH_ACT_0

/* D.3.2.6.	ActTimeService() */
/* This function starts a new thread waiting to wait for time ticks. */
/* Return Value	Meaning */
/* ==0	success */
/*     !=0	failure */
static int
ActTimeService(
	       void
	       )
{
    static BOOL          running = FALSE;
    int                  ret = 0;
    if(!running)
	{
	    pthread_t            thread_id;

	    /* kgold - hoisted out of WIndows only version */
	    printf("Starting ACT thread...\n");
	    //  Don't allow ticks to be processed before TPM is manufactured.
	    _plat__ACT_EnableTicks(FALSE);

	    //
	    ret = pthread_create(&thread_id, NULL, (void*)SimulatorTimeServiceRoutine,
				 NULL);
	    
	    if(ret != 0)
		printf("ACT thread Creation failed\n");
	    else
		running = TRUE;
	}
    return ret;
}
#endif // RH_ACT_0

// D.3.3.6.	StartTcpServer()

// Main entry-point to the TCP server.  The server listens on port specified. Note that there is no
// way to specify the network interface in this implementation.

int
StartTcpServer(
	       int              *PortNumber,
	       int              *PortNumberPlatform
	       )
{
    int                  res;

#if RH_ACT_0 || 1
    // Start the Time Service routine
    res = ActTimeService();
    if(res != 0)
	{
	    printf("TimeService failed\n");
	    return res;
	}
#endif
    // Start Platform Signal Processing Service
    res = PlatformSignalService(PortNumberPlatform);
    if (res != 0)
	{
	    printf("PlatformSignalService failed\n");
	    return res;
	}
    
    // Start Regular/DRTM TPM command service
    res = RegularCommandService(PortNumber);
    if (res != 0)
	{
	    printf("RegularCommandService failed\n");
	    return res;
	}
    
    return 0;
}

// D.3.3.7.	ReadBytes()
// This function reads the indicated number of bytes (NumBytes) into buffer from the indicated
// socket.

BOOL
ReadBytes(
	  SOCKET           s,
	  char            *buffer,
	  int              NumBytes
	  )
{
    int                  res;
    int                  numGot = 0;
    
    while(numGot<NumBytes)
	{
	    res = read(s, buffer+numGot, NumBytes-numGot);
	    if(res < 0)
		{
		    printf("read() error.  Error is %d %s\n", errno, strerror(errno));
		    return FALSE;
		}
	    if(res==0)
		{
		    return FALSE;
		}
	    numGot+=res;
	}
    return TRUE;
}

// D.3.3.8.	WriteBytes()
// This function will send the indicated number of bytes (NumBytes) to the indicated socket

BOOL
WriteBytes(
	   SOCKET           s,
	   char            *buffer,
	   int              NumBytes
	   )
{
    int                  res;
    int                  numSent = 0;
    while(numSent<NumBytes)
	{
	    res = write(s, buffer+numSent, NumBytes-numSent);
	    if(res == 0)
		{
		    printf("write() error. Error is %d %s\n",  errno, strerror(errno));
		    return FALSE;
		}
	    numSent+=res;
	}
    return TRUE;
}

// D.3.3.9.	WriteUINT32()
// Send 4 byte integer

BOOL
WriteUINT32(
	    SOCKET           s,
	    UINT32           val
	    )
{
    UINT32 netVal = htonl(val);
    return WriteBytes(s, (char*) &netVal, 4);
}

/* D.3.2.11.	ReadUINT32() */
/* Function to read 4 byte integer from socket. */
BOOL
ReadUINT32(
	   SOCKET           s,
	   UINT32          *val
	   )
{
    UINT32 netVal;
    //
    if (!ReadBytes(s, (char*)&netVal, 4))
	return FALSE;
    *val = ntohl(netVal);
    return TRUE;
}

// D.3.3.10.	ReadVarBytes()
// Get a UINT32-length-prepended binary array.  Note that the 4-byte length is in network byte
// order (big-endian).

BOOL
ReadVarBytes(
	     SOCKET           s,
	     char            *buffer,
	     UINT32          *BytesReceived,
	     int              MaxLen
	     )
{
    int                  length;
    BOOL                 res;
    
    res = ReadBytes(s, (char*) &length, 4);
    if(!res) return res;
    length = ntohl(length);
    *BytesReceived = length;
    if(length>MaxLen)
	{
	    printf("Buffer too big.  Client says %d\n", length);
	    return FALSE;
	}
    if(length==0) return TRUE;
    res = ReadBytes(s, buffer, length);
    if(!res) return res;
    return TRUE;
}

// D.3.3.11.	WriteVarBytes()
// Send a UINT32-length-prepended binary array.  Note that the 4-byte length is in network byte
// order (big-endian).

BOOL
WriteVarBytes(
	      SOCKET           s,
	      char            *buffer,
	      int              BytesToSend
	      )
{
    UINT32               netLength = htonl(BytesToSend);
    BOOL res;
    
    res = WriteBytes(s, (char*) &netLength, 4);
    if(!res) return res;
    res = WriteBytes(s, buffer, BytesToSend);
    if(!res) return res;
    return TRUE;
}

// D.3.3.12.	TpmServer()
// Processing incoming TPM command requests using the protocol / interface defined above.

BOOL
TpmServer(
	  SOCKET           s
	  )
{
    UINT32               length;
    UINT32               Command;
    BYTE                 locality;
    BOOL                 ok;
    int                  result;
    int                  clientVersion;
    _IN_BUFFER           InBuffer;
    _OUT_BUFFER          OutBuffer;
    
    for(;;)
	{
	    ok = ReadBytes(s, (char*) &Command, 4);
	    // client disconnected (or other error).  We stop processing this client
	    // and return to our caller who can stop the server or listen for another
	    // connection.
	    if(!ok)
		return TRUE;
	    Command = ntohl(Command);
	    switch(Command)
		{
		  case TPM_SIGNAL_HASH_START:
		    _rpc__Signal_Hash_Start();
		    break;
		    
		  case TPM_SIGNAL_HASH_END:
		    _rpc__Signal_HashEnd();
		    break;
		    
		  case TPM_SIGNAL_HASH_DATA:
		    ok = ReadVarBytes(s, InputBuffer, &length, MAX_BUFFER);
		    if(!ok) return TRUE;
		    InBuffer.Buffer = (BYTE*) InputBuffer;
		    InBuffer.BufferSize = length;
		    _rpc__Signal_Hash_Data(InBuffer);
		    break;
		    
		  case TPM_SEND_COMMAND:
		    ok = ReadBytes(s, (char*) &locality, 1);
		    if(!ok)
			return TRUE;
		    
		    ok = ReadVarBytes(s, InputBuffer, &length, MAX_BUFFER);
		    if(!ok)
			return TRUE;
		    InBuffer.Buffer = (BYTE*) InputBuffer;
		    InBuffer.BufferSize = length;
		    OutBuffer.BufferSize = MAX_BUFFER;
		    OutBuffer.Buffer = (_OUTPUT_BUFFER) OutputBuffer;
		    // record the number of bytes in the command if it is the largest
		    // we have seen so far.
		    if(InBuffer.BufferSize > CommandResponseSizes.largestCommandSize)
			{
			    CommandResponseSizes.largestCommandSize = InBuffer.BufferSize;
			    memcpy(&CommandResponseSizes.largestCommand,
				   &InputBuffer[6], sizeof(UINT32));
			}
		    
		    _rpc__Send_Command(locality, InBuffer, &OutBuffer);
		    // record the number of bytes in the response if it is the largest
		    // we have seen so far.
		    if(OutBuffer.BufferSize > CommandResponseSizes.largestResponseSize)
			{
			    CommandResponseSizes.largestResponseSize
				= OutBuffer.BufferSize;
			    memcpy(&CommandResponseSizes.largestResponse,
				   &OutputBuffer[6], sizeof(UINT32));
			}
		    ok = WriteVarBytes(s,
				       (char*) OutBuffer.Buffer,
				       OutBuffer.BufferSize);
		    if(!ok)
			return TRUE;
		    break;
		    
		  case TPM_REMOTE_HANDSHAKE:
		    ok = ReadBytes(s, (char*)&clientVersion, 4);
		    if(!ok)
			return TRUE;
		    if( clientVersion == 0 )
			{
			    printf("Unsupported client version (0).\n");
			    return TRUE;
			}
		    ok &= WriteUINT32(s, ServerVersion);
		    ok &= WriteUINT32(s,
				      tpmInRawMode | tpmPlatformAvailable | tpmSupportsPP);
		    break;
		    
		  case TPM_SET_ALTERNATIVE_RESULT:
		    ok = ReadBytes(s, (char*)&result, 4);
		    if(!ok)
			return TRUE;
		    // Alternative result is not applicable to the simulator.
		    break;
		    
		  case TPM_SESSION_END:
		    // Client signaled end-of-session
		    return TRUE;
		    
		  case TPM_STOP:
		    // Client requested the simulator to exit
		    return FALSE;
		  default:
		    printf("Unrecognized TPM interface command %08x\n", Command);
		    return TRUE;
		}
	    ok = WriteUINT32(s,0);
	    if(!ok)
		return TRUE;
	}
}

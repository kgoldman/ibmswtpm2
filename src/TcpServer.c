/********************************************************************************/
/*										*/
/*		Socket Interface to a TPM Simulator    				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TcpServer.c 1658 2021-01-22 23:14:01Z kgoldman $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2021				*/
/*										*/
/********************************************************************************/

/* D.3 TcpServer.c */
/* D.3.1. Description */
/* This file contains the socket interface to a TPM simulator. */
/* D.3.2. Includes, Locals, Defines and Function Prototypes */
#include "TpmBuildSwitches.h"
#include <stdio.h>
#include <stdbool.h>

#ifdef TPM_WINDOWS
#include <windows.h>
#include <winsock.h>
typedef int socklen_t;
#endif
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "TpmTcpProtocol.h"
#include "Manufacture_fp.h"
#include "TpmProfile.h"
#include "Simulator_fp.h"
#include "TcpServer_fp.h"
#include "Platform_fp.h"
#include "PlatformACT_fp.h"		/* added kgold */

/*     To access key cache control in TPM */
void RsaKeyCacheControl(int state);
#ifndef __IGNORE_STATE__
static uint32_t ServerVersion = 1;
#define MAX_BUFFER 1048576
char InputBuffer[MAX_BUFFER];       //The input data buffer for the simulator.
char OutputBuffer[MAX_BUFFER];      //The output data buffer for the simulator.
struct
{
    uint32_t    largestCommandSize;
    uint32_t    largestCommand;
    uint32_t    largestResponseSize;
    uint32_t    largestResponse;
} CommandResponseSizes = {0};
#endif // __IGNORE_STATE___
/* D.3.3. Functions */
/* D.3.3.1. CreateSocket() */
/* This function creates a socket listening on PortNumber. */
static int
CreateSocket(
	     int                  PortNumber,
	     SOCKET              *listenSocket
	     )
{
    WSADATA              wsaData;
    struct               sockaddr_in MyAddress;
    int res;
    // Initialize Winsock
    res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(res != 0)
	{
	    printf("WSAStartup failed with error: %d\n", res);
	    return -1;
	}
    // create listening socket
    *listenSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(INVALID_SOCKET == *listenSocket)
	{
	    printf("Cannot create server listen socket.  Error is 0x%x\n",
		   WSAGetLastError());
	    return -1;
	}
    // bind the listening socket to the specified port
    ZeroMemory(&MyAddress, sizeof(MyAddress));
    MyAddress.sin_port = htons((short)PortNumber);
    MyAddress.sin_family = AF_INET;
    res = bind(*listenSocket, (struct sockaddr*) &MyAddress, sizeof(MyAddress));
    if(res == SOCKET_ERROR)
	{
	    printf("Bind error.  Error is 0x%x\n", WSAGetLastError());
	    return -1;
	}
    // listen/wait for server connections
    res = listen(*listenSocket, 3);
    if(res == SOCKET_ERROR)
	{
	    printf("Listen error.  Error is 0x%x\n", WSAGetLastError());
	    return -1;
	}
    return 0;
}
/* D.3.3.2. PlatformServer() */
/* This function processes incoming platform requests. */
bool
PlatformServer(
	       SOCKET           s
	       )
{
    bool                 OK = true;
    uint32_t             Command;

    for(;;)
	{
	    OK = ReadBytes(s, (char*)&Command, 4);
	    // client disconnected (or other error).  We stop processing this client
	    // and return to our caller who can stop the server or listen for another
	    // connection.
	    if(!OK) return true;
	    Command = ntohl(Command);
	    switch(Command)
		{
		  case TPM_SIGNAL_POWER_ON:
		    _rpc__Signal_PowerOn(false);
		    break;
		  case TPM_SIGNAL_POWER_OFF:
		    _rpc__Signal_PowerOff();
		    break;
		  case TPM_SIGNAL_RESET:
		    _rpc__Signal_PowerOn(true);
		    break;
		  case TPM_SIGNAL_RESTART:
		    _rpc__Signal_Restart();
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
		  case TPM_SIGNAL_KEY_CACHE_ON:
		    _rpc__RsaKeyCacheControl(true);
		    break;
		  case TPM_SIGNAL_KEY_CACHE_OFF:
		    _rpc__RsaKeyCacheControl(false);
		    break;
		  case TPM_SESSION_END:
		    // Client signaled end-of-session
		    TpmEndSimulation();
		    return true;
		  case TPM_STOP:
		    // Client requested the simulator to exit
		    return false;
		  case TPM_TEST_FAILURE_MODE:
		    _rpc__ForceFailureMode();
		    break;
		  case TPM_GET_COMMAND_RESPONSE_SIZES:
		    OK = WriteVarBytes(s, (char *)&CommandResponseSizes,
				       sizeof(CommandResponseSizes));
		    memset(&CommandResponseSizes, 0, sizeof(CommandResponseSizes));
		    if(!OK)
			return true;
		    break;
		  case TPM_ACT_GET_SIGNALED:
		      {
			  uint32_t actHandle;
			  OK = ReadUINT32(s, &actHandle);
			  WriteUINT32(s, _rpc__ACT_GetSignaled(actHandle));
			  break;
		      }
		  default:
		    printf("Unrecognized platform interface command %d\n",
			   (int)Command);
		    WriteUINT32(s, 1);
		    return true;
		}
	    WriteUINT32(s, 0);
	}
    return false;
}
/* D.3.3.3. PlatformSvcRoutine() */
/* This function is called to set up the socket interfaces to listen for commands. */
DWORD WINAPI
PlatformSvcRoutine(
		   void *port
		   )
{
    int                  PortNumber = *(int *)port;
    SOCKET               listenSocket, serverSocket;
    struct               sockaddr_in HerAddress;
    int                  res;
    socklen_t            length;
    bool                 continueServing;
    res = CreateSocket(PortNumber, &listenSocket);
    if(res != 0)
	{
	    printf("Create platform service socket fail\n");
	    return res;
	}
    // Loop accepting connections one-by-one until we are killed or asked to stop
    // Note the platform service is single-threaded so we don't listen for a new
    // connection until the prior connection drops.
    do
	{
	    printf("Platform server listening on port %d\n", PortNumber);
	    // blocking accept
	    length = sizeof(HerAddress);
	    serverSocket = accept(listenSocket,
				  (struct sockaddr*) &HerAddress,
				  &length);
	    if(serverSocket == INVALID_SOCKET)
		{
		    printf("Accept error.  Error is 0x%x\n", WSAGetLastError());
		    return (DWORD)-1;
		}
	    printf("Client accepted\n");
	    // normal behavior on client disconnection is to wait for a new client
	    // to connect
	    continueServing = PlatformServer(serverSocket);
	    closesocket(serverSocket);
	} while(continueServing);
    return 0;
}
/* D.3.3.4. PlatformSignalService() */
/* This function starts a new thread waiting for platform signals. Platform signals are processed
   one at a time in the order in which they are received. */
int
PlatformSignalService(
		      int              *PortNumberPlatform
		      )
{
    HANDLE               hPlatformSvc;
    int                  ThreadId;

    // Create service thread for platform signals
    hPlatformSvc = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)PlatformSvcRoutine,
				(void *)PortNumberPlatform, 0, (LPDWORD)&ThreadId);
    if(hPlatformSvc == NULL)
	{
	    printf("Thread Creation failed\n");
	    return -1;
	}
    return 0;
}
/* D.3.3.5. RegularCommandService() */
/* This function services regular commands. */
int
RegularCommandService(
		      int              *PortNumber
		      )
{
    SOCKET               listenSocket;
    SOCKET               serverSocket;
    struct               sockaddr_in HerAddress;
    int 		 res;
    socklen_t 		 length;
    bool 		 continueServing;
    res = CreateSocket(*PortNumber, &listenSocket);
    if(res != 0)
	{
	    printf("Create platform service socket fail\n");
	    return res;
	}
    // Loop accepting connections one-by-one until we are killed or asked to stop
    // Note the TPM command service is single-threaded so we don't listen for
    // a new connection until the prior connection drops.
    do
	{
	    printf("TPM command server listening on port %d\n", *PortNumber);
	    // blocking accept
	    length = sizeof(HerAddress);
	    serverSocket = accept(listenSocket,
				  (struct sockaddr*) &HerAddress,
				  &length);
	    if(serverSocket == INVALID_SOCKET)
		{
		    printf("Accept error.  Error is 0x%x\n", WSAGetLastError());
		    return -1;
		}
	    printf("Client accepted\n");
	    // normal behavior on client disconnection is to wait for a new client
	    // to connect
	    continueServing = TpmServer(serverSocket);
	    closesocket(serverSocket);
	} while(continueServing);
    return 0;
}

#ifdef RH_ACT_0

/* D.3.2.5.	SimulatorTimeServiceRoutine() */
/* This function is called to service the time ticks. */
static unsigned long WINAPI
SimulatorTimeServiceRoutine(
			    LPVOID           notUsed
			    )
{
    // All time is in ms
    const int64_t   tick = 1000;
    uint64_t prevTime = _plat__RealTime();
    int64_t   timeout = tick;

    (void)notUsed;

    while (true)
	{
	    uint64_t  curTime;

#if defined(TPM_WINDOWS)
	    Sleep((DWORD)timeout);
#else
	    struct timespec req = {timeout / 1000, (timeout % 1000) * 1000};
	    struct timespec rem;
	    nanosleep(&req, &rem);
#endif // _MSC_VER
	    curTime = _plat__RealTime();

	    // May need to issue several ticks if the Sleep() took longer than asked,
	    // or no ticks at all, it Sleep() was interrupted prematurely.
	    while (prevTime < curTime - tick / 2)
	        {
	            //printf("%05lld | %05lld\n",
	            //      prevTime % 100000, (curTime - tick / 2) % 100000);
	            _plat__ACT_Tick();
	            prevTime += (uint64_t)tick;
	        }
	    // Adjust the next timeout to keep the average interval of one second
	    timeout = tick + (prevTime - curTime);
	    //prevTime = curTime;
	    //printf("%04lld | c:%05lld | p:%05llu\n",
	    //          timeout, curTime % 100000, prevTime);
	}
    return 0;
}

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
    static bool          running = false;
    int                  ret = 0;
    if(!running)
	{
#if defined(TPM_WINDOWS)
	    HANDLE               hThr;
	    int                  ThreadId;
	    //
	    printf("Starting ACT thread...\n");
	    //  Don't allow ticks to be processed before TPM is manufactured.
	    _plat__ACT_EnableTicks(false);

	    // Create service thread for ACT internal timer
	    hThr = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)SimulatorTimeServiceRoutine,
				(LPVOID)(INT_PTR)NULL, 0, (LPDWORD)&ThreadId);
	    if(hThr != NULL)
		CloseHandle(hThr);
	    else
		ret = -1;
#else
	    pthread_t            thread_id;
	    //
	    ret = pthread_create(&thread_id, NULL, (void*)SimulatorTimeServiceRoutine,
				 NULL);
#endif // TPM_WINDOWS

	    if(ret != 0)
		printf("ACT thread Creation failed\n");
	    else
		running = true;
	}
    return ret;
}
#endif // RH_ACT_0

/* D.3.3.6. StartTcpServer() */
/* Main entry-point to the TCP server.  The server listens on port specified. Note that there is no
   way to specify the network interface in this implementation. */
int
StartTcpServer(
	       int              *PortNumber,
	       int              *PortNumberPlatform
	       )
{
    int                  res;
#ifdef RH_ACT_0
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
    if(res != 0)
	{
	    printf("PlatformSignalService failed\n");
	    return res;
	}
    // Start Regular/DRTM TPM command service
    res = RegularCommandService(PortNumber);
    if(res != 0)
	{
	    printf("RegularCommandService failed\n");
	    return res;
	}
    return 0;
}
/* D.3.3.7. ReadBytes() */
/* This function reads the indicated number of bytes (NumBytes) into buffer from the indicated
   socket. */
bool
ReadBytes(
	  SOCKET           s,
	  char            *buffer,
	  int              NumBytes
	  )
{
    int                  res;
    int                  numGot = 0;

    while(numGot < NumBytes)
	{
	    res = recv(s, buffer + numGot, NumBytes - numGot, 0);
	    if(res == -1)
		{
		    printf("Receive error.  Error is 0x%x\n", WSAGetLastError());
		    return false;
		}
	    if(res == 0)
		{
		    return false;
		}
	    numGot += res;
	}
    return true;
}
/* D.3.3.8. WriteBytes() */
/* This function will send the indicated number of bytes (NumBytes) to the indicated socket */
bool
WriteBytes(
	   SOCKET           s,
	   char            *buffer,
	   int              NumBytes
	   )
{
    int                  res;
    int                  numSent = 0;
    while(numSent < NumBytes)
	{
	    res = send(s, buffer + numSent, NumBytes - numSent, 0);
	    if(res == -1)
		{
		    if(WSAGetLastError() == 0x2745)
			{
			    printf("Client disconnected\n");
			}
		    else
			{
			    printf("Send error.  Error is 0x%x\n", WSAGetLastError());
			}
		    return false;
		}
	    numSent += res;
	}
    return true;
}
/* D.3.3.9. WriteUINT32() */
/* Send 4 byte integer */

bool
WriteUINT32(
	    SOCKET           s,
	    uint32_t         val
	    )
{
    uint32_t netVal = htonl(val);
    return WriteBytes(s, (char*)&netVal, 4);
}

/* D.3.2.11.	ReadUINT32() */
/* Function to read 4 byte integer from socket. */

bool
ReadUINT32(
	   SOCKET           s,
	   uint32_t         *val
	   )
{
    uint32_t netVal;
    //
    if (!ReadBytes(s, (char*)&netVal, 4))
	return false;
    *val = ntohl(netVal);
    return true;
}

/* D.3.3.10. ReadVarBytes() */
/* Get a uint32_t-length-prepended binary array.  Note that the 4-byte length is in network byte order
   (big-endian). */

bool
ReadVarBytes(
	     SOCKET           s,
	     char            *buffer,
	     uint32_t        *BytesReceived,
	     int              MaxLen
	     )
{
    int                  length;
    bool                 res;

    res = ReadBytes(s, (char*)&length, 4);
    if(!res) return res;
    length = ntohl(length);
    *BytesReceived = length;
    if(length > MaxLen)
	{
	    printf("Buffer too big.  Client says %d\n", length);
	    return false;
	}
    if(length == 0) return true;
    res = ReadBytes(s, buffer, length);
    if(!res) return res;
    return true;
}
/* D.3.3.11. WriteVarBytes() */
/* Send a uint32_t-length-prepended binary array.  Note that the 4-byte length is in network byte
   order (big-endian). */
bool
WriteVarBytes(
	      SOCKET           s,
	      char            *buffer,
	      int              BytesToSend
	      )
{
    uint32_t             netLength = htonl(BytesToSend);
    bool res;
    res = WriteBytes(s, (char*)&netLength, 4);
    if(!res) return res;
    res = WriteBytes(s, buffer, BytesToSend);
    if(!res) return res;
    return true;
}
/* D.3.3.12. TpmServer() */
/* Processing incoming TPM command requests using the protocol / interface defined above. */

bool
TpmServer(
	  SOCKET           s
	  )
{
    uint32_t             length;
    uint32_t             Command;
    uint8_t              locality;
    bool                 OK;
    int                  result;
    int                  clientVersion;
    _IN_BUFFER           InBuffer;
    _OUT_BUFFER          OutBuffer;
    for(;;)
	{
	    OK = ReadBytes(s, (char*)&Command, 4);
	    // client disconnected (or other error).  We stop processing this client
	    // and return to our caller who can stop the server or listen for another
	    // connection.
	    if(!OK)
		return true;
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
		    OK = ReadVarBytes(s, InputBuffer, &length, MAX_BUFFER);
		    if(!OK) return true;
		    InBuffer.Buffer = (uint8_t *)InputBuffer;
		    InBuffer.BufferSize = length;
		    _rpc__Signal_Hash_Data(InBuffer);
		    break;
		  case TPM_SEND_COMMAND:
		    OK = ReadBytes(s, (char*)&locality, 1);
		    if(!OK)
			return true;
		    OK = ReadVarBytes(s, InputBuffer, &length, MAX_BUFFER);
		    if(!OK)
			return true;
		    InBuffer.Buffer = (uint8_t *)InputBuffer;
		    InBuffer.BufferSize = length;
		    OutBuffer.BufferSize = MAX_BUFFER;
		    OutBuffer.Buffer = (_OUTPUT_BUFFER)OutputBuffer;
		    // record the number of bytes in the command if it is the largest
		    // we have seen so far.
		    if(InBuffer.BufferSize > CommandResponseSizes.largestCommandSize)
			{
			    CommandResponseSizes.largestCommandSize = InBuffer.BufferSize;
			    memcpy(&CommandResponseSizes.largestCommand,
				   &InputBuffer[6], sizeof(uint32_t));
			}
		    _rpc__Send_Command(locality, InBuffer, &OutBuffer);
		    // record the number of bytes in the response if it is the largest
		    // we have seen so far.
		    if(OutBuffer.BufferSize > CommandResponseSizes.largestResponseSize)
			{
			    CommandResponseSizes.largestResponseSize
				= OutBuffer.BufferSize;
			    memcpy(&CommandResponseSizes.largestResponse,
				   &OutputBuffer[6], sizeof(uint32_t));
			}
		    OK = WriteVarBytes(s,
				       (char*)OutBuffer.Buffer,
				       OutBuffer.BufferSize);
		    if(!OK)
			return true;
		    break;
		  case TPM_REMOTE_HANDSHAKE:
		    OK = ReadBytes(s, (char*)&clientVersion, 4);
		    if(!OK)
			return true;
		    if(clientVersion == 0)
			{
			    printf("Unsupported client version (0).\n");
			    return true;
			}
		    OK &= WriteUINT32(s, ServerVersion);
		    OK &= WriteUINT32(s, tpmInRawMode
				      | tpmPlatformAvailable | tpmSupportsPP);
		    break;
		  case TPM_SET_ALTERNATIVE_RESULT:
		    OK = ReadBytes(s, (char*)&result, 4);
		    if(!OK)
			return true;
		    // Alternative result is not applicable to the simulator.
		    break;
		  case TPM_SESSION_END:
		    // Client signaled end-of-session
		    return true;
		  case TPM_STOP:
		    // Client requested the simulator to exit
		    return false;
		  default:
		    printf("Unrecognized TPM interface command %d\n", (int)Command);
		    return true;
		}
	    OK = WriteUINT32(s, 0);
	    if(!OK)
		return true;
	}
}

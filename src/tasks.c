//#include <ti/ndk/inc/netmain.h>
#include <ti/csl/tistdtypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
//#include <ti/ndk/inc/socket.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>
#include "app4Core0.h"

#define URL_ITEM_LEN (100)

extern Semaphore_Handle g_readSemaphore;
extern Semaphore_Handle g_writeSemaphore;
extern Semaphore_Handle httptodpmSemaphore;
extern Semaphore_Handle gSendSemaphore;

//#pragma DATA_SECTION(g_outBuffer,".WtSpace");
unsigned char g_outBuffer[0x00e00000]; //4M-->max size=500M

//------------------------------------------------------------------
//2	M
#define inBufSize (0x00200000)
// 512K*4*7=0x00e00000
#pragma DATA_SECTION(coreNInBuf,".coreNInBuf");
unsigned char coreNInBuf[0x00e00000];
unsigned char *pCore1InBuf=coreNInBuf;
unsigned char *pCore2InBuf=coreNInBuf+inBufSize;
unsigned char *pCore3InBuf=coreNInBuf+(inBufSize*2);
unsigned char *pCore4InBuf=coreNInBuf+(inBufSize*3);
unsigned char *pCore5InBuf=coreNInBuf+(inBufSize*4);
unsigned char *pCore6InBuf=coreNInBuf+(inBufSize*5);
unsigned char *pCore7InBuf=coreNInBuf+(inBufSize*6);


#define OutBufSize (0x01C00000)
// 7M*4*7=196M
#pragma DATA_SECTION(coreNOutBuf,".coreNOutBuf");
unsigned char coreNOutBuf[0x0c400000];
unsigned char *pCore1OutBuf=coreNOutBuf;
unsigned char *pCore2OutBuf=coreNOutBuf+OutBufSize;
unsigned char *pCore3OutBuf=coreNOutBuf+(OutBufSize*2);
unsigned char *pCore4OutBuf=coreNOutBuf+(OutBufSize*3);
unsigned char *pCore5OutBuf=coreNOutBuf+(OutBufSize*4);
unsigned char *pCore6OutBuf=coreNOutBuf+(OutBufSize*5);
unsigned char *pCore7OutBuf=coreNOutBuf+(OutBufSize*6);


uint32_t *g_pReceiveBuffer = (uint32_t *) (C6678_PCIEDATA_BASE + 2 * 4 * 1024);
extern void write_uart(char* msg);
int pollValue(uint32_t * pAddress, uint32_t pollVal, uint32_t maxPollCount)
{
	int retVal = 0;
	uint32_t loopCount = 0;
	uint32_t stopPoll = 0;
	uint32_t realTimeVal = 0;
	//char debugInfor[100];

	for (loopCount = 0; ((loopCount < maxPollCount) && (stopPoll == 0));
			loopCount++)
	{
		realTimeVal = DEVICE_REG32_R(pAddress);
		//realTimeVal = *pAddress;
		if (realTimeVal & pollVal)
		{
			stopPoll = 1;
		}
		else
		{
		}
	}
	if (loopCount < maxPollCount)
	{
		retVal = 0;
	}
	else
	{
		retVal = -1;
	}

	return (retVal);
}
int pollEqualValue(uint32_t * pAddress, uint32_t pollVal, uint32_t maxPollCount)
{
	int retVal = 0;
	uint32_t loopCount = 0;
	uint32_t stopPoll = 0;
	uint32_t realTimeVal = 0;
	//char debugInfor[100];

	for (loopCount = 0; ((loopCount < maxPollCount) && (stopPoll == 0));
			loopCount++)
	{
		realTimeVal = DEVICE_REG32_R(pAddress);
		//realTimeVal = *pAddress;
		if (realTimeVal == pollVal)
		{
			stopPoll = 1;
		}
		else
		{
		}
	}
	if (loopCount < maxPollCount)
	{
		retVal = 0;
	}
	else
	{
		retVal = -1;
	}

	return (retVal);
}
int pollZero(uint32_t * pAddress, uint32_t pollVal, uint32_t maxPollCount)
{
	int retVal = 0;
	uint32_t loopCount = 0;
	uint32_t stopPoll = 0;
	uint32_t realTimeVal = 0;
	char debugInfor[100];

	for (loopCount = 0; (loopCount < maxPollCount) && (stopPoll == 0);
			loopCount++)
	{
		realTimeVal = DEVICE_REG32_R(pAddress);
		//realTimeVal = *pAddress;
		if ((realTimeVal - pollVal < 0.0000001)
				|| (pollVal - realTimeVal < 0.0000001))
		//if (realTimeVal == pollVal)
		{
			stopPoll = 1;
		}
		else
		{
		}
	}
	if (loopCount < maxPollCount)
	{
		retVal = 0;
	}
	else
	{
		retVal = -1;
	}
	sprintf(debugInfor, "loopCount=%d\r\n", loopCount);
	write_uart(debugInfor);

	return (retVal);
}





int getPicTask()
{
	int retVal = 0;

	uint32_t picCount = 0;

	char debugBuf[200];

	int urlIndex = 0;

	uint8_t *pUrlAddr = NULL;

	registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;

	write_uart("getPicTask\n\r");

	/*
	 g_outBuffer=(unsigned char *)malloc(0x00600000*sizeof(char));
	 if(g_outBuffer!=NULL)
	 {
	 write_uart("alloc the g_outBuffer finished\r\n");
	 }
	 else
	 {
	 write_uart("alloc the g_outBuffer error\r\n");
	 return(0);
	 }
	 pOutbuffer=pPicDestAddr=(uint32_t *)g_outBuffer;
	 //memset(&gPictureInfor, 0, sizeof(PicInfor));
	 */
#if 1

	//dsp init ready and can read urls.
	//*((uint32_t *) (PCIE_EP_IRQ_SET)) = 0x1;
	/********************************************DSP read and pc write*/
	{
		pRegisterTable->readControl = DSP_RD_RESET;
		retVal = pollEqualValue(&(pRegisterTable->readStatus), PC_WT_FINISH,
				0x07ffffff);
		if (-1 == retVal)
		{
			write_uart("wait pc write finished timeout\r\n");
			return (retVal);
		}
		// read;
		{
			write_uart("read begin\r\n");
			pUrlAddr = (uint8_t *) g_pReceiveBuffer;
			picCount = DEVICE_REG32_R(&(pRegisterTable->DSP_urlNumsReg));
			sprintf(debugBuf, "urlItemNum=%d\n\r",
					pRegisterTable->DSP_urlNumsReg);
			write_uart(debugBuf);

			//memcpy(core1InBuf,pUrlAddr,0x100000);
			while (urlIndex < picCount)
			{
				//read the length;
				retVal = *(int *) pUrlAddr;
				pUrlAddr += sizeof(int);
				//todo read jpeg
				{
				};
				pUrlAddr += ((retVal + 3) / 4) * 4;
				urlIndex++;
				sprintf(debugBuf, "picLength=%d\n\r", retVal);
				write_uart(debugBuf);
			}
			write_uart("read finish\n\r");

		}
		pRegisterTable->readControl = DSP_RD_FINISH;
		write_uart("wait pc reset\n\r");
		retVal = pollEqualValue(&(pRegisterTable->readStatus), PC_WT_RESET,
				0x07ffffff);
		if (-1 == retVal)
		{
			write_uart("wait pc write reset timeout\n\r");
			return (retVal);
		}
		pRegisterTable->readControl = DSP_RD_RESET;
		write_uart("getPicTask post g_writeSemaphore\n\r");

		// distributePicToCoreN();
		memcpy(pCore1InBuf,g_pReceiveBuffer,(inBufSize/2));
		// todo wait core1 writeOver. in the isrHandle while judge
		//interrupt2CoreN();
		write_uart("triggle INT to core1\n\r");
		DEVICE_REG32_W(IPC_INT_ADDR(1), 1);
		//Semaphore_pend()
	}
#if 0
	/********************************************DSP write and pc read*/
	{
		pRegisterTable->writeControl = DSP_WT_RESET;
		retVal = pollValue(&(pRegisterTable->writeStatus), PC_RD_RESET,
				0x07ffffff);
		//write()
		{

		}
		pRegisterTable->writeControl = DSP_WT_FINISH;
		retVal = pollValue(&(pRegisterTable->writeStatus), PC_RD_FIN,
				0x07ffffff);
		pRegisterTable->writeControl = DSP_WT_RESET;

	}
	/***************************************************************/
#endif
#if 0
	while (1 == g_DownloadFlags)
	{
		//5.13
		pRegisterTable->dpmOverControl = 0x00000000;
		// polling PC to write the url.
		retVal = pollValue(&(pRegisterTable->readStatus), DSP_RD_READY,
				0x07ffffff);
		if (retVal == 0)
		{
			picCount = DEVICE_REG32_R(&(pRegisterTable->DSP_urlNumsReg));
			sprintf(debugBuf, "urlItemNum=%d\n\r",
					pRegisterTable->DSP_urlNumsReg);
			write_uart(debugBuf);
			pUrlAddr = g_pReceiveBuffer;
			pPicDestAddr = pOutbuffer;
			// polling the PC can be written to.
			retVal = pollValue(&(pRegisterTable->writeStatus), DSP_WT_INIT,
					0x07ffffff);

		}
		else
		{
			retVal = -1;
			sprintf(debugBuf,
					"wait url time out. readStatus=0x%x,retVal=%d,urlItemNum=%d\n\r",
					pRegisterTable->readStatus, retVal, picCount);
			write_uart(debugBuf);
			picCount = 0;
			//timeout
			Semaphore_pend(g_readSemaphore, BIOS_WAIT_FOREVER);
			picCount = DEVICE_REG32_R(&(pRegisterTable->DSP_urlNumsReg));
			sprintf(debugBuf, "urlItemNum=%d\n\r",
					pRegisterTable->DSP_urlNumsReg);
			write_uart(debugBuf);
			pUrlAddr = g_pReceiveBuffer;
			pPicDestAddr = pOutbuffer;
			// polling the PC can be written to.
			retVal = pollValue(&(pRegisterTable->writeStatus), DSP_WT_INIT,
					0x07ffffff);
		}

		// polling PC can be writed to.
		if (retVal == 0)
		{

		}
		else
		{
			retVal = -2;
			sprintf(debugBuf,
					"wait the pc be written.time over.writeStatus=0x%x\n\r",
					pRegisterTable->writeStatus);
			write_uart(debugBuf);
			picCount = 0;
		}

		write_uart("start download the pci loop\n");
		p_gPictureInfor->picNums = 0;
		picNum = 0;
		// start the down load loop.
		while (picCount > 0)
		{
			// declare the socket.

			//SOCKET socket_handle = INVALID_SOCKET;
			// get the url.
			memset(urlBuffer, 0, URL_ITEM_LEN);
			memcpy(urlBuffer, pUrlAddr, URL_ITEM_LEN);
			ptrUrl = urlBuffer;
			memcpy(inrequest, ptrUrl, (strlen(ptrUrl) + 1));

		}

		// parse the url.
		retVal = http_parseURL(inrequest, &url_infor);

		// save the pic name.
		strcpy(p_gPictureInfor->picName[picNum], url_infor.p_fileName);
		sprintf(debugBuf, "the pic name is %s\n\r",
				p_gPictureInfor->picName[picNum]);
		write_uart(debugBuf);

		if (retVal == 0)
		{

			bzero(&socket_address, sizeof(struct sockaddr_in));
			socket_address.sin_family = AF_INET;
			socket_address.sin_len = sizeof(socket_address);
			socket_address.sin_addr.s_addr = inet_addr(url_infor.p_host_ip);
			socket_address.sin_port = htons(url_infor.n_port);
			// TODO: add the create socket code segment.

		}
		else
		{
			retVal = -3;
			sprintf(debugBuf, "url parse error. retVal=%d\n\r", retVal);
			write_uart(debugBuf);
		}
		// create socket.
		if (retVal == 0)
		{
			socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (socket_handle != INVALID_SOCKET)
			{

			}
			else
			{
				retVal = -4;
				socketErrorCode = fdError();
				sprintf(debugBuf, "socket error. errorCode=%d\n\r",
						socketErrorCode);
				write_uart(debugBuf);
			}
		}
#if 0
		// set the socket opt
		if (retVal == 0)
		{
			retVal = setsockopt(socket_handle, SOL_SOCKET, SO_BLOCKING,
					&socketOptFlag, sizeof(socketOptFlag));
			{
				if (retVal == 0)
				{
				}
				else
				{
					socketErrorCode = fdError();
					sprintf(debugBuf, "setsockopt error. errorCode=%d\n\r",
							socketErrorCode);
					write_uart(debugBuf);
					retVal = -5;
				}
			}
		}
#endif
		// connect.
		if (retVal == 0)
		{
			retConnect = connect(socket_handle, (PSA) &socket_address,
					sizeof(socket_address));
			if (retConnect != -1)
			{
				write_uart("connect successful\n\r");
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "connect error. errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
			}
		}

		// generate the head requet.
		if (retVal == 0)
		{
			sprintf(headRequest,
					"HEAD %s HTTP/1.1\r\nHOST:%s\r\nAccept:*/*\r\n\r\n",
					url_infor.p_input_url, url_infor.p_host_ip);
		}

		// send the head request.
		if (retVal == 0)
		{
			timeStart = llTimerGetTime(0);
			sendBufferLength = strlen(headRequest);
			write_uart("send the head request start\r\n");
			//retVal = httpSendRequest(socket_handle, headRequest,&sendBufferLength, 0);
			retRecv = send(socket_handle, headRequest, sendBufferLength, 0);
			if (retRecv == sendBufferLength)
			{
				write_uart("send head request successful\r\n");
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "send error. errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -7;
			}
		}

		// recv the headInfo.
		if (retVal == 0)
		{
			write_uart("recv the head start\r\n");
			//retVal = httpRecvHead(socket_handle, pHttpHeadbuffer,&recvHttpHeadLength);
			retVal = httpRecvHeadTemp(socket_handle, pHttpHeadbuffer,
					&recvHttpHeadLength);
			if (retVal == 0)
			{
				pHttpHeadbuffer[recvHttpHeadLength] = '\0';
				write_uart(pHttpHeadbuffer);
				write_uart("\r\n");
				write_uart("recv head successfull\r\n");
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "head recv error. errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -8;
			}
		}

		// parse the http head info.save the picLength.
		if (retVal == 0)
		{
			write_uart("parse http head start.\r\n");
			i = 1;
			pContentLength = strstr(pHttpHeadbuffer, "Content-Length:");
			if (pContentLength != NULL)
			{
				pContentLength += strlen("Content-Length:");
				while (*(pContentLength + i) == ' ')
				++i;
				nContentLength = atoi(pContentLength + i);
				// NOTE: nContentLength is the pictureLength;
				// NOTE: recvHttpHeadLength is the headLength;
				mmCopy(pPicDestAddr, &nContentLength, sizeof(int));
				pPicBuffer = (((uint8_t *) (pPicDestAddr)) + sizeof(int));
				//pPicBuffer = (uint8_t *) pPicDestAddr;
				recvHttpGetLength = nContentLength + recvHttpHeadLength;
				sprintf(debugBuf,
						"content-length=%d,pPicBuffer address:%x\r\n",
						nContentLength, pPicBuffer);
				write_uart(debugBuf);
			}
			else
			{
				retVal = -9;
				write_uart("error:Content-Length\n");
			}
		}

		// generate the http get request.
		if (retVal == 0)
		{
			sprintf(getRequest,
					"GET %s HTTP/1.1\r\nHOST:%s\r\nAccept:*/*\r\n\r\n",
					url_infor.p_input_url, url_infor.p_host_ip);
		}
		else
		{
		}

		// send the http get request
		if (retVal == 0)
		{
			write_uart("send the get start\r\n");
			sendBufferLength = strlen(getRequest);
			retRecv = send(socket_handle, getRequest, sendBufferLength, 0);
			if (retRecv == sendBufferLength)
			{
				write_uart("send get request successful\r\n");
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "get send error. errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -10;
			}
		}
		else
		{
		}

		// recv picture data.
		if (retVal == 0)
		{
			write_uart("recv the get start\r\n");
			retVal = httpRecvGetTemp(socket_handle, pHttpGetbuffer,
					&recvHttpGetLength, 0);
			if (retVal == 0)
			{
				// NOTE: Get the picture data.
				memcpy(pPicBuffer, (pHttpGetbuffer + recvHttpHeadLength),
						nContentLength);
				write_uart("get get request successful\r\n");
				timeEnd = llTimerGetTime(0);
				sprintf(debugBuf, "body recv,elapseTime=%d\r\n",
						(timeEnd - timeStart));
				write_uart(debugBuf);
				sprintf(debugBuf, "getRecvBytes=%d\r\n", recvHttpGetLength);
				write_uart(debugBuf);
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "get recv error. errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
			}
		}

#if 0
		// generate request and send.
		if (retVal == 0)
		{
			// TODO :improve the set_http_package.
			// fixed the bug.
			set_http_package(&url_infor, request);
			//pHttpRequest = (char *) mmAlloc(strlen(request) + 1);
			//strcpy(pHttpRequest, request);

			retSend = send(socket_handle, request, strlen(request),
					MSG_WAITALL);
			if (retSend == strlen(request))
			{
				sprintf(debugBuf,
						"send http request fininshed retSend=%d\r\n",
						retSend);
				write_uart(debugBuf);
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "retSend=%d,send error. errorCode=%d\r\n",
						retSend, socketErrorCode);
				write_uart(debugBuf);
				retVal = -6;
			}
			//mmFree(pHttpRequest);
		}

		// recv
		if (retVal == 0)
		{
			nTotalPicBytes = 1024;
			nRestPicBytes = nTotalPicBytes;
			retRecv = 0;
			memset(pbuffer, 0, 1024);
			write_uart("head data recv start.\r\n");
			nCountRecv = 1;
			while ((nRestPicBytes > 0) && (nCountRecv > 0))
			{
				nCountRecv = recv(socket_handle,
						pbuffer + (nTotalPicBytes - nRestPicBytes),
						nRestPicBytes, 0);
				nRestPicBytes -= nCountRecv;
				retRecv += nCountRecv;
			}
			if (retRecv == 1024)
			{
				write_uart("head data recv finished.\r\n");
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "head recv error. errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -7;
			}
		}

		// parse the head.
		if (retVal == 0)
		{
			write_uart("parse http head start.\r\n");
			pContentLength = strstr(pbuffer, "Content-Length:");
			if (pContentLength != NULL)
			{
				pContentLength += strlen("Content-Length:");
				while (*(pContentLength + i) == ' ')
				++i;
				nContentLength = atoi(pContentLength + i);
				mmCopy(pPicDestAddr, (char *) &nContentLength, sizeof(int));
				pPicBuffer = (((uint8_t *) (pPicDestAddr)) + sizeof(int));
				nTotalPicBytes = nContentLength;

				pContentStart = strstr(pbuffer, "\r\n\r\n");
			}
			else
			{
				retVal = -8;
				write_uart("error:Content-Length\n");
			}
		}

		if ((retVal == 0) && (pContentStart != NULL))
		{
			pContentStart += strlen("\r\n\r\n");
			nContentStart = (int) (pContentStart - pbuffer);
			mmCopy(pPicBuffer, pContentStart, (retRecv - nContentStart));
			nRecvPicBytes = retRecv - nContentStart;
			nRestPicBytes = nContentLength - nRecvPicBytes;
			write_uart("parse http head finished.\r\n");
		}
		else if (pContentStart == NULL)
		{
			retVal = -9;
			write_uart("error:can't find the picture start symbols\r\n");
		}
		// recv.
		if (retVal == 0)
		{
			write_uart("pci data recv start.\r\n");
			timeStart = llTimerGetTime(0);
			nCountRecv = 1;

			while ((nRestPicBytes > 0) && (nCountRecv > 0))
			{
				nCountRecv = recv(socket_handle,
						pPicBuffer + (nTotalPicBytes - nRestPicBytes),
						nRestPicBytes, 0);
				nRestPicBytes -= nCountRecv;
				nRecvPicBytes += nCountRecv;
			}
			if (nRecvPicBytes == nTotalPicBytes)
			{
				timeEnd = llTimerGetTime(0);
				sprintf(debugBuf, "body recv,elapseTime=%d\r\n",
						(timeEnd - timeStart));
				write_uart(debugBuf);
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "body recv error. errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -10;
			}
		}
#endif
		// down load successful.
		if (retVal == 0)
		{

			p_gPictureInfor->picAddr[picNum] = (uint8_t *) pPicDestAddr;
			p_gPictureInfor->picLength[picNum] = nContentLength;
			memcpy(p_gPictureInfor->picUrls[picNum], inrequest,
					URL_ITEM_LEN);
			p_gPictureInfor->picNums++;

			sprintf(debugBuf,
					"download fileName:%s,fileLength=%d bytes,picNum=%dth,p_gPictureInfor->picNums=%d,pRegisterTable->DSP_urlNumsReg=%d\r\n",
					url_infor.p_fileName, nContentLength, picNum,
					p_gPictureInfor->picNums,
					pRegisterTable->DSP_urlNumsReg);
			write_uart(debugBuf);
			downLoadPicNum++;
			picNum++;
		} // down load failed.
		else if (retVal < -2)
		{
			p_gPictureInfor->picAddr[picNum] = NULL;
			p_gPictureInfor->picLength[picNum] = 0;
			memcpy(p_gPictureInfor->picUrls[picNum], inrequest,
					URL_ITEM_LEN);
			p_gPictureInfor->picNums++;

			downloadFail++;
			sprintf(debugBuf, "download %s fail retVal=%d\r\n", inrequest,
					retVal);
			write_uart(debugBuf);
			nContentLength = 0;
			picNum++;
		}

		// update the src and dest,urlItemNum.
		pPicDestAddr = (uint32_t *) ((uint8_t *) (pPicDestAddr)
				+ nContentLength + sizeof(int));
		//pPicDestAddr = (pPicDestAddr + (nContentLength + 3) / 4);
		//pPicDestAddr = pPicDestAddr + nContentLength ;
		pUrlAddr = (pUrlAddr + URL_ITEM_LEN / 4);

		picCount--;

		//close socket.
		//fdClose(socket_handle);

	} //while

	// if download haven started.dsp write to pc over.
	if ((downLoadPicNum + downloadFail) > 0)
	{
		pRegisterTable->getPicNumers = downLoadPicNum;
		pRegisterTable->failPicNumers = downloadFail;
		sprintf(debugBuf, "download %d pcitures,failed %d\r\n\r\n",
				downLoadPicNum, downloadFail);
		write_uart(debugBuf);
		// dsp write to pc over. Note: PC polling this for reading pc's inBuffer or not.
		pRegisterTable->writeControl = DSP_WT_OVER;

		// polling for the PC read over or not.
		retVal = pollValue(&(pRegisterTable->writeStatus), DSP_WT_READY,
				0x07ffffff);
		if (retVal == 0)
		{

		}
		else
		{
			Semaphore_pend(g_writeSemaphore, BIOS_WAIT_FOREVER);
			//retVal = -1;
			write_uart("wait the pc read.time over\n\r");
		}
	}
	// one com is finished .init the register.
	sprintf(debugBuf, "11111 p_gPictureInfor->picNums is %d\r\n",
			p_gPictureInfor->picNums);
	write_uart(debugBuf);
	//p_gPictureInfor->picNums = 0;
	pRegisterTable->getPicNumers = 0;
	pRegisterTable->failPicNumers = 0;
	downLoadPicNum = 0;
	downloadFail = 0;
	// set the dsp can be write again. NOte PC polling this for writting urls to DSP.
	pRegisterTable->readControl = DSP_RD_OVER;// pc can write to.
	// after the PC read,reset this. Note: PC polling this for reading pc's inBuffer
	pRegisterTable->writeControl = DSP_WT_READY;// pc can't read.
	//send interrupt

	// http download picture over.
	*((uint32_t *) (PCIE_EP_IRQ_SET)) = 0x1;
	write_uart("send the second interrupt from dsp to pc INT singal\n\r");
	write_uart("wait PC input\r\n");

	Semaphore_post(httptodpmSemaphore);
	write_uart("post the httptodpmSemaphore,and DPMMain can run\r\n");

	//stop program to wait dpmmain
	Semaphore_pend(gSendSemaphore, BIOS_WAIT_FOREVER);
	write_uart("pend the gSendSemaphore success\r\n");

}

#endif
#endif
	return (retVal);
//free(g_outBuffer);
//fdCloseSession(TaskSelf());
// display the download status
}

int getPics()
{
	while (1)
	{
		// wait pc to write.
		// update pictureBuffer status.
		// wait core1-7 to read.
		// wait pictureBuffer status.

	}
}

int putPics()
{
	while (1)
	{
		//getWantSendPicNums();
		//getPicBufferOfDSP();
		//sendPicsToDsp();
		//getPicBufferOfDSP();
	}
}

int distributePicTask()
{
	int retVal = 0;
	write_uart("distributePicTask wait g_writeSemaphore\n\r");
	Semaphore_pend(g_writeSemaphore, BIOS_WAIT_FOREVER);
	return (retVal);
}

/***************************************************************************
 *	parse the URL. for example: http://192.168.20.124:8088/1.jpg
 *  get the server ip + port + the need file.
 ***************************************************************************/
#if 0
int http_parseURL(char *p_url, http_downloadInfo *p_info)
{
	char *p_temp_string;
	char *p_temp_port;
	char *p_temp_filePath;
	char *p_port;
	int nTemp_fileDirStringLength = 0;
	int nTemp_urlLength = 0;
	int i;
//check the url is not NULL
	if (p_url == NULL)
	{
#if DEBUG
		printf("the input url is empty\n");
#endif
		return URL_NULL_ERROR;
	}
	else
	{
		nTemp_urlLength = strlen(p_url);
		p_info->p_input_url = (char *) malloc(nTemp_urlLength + 1);
		memcpy(p_info->p_input_url, p_url, nTemp_urlLength);
		p_info->p_input_url[nTemp_urlLength] = '\0';
	}
//check the url is the http protocol.
	p_temp_string = strstr(p_url, "http://");
	if (!p_temp_string)
	{
#if DEBUG
		printf("the protocal is not http\n");
#endif
		return URL_PROTOCAL_ERROR;
	}
//get the host .for example 192.168.30.124
	p_temp_string += strlen("http://");
	for (i = 0;
			(p_temp_string[i]) && (p_temp_string[i] != '/')
			&& (p_temp_string[i] != ':'); i++)
	;
	if (i == 0)
	{
#if DEBUG
		printf("error:there is no host information in the url\n");
#endif
		return URL_HOST_ERROR;
	}
	p_info->p_host_ip = (char *) malloc(i + 1);
	memcpy(p_info->p_host_ip, p_temp_string, i);
	p_info->p_host_ip[i] = '\0';
//get the port
	p_temp_port = strchr(p_temp_string, ':');
	if (p_temp_port != NULL)
	{
		p_temp_port += 1;
		for (i = 0; isdigit(p_temp_port[i]); i++)
		;
		if (i == 0)
		{
			p_info->n_port = 0;
#if DEBUG
			printf("there nedd a number after the ':' \n");
#endif
			return URL_PORT_ERROR;
		}
		p_port = (char *) malloc(i + 1);
		memcpy(p_port, p_temp_port, i);
		p_port[i] = '\0';
		p_info->n_port = atoi(p_port);
		free(p_port);
	}
	else
	p_info->n_port = 80;
//get the fileDir
	p_temp_filePath = strchr(p_temp_port, '/');
	if (p_temp_filePath == NULL)
	{
#if DEBUG
		printf("there nedd a number after the ':' \n");
#endif
		return URL_FILE_PATH_ERROR;
	}
	p_temp_filePath += 1;
	nTemp_fileDirStringLength = strlen(p_temp_filePath);
	if (!nTemp_fileDirStringLength)
	{
#if DEBUG
		printf("there nedd a number after the ':' \n");
#endif
		return URL_FILE_PATH_ERROR;
	}
	p_info->p_fileDir = (char *) malloc(nTemp_fileDirStringLength + 1);
	memcpy(p_info->p_fileDir, p_temp_filePath, nTemp_fileDirStringLength);
	p_info->p_fileDir[nTemp_fileDirStringLength] = '\0';
//Get fileName;
	/*************************************************************************
	 *	start from the end of fileDir,find the last '/'  ,record the number of the search i
	 *	if the i==0 ,so,the fileDir is the fileName.else
	 **************************************************************************/
	for (i = strlen(p_temp_filePath); i > 0 && p_temp_filePath[i] != '/'; i--)
	;

	if (!i) //modify.
	{
		p_info->p_fileName = (char *) malloc(nTemp_fileDirStringLength + 1);
		memcpy(p_info->p_fileName, p_info->p_fileDir,
				nTemp_fileDirStringLength);
		p_info->p_fileName[nTemp_fileDirStringLength] = '\0';
	}
	else
	{
		p_temp_string = p_temp_filePath + i + 1;
		i = strlen(p_temp_string);
		p_info->p_fileName = (char *) malloc(i + 1);
		memcpy(p_info->p_fileName, p_temp_string, i);
		p_info->p_fileName[i] = '\0';
	}
	return 0;
}

int httpSendRequest(SOCKET socket, void *buffer, int *length, int flag)
{
	int retVal = 0;
	fd_set fdWrite;
	struct timeval timeout;
	timeout.tv_sec = 2;
	Bool bSendFlag = TRUE;
	char debugBuf[100];
	int sendLength = 0;
	int needSendLength = *length;
	int socketErrorCode = 0;
	int sockNum = (int) socket;

	while (bSendFlag)
	{
		FD_ZERO(&fdWrite);
		FD_SET(socket, &fdWrite);

		switch (fdSelect(sockNum + 1, NULL, &fdWrite, NULL, &timeout))
		{
			case 0:
			{
				break;
			}
			case -1:
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "connect error:errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -1;
				bSendFlag = FALSE;
				break;
			}
			default:
			{
				if (FD_ISSET(socket,&fdWrite))
				{
					sendLength = send(socket, buffer, needSendLength, flag);
					if (sendLength == needSendLength)
					{
						write_uart("send successful\r\n");
						*length = needSendLength;
						bSendFlag = FALSE;
					}
					else
					{
						socketErrorCode = fdError();
						sprintf(debugBuf, "send error:errorCode=%d\r\n",
								socketErrorCode);
						write_uart(debugBuf);
						retVal = -2;
						bSendFlag = FALSE;

					}
				}
				break;
			}

		}
	}

	return (retVal);
}

int netSend(SOCKET socket, void *buffer, int length, int flag)
{
	int retVal = 0;
	char debugBuf[100];
	fd_set fdWrite;
	struct timeval timeout;
	timeout.tv_sec = 2;
	int socketErrorCode = 0;
	FD_ZERO(&fdWrite);
	FD_SET(socket, &fdWrite);
	int sockNum = (int) socket;
	int sendLength = 0;

	switch (fdSelect(sockNum + 1, &fdWrite, NULL, NULL, &timeout))
	{
		case 0:
		{
			socketErrorCode = fdError();
			sprintf(debugBuf, "send timeout:errorCode=%d\r\n", socketErrorCode);
			write_uart(debugBuf);
			retVal = -1;
			break;
		}
		case -1:
		{
			socketErrorCode = fdError();
			sprintf(debugBuf, "connect error:errorCode=%d\r\n", socketErrorCode);
			write_uart(debugBuf);
			retVal = -2;
			break;
		}
		default:
		{
			if (FD_ISSET(socket,&fdWrite))
			{
				sendLength = send(socket, buffer, length, flag);
				if (sendLength == length)
				{
					write_uart("send successful\r\n");
				}
				else
				{
					socketErrorCode = fdError();
					sprintf(debugBuf, "connect error:errorCode=%d\r\n",
							socketErrorCode);
					write_uart(debugBuf);
					retVal = -3;
				}
			}
			break;
		}

	}
	return (retVal);
}
#if 0
int HttpRecvHead(SOCKET socket, void *buffer, int *length)
{
	int retVal = 0;

	Bool bRecvFlag = TRUE;
	int recvOneByte = 1;
	int recvLength = 0;
	int totalRecv = 0;
	int peerLength = 0;
	char *pBuffer = (char *) buffer;

	fd_set fdRead;
	struct timeval timeout;
	timeout.tv_sec = 2;

	FD_ZERO(&fdRead);
	FD_SET(socket, &fdRead);

	switch (fdSelect(socket + 1, NULL, &fdRead, NULL, &timeout))
	{
		case 0:
		{
			socketErrorCode = fdError();
			sprintf(debugBuf, "send timeout:errorCode=%d\r\n", socketErrorCode);
			write_uart(debugBuf);
			retVal = -1;
		}
		case -1:
		{
			socketErrorCode = fdError();
			sprintf(debugBuf, "connect error:errorCode=%d\r\n", socketErrorCode);
			write_uart(debugBuf);
			retVal = -1;
		}
		default:
		{
			if (FD_ISSET(socket,&fdRead))
			{
				bRecvFlag = TRUE;
				while (bRecvFlag)
				{
					recvLength = recv(socket, pBuffer, recvOneByte, 0);
					if (recvLength > 0)
					{
						totalRecv += recvLength;
						pBuffer += recvLength;

						if (totalRecv > 4)
						{
							if ((memcmp(pBuffer - 4), "\r\n\r\n", 4) == 0)
							{
								bRecvFlag = FALSE;
								*length = totalRecv;
							}
						}

					}
					else if (recvLength == -1)
					{
						socketErrorCode = fdError();
						sprintf(debugBuf, "connect error:errorCode=%d\r\n",
								socketErrorCode);
						write_uart(debugBuf);
						bRecvFlag = FALSE;
						retVal = -2;
					}
					else if (recvLength == 0)
					{
						if ((memcmp(pBuffer - 4), "\r\n\r\n", 4) == 0)
						{
							bRecvFlag = FALSE;
						}
						else
						{
							socketErrorCode = fdError();
							sprintf(debugBuf,
									"recv error:errorCode=%d,recvLength=%d\r\n",
									socketErrorCode, totalRecv);
							write_uart(debugBuf);
							retVal = -1;
						}
					}
				}
			}
		}
	}

	return (retVal);
}
#endif
#if 0
int httpRecvHead(SOCKET socket, void *buffer, int *length)
{
	int retVal = 0;

	char debugBuf[100];
	Bool bRecvFlag = TRUE;
	int recvOneByte = 1;
	int recvLength = 0;
	int totalRecv = 0;
	char *pBuffer = (char *) buffer;
	int socketErrorCode = 0;
	fd_set fdRead;
	struct timeval timeout;
	timeout.tv_sec = 2;
	int sockNum = (int) socket;

	while (bRecvFlag)
	{
		FD_ZERO(&fdRead);
		FD_SET(socket, &fdRead);
		switch (fdSelect(sockNum + 1, &fdRead, NULL, NULL, &timeout))
		{
			case 0:
			{
				break;
			}
			case -1:
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "connect error:errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -1;
				bRecvFlag = FALSE;
				break;
			}
			default:
			{
				if (FD_ISSET(socket,&fdRead))
				{
					recvLength = recv(socket, pBuffer, recvOneByte, 0);
					if (recvLength > 0)
					{
						totalRecv += recvLength;
						pBuffer += recvLength;
					}
					else if (recvLength == 0)
					{
						socketErrorCode = fdError();
						sprintf(debugBuf, "connect break:errorCode=%d\r\n",
								socketErrorCode);
						write_uart(debugBuf);
						retVal = -2;
						bRecvFlag = FALSE;
					}
					else if (recvLength == -1)
					{
						socketErrorCode = fdError();
						sprintf(debugBuf, "recv error:errorCode=%d\r\n",
								socketErrorCode);
						write_uart(debugBuf);
						bRecvFlag = FALSE;
						retVal = -3;
					}
				}
				break;
			}
		}
		if (totalRecv > 4)
		{
			if (memcmp((pBuffer - 4), "\r\n\r\n", 4) == 0)
			{
				bRecvFlag = FALSE;
				*length = totalRecv;
				write_uart("get the http head successful\r\n");
			}
		}
		else
		{
		}
		// TODO: if there are some error.
	}
	return (retVal);
}
#endif
int httpRecvHead(SOCKET socket, void *buffer, int *length)
{
	int retVal = 0;

	char debugBuf[100];
	Bool bRecvFlag = TRUE;
	int recvOneByte = 1024;
	int recvLength = 0;
	int totalRecv = 0;
	char *pBuffer = (char *) buffer;
	int socketErrorCode = 0;
	fd_set fdRead;
	struct timeval timeout;
	timeout.tv_sec = 2;
	int sockNum = (int) socket;

	while (bRecvFlag)
	{
		FD_ZERO(&fdRead);
		FD_SET(socket, &fdRead);
		switch (fdSelect(sockNum + 1, &fdRead, NULL, NULL, &timeout))
		{
			case 0:
			{
				break;
			}
			case -1:
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "connect error:errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -1;
				bRecvFlag = FALSE;
				break;
			}
			default:
			{
				if (FD_ISSET(socket,&fdRead))
				{
					recvLength = recv(socket, pBuffer, recvOneByte, 0);
					if (recvLength > 0)
					{
						totalRecv += recvLength;
						pBuffer += recvLength;
					}
					else if (recvLength == 0)
					{
						socketErrorCode = fdError();
						sprintf(debugBuf, "connect break:errorCode=%d\r\n",
								socketErrorCode);
						write_uart(debugBuf);
						retVal = -2;
						bRecvFlag = FALSE;
					}
					else if (recvLength == -1)
					{
						socketErrorCode = fdError();
						sprintf(debugBuf, "recv error:errorCode=%d\r\n",
								socketErrorCode);
						write_uart(debugBuf);
						bRecvFlag = FALSE;
						retVal = -3;
					}
				}
				break;
			}
		}
		if (totalRecv > 4)
		{
			//if (memcmp((pBuffer - 4), "\r\n\r\n", 4) == 0)
			if (strstr(buffer, "\r\n\r\n") != 0)
			{
				bRecvFlag = FALSE;
				*length = totalRecv;
				sprintf(debugBuf, "headLength=%d\r\n", totalRecv);
				write_uart(debugBuf);
				write_uart("get the http head successful\r\n");
			}
		}
		else
		{
		}
		// TODO: if there are some error.
	}
	return (retVal);
}
int httpRecvGet(SOCKET socket, void *buffer, int *length, int flag)
{
	int retVal = 0;
	char debugBuf[100];
	Bool bRecvFlag = TRUE;
	int needRecvLength = *length;
	int restRecvLength = needRecvLength;
	int totalRecv = 0;
	int recvLength = 0;
	char *pBuffer = (char *) buffer;
	int socketErrorCode;
	fd_set fdRead;
	struct timeval timeout;
	timeout.tv_sec = 2;
	int sockNum = (int) socket;

	while (bRecvFlag)
	{
		FD_ZERO(&fdRead);
		FD_SET(socket, &fdRead);

		switch (fdSelect(sockNum + 1, &fdRead, NULL, NULL, &timeout))
		{
			case 0:
			{
				break;
			}
			case -1:
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "connect error:errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -1;
				bRecvFlag = FALSE;
				break;
			}
			default:
			{
				if (FD_ISSET(socket,&fdRead))
				{
					recvLength = recv(socket,
							pBuffer + (needRecvLength - restRecvLength),
							restRecvLength, flag);
					if (recvLength > 0)
					{
						totalRecv += recvLength;
						restRecvLength -= recvLength;
					}
					else if (recvLength == 0)
					{
						if (totalRecv == needRecvLength)
						{
							bRecvFlag = FALSE;
						}
						else
						{
							socketErrorCode = fdError();
							sprintf(debugBuf, "connect break :errorCode=%d\r\n",
									socketErrorCode);
							write_uart(debugBuf);
							retVal = -2;
							bRecvFlag = FALSE;
						}
					}
					else if (recvLength == -1)
					{
						socketErrorCode = fdError();
						sprintf(debugBuf, "recv error:errorCode=%d\r\n",
								socketErrorCode);
						write_uart(debugBuf);
						bRecvFlag = FALSE;
						retVal = -3;
					}

				}
				break;
			}

		}
		if (totalRecv == needRecvLength)
		{
			bRecvFlag = FALSE;
			write_uart("recv successful\r\n");
		}
		else
		{
		}
		// TODO: if there are some error.
	}
	return (retVal);
}
int httpRecvGetTemp(SOCKET socket, void *buffer, int *length, int flag)
{
	int retVal = 0;
	char debugBuf[100];
	Bool bRecvFlag = TRUE;
	int needRecvLength = *length;
	int restRecvLength = needRecvLength;
	int totalRecv = 0;
	int recvLength = 0;
	char *pBuffer = (char *) buffer;
	int socketErrorCode;
	fd_set fdRead;
	struct timeval timeout;
	timeout.tv_sec = 2;
	int sockNum = (int) socket;

	while (bRecvFlag)
	{

		recvLength = recv(socket, pBuffer + (needRecvLength - restRecvLength),
				restRecvLength, flag);
		if (recvLength > 0)
		{
			totalRecv += recvLength;
			restRecvLength -= recvLength;
		}
		else if (recvLength == 0)
		{
			if (totalRecv == needRecvLength)
			{
				bRecvFlag = FALSE;
			}
			else
			{
				socketErrorCode = fdError();
				sprintf(debugBuf, "connect break :errorCode=%d\r\n",
						socketErrorCode);
				write_uart(debugBuf);
				retVal = -2;
				bRecvFlag = FALSE;
			}
		}
		else if (recvLength == -1)
		{
			socketErrorCode = fdError();
			sprintf(debugBuf, "recv error:errorCode=%d\r\n", socketErrorCode);
			write_uart(debugBuf);
			bRecvFlag = FALSE;
			retVal = -3;
		}
		if (restRecvLength == 0)
		{
			*length = totalRecv;
			bRecvFlag = FALSE;
			write_uart("recv get successful\r\n");
		}
		else
		{
		}

	}

// TODO: if there are some error.

	return (retVal);
}
int httpRecvHeadTemp(SOCKET socket, void *buffer, int *length)
{
	int retVal = 0;

	char debugBuf[100];
	Bool bRecvFlag = TRUE;
	int recvOneByte = 1024;
	int recvLength = 0;
	int totalRecv = 0;
	char *pBuffer = (char *) buffer;
	int socketErrorCode = 0;

	while (bRecvFlag)
	{
		recvLength = recv(socket, pBuffer, recvOneByte, 0);
		if (recvLength > 0)
		{
			totalRecv += recvLength;
			pBuffer += recvLength;
		}
		else if (recvLength == 0)
		{
			socketErrorCode = fdError();
			sprintf(debugBuf, "connect break:errorCode=%d\r\n",
					socketErrorCode);
			write_uart(debugBuf);
			retVal = -2;
			bRecvFlag = FALSE;
		}
		else if (recvLength == -1)
		{
			socketErrorCode = fdError();
			sprintf(debugBuf, "recv error:errorCode=%d\r\n", socketErrorCode);
			write_uart(debugBuf);
			bRecvFlag = FALSE;
			retVal = -3;
		}
		if (strstr(buffer, "\r\n\r\n") != 0)
		{
			bRecvFlag = FALSE;
			*length = totalRecv;
			sprintf(debugBuf, "headLength=%d\r\n", totalRecv);
			write_uart(debugBuf);
			write_uart("get the http head successful\r\n");
		}
	}

	return (retVal);
}
#endif

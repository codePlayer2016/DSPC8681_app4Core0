/*
 *  ======== client.c ========
 *
 * TCP/IP Network Client example ported to use BIOS6 OS.
 *
 * Copyright (C) 2007, 2011 Texas Instruments Incorporated - http://www.ti.com/
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
//#include <ti/ndk/inc/netmain.h>
//#include <ti/ndk/inc/_stack.h>
//#include <ti/ndk/inc/tools/console.h>
//#include <ti/ndk/inc/tools/servers.h>

/* BIOS6 include */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>
#include <ti/sysbios/knl/Semaphore.h>
/* Platform utilities include */
#include "ti/platform/platform.h"

//add the TI's interrupt component.   add by LHS
#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>
#include <ti/sysbios/hal/Hwi.h>

/* Resource manager for QMSS, PA, CPPI */
#include "ti/platform/resource_mgr.h"

#include "app4Core0.h"

extern Semaphore_Handle gRecvSemaphore;
extern Semaphore_Handle gSendSemaphore;

extern Semaphore_Handle timeoutSemaphore;

extern Semaphore_Handle g_readSemaphore;
extern Semaphore_Handle g_writeSemaphore;
extern Semaphore_Handle pcFinishReadSemaphore;

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))
#define IPC_INT_ADDR(coreNum)				(0x02620240 + ((coreNum)*4))
#define IPC_AR_ADDR(coreNum)				(0x02620280+((coreNum)*4))

#define DDR_TEST_START                 0x80000000
#define DDR_TEST_END                   0x80400000
#define BOOT_UART_BAUDRATE                 115200

#define PCIEXpress_Legacy_INTA                 50
#define PCIEXpress_Legacy_INTB                 50
/*
 #define PCIE_IRQ_EOI                   0x21800050
 #define PCIE_EP_IRQ_SET		           0x21800064
 #define PCIE_LEGACY_A_IRQ_STATUS       0x21800184
 #define PCIE_LEGACY_A_IRQ_RAW          0x21800180
 #define PCIE_LEGACY_A_IRQ_SetEnable       0x21800188
 */
#define PCIE_LEGACY_A_IRQ_STATUS       0x21800184

#ifdef _EVMC6678L_
#define MAGIC_ADDR     (0x87fffc)
#define INTC0_OUT3     63
#endif

#define PAGE_SIZE (0x1000)
//the ddr read address space zone in DSP mapped to the PC.
#define DDR_WRITE_MMAP_START (0x80B00000)
#define DDR_WRITE_MMAP_LENGTH (0x00400000)
#define DDR_WRITE_MMAP_USED_LENGTH (0x00400000)
//the ddr read address space zone in DSP mapped to the PC.
#define DDR_READ_MMAP_START (0x80F00000)
#define DDR_READ_MMAP_LENGTH (0x00100000)
#define DDR_READ_MMAP_USED_LENGTH (0x00100000)
//expand a 4K space at the end of DDR_READ_MMAP zone as read and write flag
#define DDR_REG_PAGE_START (DDR_READ_MMAP_START + DDR_READ_MMAP_LENGTH - PAGE_SIZE)
#define DDR_REG_PAGE_USED_LENGTH (PAGE_SIZE)

#define OUT_REG (0x60000000)
#define IN_REG (0x60000004)
#define WR_REG (0x60000008)

#define WAITTIME (0x0FFFFFFF)

#define RINIT (0xaa55aa55)
#define READABLE (0x0)
#define RFINISH (0xaa55aa55)

#define WINIT (0x0)
#define WRITEABLE (0x0)
#define WFINISH (0x55aa55aa)
#define WRFLAG (0xFFAAFFAA)

#define GBOOT_MAGIC_ADDR(coreNum)			((1<<28) + ((coreNum)<<24) + (MAGIC_ADDR))
#define CORE0_MAGIC_ADDR                   0x1087FFFC
#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)






//#pragma DATA_SECTION(g_outBuffer,".WtSpace");
//unsigned char g_outBuffer[0x00600000]; //4M
//#pragma DATA_SECTION(g_inBuffer,".RdSpace");
unsigned char g_inBuffer[0x00100000]; //url value.
//add the SEM mode .    add by LHS

/* Platform Information - we will read it form the Platform Library */
platform_info gPlatformInfo;

extern int getPicTask();
extern int distributePicTask();



void write_uart(char* msg)
{
	uint32_t i;
	uint32_t msg_len = strlen(msg);

	/* Write the message to the UART */
	for (i = 0; i < msg_len; i++)
	{
		platform_uart_write(msg[i]);
	}
	//platform_uart_write('\r');
	//platform_uart_write('\n');
}
#if 1
/////////////////////////////////////////////////////////////////////////////////////////////
static void isrHandler(void* handle)
{
	//char debugInfor[100];
	//registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;
	CpIntc_disableHostInt(0, 3);
#if 0
	sprintf(debugInfor, "pRegisterTable->dpmStartStatus is %x \r\n",
			pRegisterTable->dpmStartStatus);
	write_uart(debugInfor);
	if ((pRegisterTable->dpmStartStatus) & DSP_DPM_STARTSTATUS)

	{
		Semaphore_post(gRecvSemaphore);
		//clear interrupt reg
		pRegisterTable->dpmStartControl = 0x0;

	}
	if ((pRegisterTable->dpmOverStatus) & DSP_DPM_OVERSTATUS)

	{
		Semaphore_post(pcFinishReadSemaphore);
		pRegisterTable->dpmStartControl = DSP_DPM_STARTCLR;

	}
	if ((pRegisterTable->readStatus) & DSP_RD_RESET)
	{
		Semaphore_post(g_readSemaphore);
	}
	if ((pRegisterTable->writeStatus) & DSP_WT_FINISH)

	{
		Semaphore_post(g_writeSemaphore);
	}
#endif
	//clear PCIE interrupt
	DEVICE_REG32_W(PCIE_LEGACY_A_IRQ_STATUS, 0x1);
	DEVICE_REG32_W(PCIE_IRQ_EOI, 0x0);
	CpIntc_clearSysInt(0, PCIEXpress_Legacy_INTA);

	CpIntc_enableHostInt(0, 3);
}
#endif

int main()
{
	// wait and receive the picture form the readZone that be pushed by the linux driver.
	// push picture to the ddr of the other coreN.
	// wait and get the coreN processed picture from the special ddr.
	// push the processed picture to the writeZone.
	int EventID_intc;
	Hwi_Params HwiParam_intc;
	registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;
	///////////////////////////////////////////////////////////////
	//add the TI's interrupt component.   add by LHS
	//Add the interrupt componet.
	/*
	 id -- Cp_Intc number
	 sysInt -- system interrupt number
	 hostInt -- host interrupt number
	 */

	CpIntc_mapSysIntToHostInt(0, PCIEXpress_Legacy_INTA, 3);
	//modify by cyx
	//CpIntc_mapSysIntToHostInt(0, PCIEXpress_Legacy_INTB, 3);
	/*
	 sysInt -- system interrupt number
	 fxn -- function
	 arg -- argument to function
	 unmask -- bool to unmask interrupt
	 */CpIntc_dispatchPlug(PCIEXpress_Legacy_INTA, (CpIntc_FuncPtr) isrHandler,
			15, TRUE);

	/*
	 id -- Cp_Intc number
	 hostInt -- host interrupt number
	 */CpIntc_enableHostInt(0, 3);
	//hostInt -- host interrupt number
	EventID_intc = CpIntc_getEventId(3);
	//HwiParam_intc
	Hwi_Params_init(&HwiParam_intc);
	HwiParam_intc.arg = 3;
	HwiParam_intc.eventId = EventID_intc; //eventId
	HwiParam_intc.enableInt = 1;
	/*
	 intNum -- interrupt number
	 hwiFxn -- pointer to ISR function
	 params -- per-instance config params, or NULL to select default values (target-domain only)
	 eb -- active error-handling block, or NULL to select default policy (target-domain only)
	 */Hwi_create(4, &CpIntc_dispatch, &HwiParam_intc, NULL);

	//
	// THIS MUST BE THE ABSOLUTE FIRST THING DONE IN AN APPLICATION before
	//  using the stack!!
	//

	//clear interrupt
	//pRegisterTable->dpmStartControl = DSP_DPM_STARTCLR;
	//TaskCreate(http_get, "http_get", OS_TASKPRINORM, 0x1400, 0, 0, 0);
	//TaskCreate(DPMMain, "DPMMain", OS_TASKPRINORM, 0x2000, 0, 0, 0);
	write_uart("app4Core0 start\n\r");
	BIOS_start();
}


static void interruptRegister()
{
	// IPC interrupt set.
		int ipcEventId=91;
		Hwi_Params hwiParams;
		Hwi_Params_init(&hwiParams);
		hwiParams.arg=ipcEventId;
		hwiParams.eventId=ipcEventId;
		hwiParams.enableInt=TRUE;
		Hwi_create(5,ipcIrqHandler,&hwiParams,NULL);
		Hwi_enable();
}
// get interrupt from Core0 can be read the picture from Core0.
static void ipcIrqHandler(UArg params)
{
	//read the IPC_AR_ADDR(0)
	//identify the interrupt source by the SRCCn bit of the IPC_AR_ADDR(0)
	//process
	//restore the IPC_CG_ADDR(0) by write the IPC_AR_ADDR(0) that read Value.
}
//Cache_wbInv();// use after write.
//Cache_wb();
//Cache_inv();  // use before read.
//Cache_wait()
static void triggleIPCinterrupt(int destCoreNum,int srcFlag)
{
	int writeValue=srcFlag|0x01;
	DEVICE_REG32_W(IPC_INT_ADDR(destCoreNum),writeValue);
}


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

#define IPC_INT_ADDR(coreNum)				(0x02620240 + ((coreNum)*4))
#define IPC_AR_ADDR(coreNum)				(0x02620280+((coreNum)*4))
#define PCIEXpress_Legacy_INTA                 50
#define PCIEXpress_Legacy_INTB                 50
#define BOOT_UART_BAUDRATE                 115200

#ifdef _EVMC6678L_
#define MAGIC_ADDR     (0x87fffc)
#define INTC0_OUT3     63
#endif

#define PAGE_SIZE (0x1000)
#define GBOOT_MAGIC_ADDR(coreNum)			((1<<28) + ((coreNum)<<24) + (MAGIC_ADDR))
#define CORE0_MAGIC_ADDR                   0x1087FFFC

static void registeIPCint();
static void ipcIrqHandler(UArg params);
void triggleIPCinterrupt(int destCoreNum, unsigned int srcFlag);

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
	DEVICE_REG32_W(KICK0, 0x83e70b13);
	DEVICE_REG32_W(KICK1, 0x95a4f1e0);
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
	registeIPCint();
	write_uart("app4Core0 start\n\r");
	BIOS_start();
}

static void registeIPCint()
{
	// IPC interrupt set.
	int ipcEventId = 91;
	Hwi_Params hwiParams;
	Hwi_Params_init(&hwiParams);
	hwiParams.arg = 32;
	hwiParams.eventId = ipcEventId;
	hwiParams.enableInt = TRUE;
	Hwi_create(5, ipcIrqHandler, &hwiParams, NULL);
	Hwi_enable();
}
// get interrupt from Core0 can be read the picture from CoreN.
static void ipcIrqHandler(UArg params)
{
	unsigned int ipcACKregVal = 0;
	unsigned int ipcACKval = 0;
	//read the IPC_AR_ADDR(0)
	ipcACKregVal = DEVICE_REG32_R(IPC_AR_ADDR(0));
	//identify the interrupt source by the SRCCn bit of the IPC_AR_ADDR(0)
	ipcACKval = (ipcACKregVal >> 4);
	//process
	//if (0 != ipcACKval)
	{
		Semaphore_post(g_readSemaphore);
	}
	//restore the IPC_CG_ADDR(0) by write the IPC_AR_ADDR(0) that read Value.
	//DEVICE_REG32_W(IPC_AR_ADDR(0), ipcACKregVal);
}
//Cache_wbInv();// use after write.
//Cache_wb();
//Cache_inv();  // use before read.
//Cache_wait()
void triggleIPCinterrupt(int destCoreNum, unsigned int srcFlag)
{
	unsigned int writeValue = ((srcFlag<<1)|0x01);
	DEVICE_REG32_W((IPC_INT_ADDR(destCoreNum)), writeValue);
}


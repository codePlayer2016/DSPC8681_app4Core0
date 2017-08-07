#ifndef APP4CORE0_H
#define APP4CORE0_H

#define MAGIC_ADDR     0x87fffc
//add cyx
#define PCIE_EP_IRQ_SET		           0x21800064
#define PCIE_EP_IRQ_CLR	               0x21800068
#define PCIE_LEGACY_A_IRQ_STATUS      0x21800184
#define PCIE_LEGACY_B_IRQ_STATUS      0x21800194
#define PCIE_LEGACY_A_IRQ_ENABLE_SET  0x21800188
#define PCIE_LEGACY_B_IRQ_ENABLE_SET  0x21800198
#define PCIE_IRQ_EOI                  50

#define IPC_INT_ADDR(coreNum)				(0x02620240 + ((coreNum)*4))
#define IPC_AR_ADDR(coreNum)				(0x02620280+((coreNum)*4))

#define CHIP_LEVEL_REG (0x02620000)
#define KICK0 (CHIP_LEVEL_REG+0x0038)
#define KICK1 (CHIP_LEVEL_REG+0x003C)

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))
#define C6678_PCIEDATA_BASE (0x60000000U)
#define DSP_RUN_READY			(0x00010000U)
#define DSP_RUN_FAIL			(0x00000000U)

//#define TRUE (1)
#define FASLE (0)
#define PCIE_BASE_ADDRESS            0x21800000
#define EP_IRQ_CLR                   0x68
#define EP_IRQ_STATUS                0x6C
//(0x180 + PCIE_BASE_ADDRESS)
#define LEGACY_A_IRQ_STATUS_RAW      (0x21800180)
#define LEGACY_A_IRQ_ENABLE_SET      (0x21800188)
#define LEGACY_A_IRQ_ENABLE_CLR      (0x2180018C)

#define PAGE_SIZE (0x1000)

// PC-side write(DSP-side read) buffer status.
/********************PC write to DSP.*/
// DSP-side read buffer status.(dsp set)
#define DSP_RD_RESET	(0x000055aaU)
#define DSP_RD_FINISH 	(0x550000aaU)
// PC-side write buffer status.(dsp polling)
#define PC_WT_FINISH 	(0xaa000055U)
#define PC_WT_RESET 	(0x0000aa55U)
/*************************************/
/********************DSP write to PC.*/
// PC-side read buffer status.(dsp polling)
#define PC_RD_RESET		(0x000077bbU)
#define PC_RD_FINISH	(0x770000bbU)
// DSP-side write buffer status.(dsp set)
#define DSP_WT_RESET	(0x0000bb77U)
#define DSP_WT_FINISH	(0xbb000077U)
/*************************************/

#ifndef NULL
#define NULL            ((void*)0)
#endif

typedef struct _tagRegisterTable
{
	// control registers. (4k)
	uint32_t DPUBootControl;
	uint32_t reserved0;
	uint32_t reserved1;
	uint32_t readControl;
	uint32_t writeControl;
	uint32_t getPicNumers;
	uint32_t failPicNumers;
	uint32_t dpmOverControl;
	uint32_t dpmStartControl;
	uint32_t dpmAllOverControl;
	uint32_t reserve2;
	uint32_t reserved3[0x1000 / 4 - 11];

	// status registers. (4k)
	uint32_t DPUBootStatus;
	uint32_t readStatus;
	uint32_t writeStatus;
	uint32_t DSP_urlNumsReg;
	uint32_t DSP_modelType; //1:motor 2:car 3:person
	uint32_t dpmOverStatus;
	uint32_t dpmStartStatus;
	uint32_t reserve4;
	uint32_t reserved5[0x1000 / 4 - 8];
} registerTable; //DSP

typedef struct _tagLinkLayerHandler
{
	registerTable *pRegisterTable;
	uint32_t *pOutBuffer;
	uint32_t *pInBuffer;
	uint32_t outBufferLength;
	uint32_t inBufferLength;

	// try removing global var.
	uint32_t *pWriteConfirmReg;
	uint32_t *pReadConfirmReg;
} LinkLayerHandler, *LinkLayerHandlerPtr;

#endif

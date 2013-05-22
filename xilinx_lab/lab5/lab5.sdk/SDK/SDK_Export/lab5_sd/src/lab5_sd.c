#include "xparameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xil_cache.h"
#include "ff.h"
#include "devcfg.h"
#include "xil_io.h"
#include "xil_types.h"

// Parameters for Partial Reconfiguration
#define BITFILE_ADDR   0x1200000
#define LAB1_BITFILE_LEN  0x03dbafc // BIN formatted BITfile length
#define LAB3_BITFILE_LEN  0x03dbafc
#define LAB1_ELFBIN_ADDR  0x00200000 // BIN formatted ELF address
#define LAB3_ELFBIN_ADDR  0x00600000
#define LAB1_ELFBINFILE_LEN  0x0000c00c // BIN formatted ELF length
#define LAB3_ELFBINFILE_LEN  0x0000c00c
#define LAB1_ELF_EXEC_ADDR  0x002003c0 // ELF main() entry point
#define LAB3_ELF_EXEC_ADDR  0x006003c0

// Read function for STDIN
extern char inbyte(void);

static int Reset = 1;
static FATFS fatfs;

// Driver Instantiations
static XDcfg *XDcfg_0;

// prototype for load_elf
void load_elf(u32 loadaddress);

int SD_Init()
{
	FRESULT rc;

	rc = f_mount(0, &fatfs);
	if (rc) {
		xil_printf(" ERROR : f_mount returned %d\r\n", rc);
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int SD_LoadBitFile(char *FileName, u32 DestinationAddress, u32 ByteLength)
{
	FIL fil;
	FRESULT rc;
	UINT br;

	rc = f_open(&fil, FileName, FA_READ);
	if (rc) {
		xil_printf(" ERROR : f_open returned %d\r\n", rc);
		return XST_FAILURE;
	}

	rc = f_lseek(&fil, 0);
	if (rc) {
		xil_printf(" ERROR : f_lseek returned %d\r\n", rc);
		return XST_FAILURE;
	}

	rc = f_read(&fil, (void*) DestinationAddress, ByteLength, &br);
	if (rc) {
		xil_printf(" ERROR : f_read returned %d\r\n", rc);
		return XST_FAILURE;
	}

	rc = f_close(&fil);
	if (rc) {
		xil_printf(" ERROR : f_close returned %d\r\n", rc);
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int SD_ElfLoad(char *FileName, u32 DestinationAddress, u32 ByteLength)
{
	FIL fil;
	FRESULT rc;
	UINT br;

	rc = f_open(&fil, FileName, FA_READ);
	if (rc) {
		xil_printf(" ERROR : f_open returned %d\r\n", rc);
		return XST_FAILURE;
	}

	rc = f_lseek(&fil, 0);
	if (rc) {
		xil_printf(" ERROR : f_lseek returned %d\r\n", rc);
		return XST_FAILURE;
	}

	rc = f_read(&fil, (void*) DestinationAddress, ByteLength, &br);
	if (rc) {
		xil_printf(" ERROR : f_read returned %d\r\n", rc);
		return XST_FAILURE;
	}

	rc = f_close(&fil);
	if (rc) {
		xil_printf(" ERROR : f_close returned %d\r\n", rc);
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}



int main()
{
	int Status;


    // Initialize SD controller and transfer partials to DDR
	SD_Init();

	// Initialize Device Configuration Interface
	XDcfg_0 = XDcfg_Initialize(XPAR_XDCFG_0_DEVICE_ID);

    // Display Menu
    int Exit = 0;
    int OptionCurr;
    int OptionNext = 1; // start-up default
	while(Exit != 1) {
		do {
			print("    1: Lab1\n\r");
			print("    2: Lab3\n\r");
			print("    0: Exit\n\r");
			print("> ");

			OptionCurr = OptionNext;
			OptionNext = inbyte();
			if (isalpha(OptionNext)) {
				OptionNext = toupper(OptionNext);
			}

			xil_printf("%c\n\r", OptionNext);
		} while (!isdigit(OptionNext));

		if (OptionCurr == OptionNext)
			continue;

		switch (OptionNext) {
			case '0':
				Exit = 1;
				break;
			case '1':
				Reset = 1;
				// Flush and disable Data Cache
				Xil_DCacheDisable();
				xil_printf("Loading lab1 BIT file\n\r");
				SD_LoadBitFile("lab1.bin", BITFILE_ADDR, (LAB1_BITFILE_LEN << 2));
				// Invalidate and enable Data Cache
				Xil_DCacheEnable();
				Xil_Out32(0xF8000008,0x0000DF0D); // Unlock devcfg.SLCR
				Xil_Out32(0xF8000900,0x0); // turn-off the level shifter
				Xil_Out32(0xF8000240,0xFFFFFFFF); // Put all FCLK in reset condition for 1.0 silicon for 3.0 it should be opposite
				Status = XDcfg_TransferBitfile(XDcfg_0, BITFILE_ADDR, LAB1_BITFILE_LEN);
				if (Status != XST_SUCCESS) {
					xil_printf("Error : FPGA configuration failed!\n\r");
					exit(EXIT_FAILURE);
				}
				xil_printf("Lab1.bin loaded!, executing its application.\n\r");
				Xil_Out32(0xF8000900, 0xF); // turn-ON the level shifter
				Xil_Out32(0xF8000240,0x00000000); // Bring all FCLK out of reset condition
				Xil_Out32(0xF8000004,0x767B); // Lock devcfg.SLCR
				SD_ElfLoad("lab1elf.bin", LAB1_ELFBIN_ADDR, LAB1_ELFBINFILE_LEN);
				load_elf(LAB1_ELF_EXEC_ADDR);
				break;
			case '2':
				Reset = 1;
				// Flush and disable Data Cache
				Xil_DCacheDisable();
				xil_printf("Loading lab3 BIT file\n\r");
				SD_LoadBitFile("lab3.bin", BITFILE_ADDR, (LAB3_BITFILE_LEN << 2));
				// Invalidate and enable Data Cache
				Xil_DCacheEnable();
				Xil_Out32(0xF8000008,0x0000DF0D); // Unlock devcfg.SLCR
				Xil_Out32(0xF8000900,0x0); // turn-off the level shifter
				Xil_Out32(0xF8000240,0xFFFFFFFF); // Put all FCLK in reset condition for 1.0 silicon for 3.0 it should be opposite
				Status = XDcfg_TransferBitfile(XDcfg_0, BITFILE_ADDR, LAB3_BITFILE_LEN);
				if (Status != XST_SUCCESS) {
					xil_printf("Error : FPGA configuration failed!\n\r");
					exit(EXIT_FAILURE);
				}
				xil_printf("Lab3.bin loaded!, executing its application.\n\r");
				Xil_Out32(0xF8000900, 0xF); // turn-ON the level shifter
				Xil_Out32(0xF8000240,0x00000000); // Bring all FCLK out of reset condition
				Xil_Out32(0xF8000004,0x767B); // Lock devcfg.SLCR
				SD_ElfLoad("lab3elf.bin", LAB3_ELFBIN_ADDR, LAB3_ELFBINFILE_LEN);
				load_elf(LAB3_ELF_EXEC_ADDR);
				break;
			default:
				break;
		}
	}

    return 0;
}


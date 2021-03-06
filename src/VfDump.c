//---------------------------------------------------------------------------------
// V F D U M P
// For dumping GBA games including Vast Fame protected carts
// Vaguely based on SendSave
//---------------------------------------------------------------------------------

#include "gba_interrupt.h"
#include "xcomms.h"
#include "gba_input.h"
#include "gba_systemcalls.h"

#include "gba_types.h"

#include <stdio.h>
#include <stdlib.h>

#include "libVf.h"
#include "libText.h"

u8 save_data[0x20000] __attribute__ ((section (".sbss")));
const char file_name[] = {"GBA_Cart.bin"};

#define PRINT(m) { dprintf(m); text_row(m);}
#define PUTCHAR(m) { dputchar(m); text_char(m);};

unsigned int frame = 0;
u8 *pak_ROM= ((u8*)0x08000000);
u16 *pak_SRAM= ((u16*)0x0E000000);
u16 *pak_FLASH= ((u16*)0x09FE0000);
u8 *pak_EEPROM= ((u8*)0x0D000000);
u16 *vid_mem= ((u16*)0x06000000);

struct bothKeys
{
	u16 gbaKeys;
	char keyboardKey;
};

void VblankInterrupt()
{
	frame += 1;
	scanKeys();
}

void findVfAddressReordering()
{
	for (u8 mode=0x00;mode<0x4;mode++)  { // Modes go up to F validly but 0-3 contain all the unique address swaps
		dprintf("\n= MODE %02x/%02x/%02x/%02x =\n",mode,mode+4,mode+8,mode+12);
		text_print("\n= MODE %02x/%02x/%02x/%02x =\n",mode,mode+4,mode+8,mode+12);
		u16 result;
		u16 results[0xFFFF];
		u16 currentWriteAddress = 0x0001;
		for(int z=0;z<0x0F;z++){ // It never gets the F tho is that a problem // I think nah
			BlankSram();
			result = FigureOutDestinationLocationForWrite(mode,currentWriteAddress);
			results[result] = z;
			currentWriteAddress = currentWriteAddress << 1;
		}
		PRINT("0f ");
		for(int ind = 0x4000;ind>0;ind/=2){
		  dprintf("%02x ",results[ind]);
		  text_print("%02x ",results[ind]);
		}
		PRINT("\n");
	}
}

void findVfValueReordering()
{
	for (u8 mode=0x00;mode<0x10;mode+=4)  { // Modes 0,4,8,C should have the regular addressing
		dprintf("\n= MODE %02x/%02x/%02x/%02x =\n",mode,mode+1,mode+2,mode+3);
		text_print("\n= MODE %02x/%02x/%02x/%02x =\n",mode,mode+1,mode+2,mode+3);
		int result;
		int results[0xFF];
		DoVfSramInit(mode);
		int writeValue=1;
		for(int z=0;z<8;z++){
			BlankSram();
			result=DoSramWriteAndRead(0x6969,writeValue,0x6969);
			results[result] = z;
			writeValue *= 2;
		}
		for(int ind = 0x80;ind>0;ind/=2){
		  dprintf("%02x ",results[ind]);
		  text_print("%02x ",results[ind]);
		}
		PRINT("\n");
	}
}

void readSramToFile(int handle)
{
	u32 memSize = 0x10000;
	dprintf("Getting chunk of 0x10000 from sram...\n");
	text_print("Read 0x10000 from sram\n");
	DumpSram(save_data);
	dfwrite(save_data,1,memSize,handle);
}

void readRomToFile(int handle,u32 offset,u32 chunkSize)
{
	dprintf("Getting chunk of 0x%08X from offset 0x%08X...\n",chunkSize,offset);
	text_print("Read %08X from %08X\n",chunkSize,offset);
	DumpRom(save_data,offset,chunkSize);
	dfwrite(save_data,1,chunkSize,handle);
}

struct bothKeys readKeys()
{
	struct bothKeys keyInput;
	VBlankIntrWait();
	keyInput.gbaKeys = keysDown();
	keyInput.keyboardKey = dgetch();
	return keyInput;
}

struct bothKeys waitForKey()
{
	irqEnable(IRQ_VBLANK); // Enable Vblank interrupt to allow VblankIntrWait
	REG_IME = 1; // Allow interrupts
	struct bothKeys keyInput;
	keyInput.gbaKeys = 0;
	keyInput.keyboardKey = 0;
	do {
		keyInput = readKeys();
	} while ( keyInput.gbaKeys == 0 && keyInput.keyboardKey == 0 );
	irqDisable(IRQ_VBLANK); // Disable interrupts
	REG_IME = 0;
	return keyInput;
}

void romdump(bool vfame,bool wholeCartArea)
{
	int handle;
	u32 chunkSize;

	chunkSize = 0x8000;

	u32 totalSize = wholeCartArea ? 0x8000000 : 0x2000000;
	u32 offset = 0;

	handle = dfopen(file_name,"wb");
	dfseek(handle,0,SEEK_SET);

	if (vfame) {
		DoVfRomInit();
	}
	while(offset<totalSize) {
		readRomToFile(handle,offset,chunkSize);
		offset+=chunkSize;
	}

	dfclose(handle);
}

int main(void)
{
	u32 x;

    struct bothKeys keyInput;

	// Set up the interrupt handlers
	irqInit();

	irqSet( IRQ_VBLANK, VblankInterrupt);

	xcomms_init();
	text_init();
    PRINT("\n");
	PRINT("NAME: ");
	for (x = 0; x < 12; x++){
		PUTCHAR(pak_ROM[160 + x]);
	}
	PRINT("\n\n");

	text_print("Press A to dump normal ROM\n");
    text_print("Press B to dump VF ROM\n");
    text_print("START to get value reordering\n");
    text_print("SELECT to get addr reordering\n");
    text_print("\n*SEL/START will erase save!*\n");
	dprintf("Press D to dump ROM\n");
	dprintf("Press V to dump VF ROM\n");
	dprintf("Press R to get value reordering\n");
	dprintf("Press S to get address reordering\n");
    dprintf("\n* R/S will erase your save data!\n");
    PRINT("\n");

	do {
		keyInput = waitForKey();
	} while ( (keyInput.gbaKeys != (KEY_A)) && (keyInput.keyboardKey !='D') && (keyInput.keyboardKey !='d') &&
	          (keyInput.gbaKeys != (KEY_B)) && (keyInput.keyboardKey !='V') && (keyInput.keyboardKey !='v')  &&
			  (keyInput.gbaKeys != (KEY_START)) && (keyInput.keyboardKey !='R') && (keyInput.keyboardKey !='r') &&
			  (keyInput.gbaKeys != (KEY_SELECT)) && (keyInput.keyboardKey !='S') && (keyInput.keyboardKey !='s') &&
			  (keyInput.gbaKeys != (KEY_DOWN)) && (keyInput.keyboardKey !='M') && (keyInput.keyboardKey !='m')  &&
			  (keyInput.gbaKeys != (KEY_UP)) && (keyInput.keyboardKey !='T') && (keyInput.keyboardKey !='t') &&
			  (keyInput.gbaKeys != (KEY_RIGHT)) && (keyInput.keyboardKey !='Q') && (keyInput.keyboardKey !='q') );

    if ( keyInput.gbaKeys == KEY_B || keyInput.keyboardKey == 'V' || keyInput.keyboardKey == 'v' ) {
        PRINT("Let's VF DUMP\n");
        romdump(true,false);
    } else if (keyInput.gbaKeys == KEY_START || keyInput.keyboardKey == 'R' || keyInput.keyboardKey == 'r') {
        PRINT("Let's GET VALUE REORDERING\n");
        findVfValueReordering();
    } else if (keyInput.gbaKeys == KEY_SELECT || keyInput.keyboardKey == 'S' || keyInput.keyboardKey == 's') {
        PRINT("Let's GET ADDRESS REORDERING\n");
        findVfAddressReordering();
    } else if (keyInput.gbaKeys == KEY_DOWN || keyInput.keyboardKey == 'M' || keyInput.keyboardKey == 'm') {
        PRINT("MEGA DUMP EVERYTHING\n");
		romdump(false,true);
    } else if (keyInput.gbaKeys == KEY_A || keyInput.keyboardKey == 'D' || keyInput.keyboardKey == 'd')  {
        PRINT("Let's NORMAL DUMP\n");
		romdump(false,false);
    }

	PRINT("\nDone\n");

	PRINT("\nPress any key to reset GBA\n");
	waitForKey();

    SystemCall(0x26); // reset!

	return (0);
}

/*
	libSave
	Cartridge backup memory save routines. To use, call the required
	routine with a pointer to an appropriately sized array of data to
	be read from or written to the cartridge.
	Data types are from wintermute's gba_types.h libgba library.
*/

#include <stdio.h>
#include <stdlib.h>

//---------------------------------------------------------------------------------
#ifndef	_gba_types_h_
#define	_gba_types_h_
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// Data	types
//---------------------------------------------------------------------------------
/** Unsigned 8 bit value

*/
typedef	unsigned char			u8;
/** Unsigned 16 bit value

*/
typedef	unsigned short int		u16;
/** Unsigned 32 bit value

*/
typedef	unsigned int			u32;

/** signed 8 bit value

*/
typedef	signed char				s8;
/** Signed 16 bit value

*/
typedef	signed short int		s16;
/** Signed 32 bit value

*/
typedef	signed int				s32;

/** Unsigned volatile 8 bit value

*/
typedef	volatile u8				vu8;
/** Unsigned volatile 16 bit value

*/
typedef	volatile u16			vu16;
/** Unsigned volatile 32 bit value

*/
typedef	volatile u32			vu32;

/** Unsigned volatile 8 bit value

*/
typedef	volatile s8				vs8;
/** Signed volatile 16 bit value

*/
typedef	volatile s16			vs16;
/** Signed volatile 32 bit value

*/
typedef	volatile s32			vs32;

#ifndef __cplusplus
/** C++ compatible bool for C

*/
typedef enum { false, true } bool;
#endif

//---------------------------------------------------------------------------------
#endif // data types
//---------------------------------------------------------------------------------


#define EEPROM_ADDRESS (volatile u16*)0xD000000 
#define SRAM_ADDRESS (volatile u16*)0x0E000000
#define FLASH_1M_ADDRESS (volatile u16*)0x09FE0000
#define REG_EEPROM  (*(volatile u16*)0xD000000) 
#define REG_DM3SAD  (*(volatile u32*)0x40000D4) 
#define REG_DM3DAD  (*(volatile u32*)0x40000D8) 
#define REG_DM3CNT  (*(volatile u32*)0x40000DC)


//-----------------------------------------------------------------------
// Common EEPROM Routines
//-----------------------------------------------------------------------

void EEPROM_SendPacket( u16* packet, int size ) 
{ 
   REG_DM3SAD = (u32)packet; 
   REG_DM3DAD = (u32)EEPROM_ADDRESS; 
   REG_DM3CNT = 0x80000000 + size; 
} 

void EEPROM_ReceivePacket( u16* packet, int size ) 
{ 
   REG_DM3SAD = (u32)EEPROM_ADDRESS; 
   REG_DM3DAD = (u32)packet; 
   REG_DM3CNT = 0x80000000 + size; 
} 

//-----------------------------------------------------------------------
// Routines for 512B EEPROM
//-----------------------------------------------------------------------

void EEPROM_Read_512B( volatile u8 offset, u8* dest ) // dest must point to 8 bytes 
{ 
   u16 packet[68]; 
   u8* out_pos; 
   u16* in_pos; 
   u8 out_byte; 
   int byte, bit; 

   // Read request 
   packet[0] = 1; 
   packet[1] = 1; 

   // 6 bits eeprom address (MSB first) 
   packet[2] = offset>>5; 
   packet[3] = offset>>4; 
   packet[4] = offset>>3; 
   packet[5] = offset>>2; 
   packet[6] = offset>>1; 
   packet[7] = offset; 

   // End of request 
   packet[8] = 0; 

   // Do transfers 
   EEPROM_SendPacket( packet, 9 ); 
   EEPROM_ReceivePacket( packet, 68 ); 
   
   // Extract data 
   in_pos = &packet[4]; 
   out_pos = dest; 
   for( byte = 7; byte >= 0; --byte ) 
   { 
      out_byte = 0; 
      for( bit = 7; bit >= 0; --bit ) 
      { 
//         out_byte += (*in_pos++)<<bit; 
		  out_byte += ((*in_pos++)&1)<<bit;
      } 
      *out_pos++ = out_byte ; 
   } 
} 

void EEPROM_Write_512B( volatile u8 offset, u8* source ) // source must point to 8 bytes 
{ 
   u16 packet[73]; 
   u8* in_pos; 
   u16* out_pos; 
   u8 in_byte; 
   int byte, bit; 

   // Write request 
   packet[0] = 1; 
   packet[1] = 0; 

   // 6 bits eeprom address (MSB first) 
   packet[2] = offset>>5; 
   packet[3] = offset>>4; 
   packet[4] = offset>>3; 
   packet[5] = offset>>2; 
   packet[6] = offset>>1; 
   packet[7] = offset; 

   // Extract data 
   in_pos = source; 
   out_pos = &packet[8]; 
   for( byte = 7; byte >= 0; --byte ) 
   { 
      in_byte = *in_pos++; 
      for( bit = 7; bit >= 0; --bit ) 
      { 
         *out_pos++ = in_byte>>bit; 
      } 
   } 

   // End of request 
   packet[72] = 0; 

   // Do transfers 
   EEPROM_SendPacket( packet, 73 ); 

   // Wait for EEPROM to finish (should timeout after 10 ms) 
   while( (REG_EEPROM & 1) == 0 ); 
} 

//---------------------------------------------------------------------------------
void GetSave_EEPROM_512B(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 x;
	u32 sleep;

	// Set up waitstates for EEPROM access etc. 
	*(volatile unsigned short *)0x04000204 = 0x4317; 

    for (x=0;x<64;++x){
		EEPROM_Read_512B(x,&data[x*8]);
		for(sleep=0;sleep<512000;sleep++);
	}
}

//---------------------------------------------------------------------------------
void PutSave_EEPROM_512B(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 x;
	u32 sleep;

	// Set up waitstates for EEPROM access etc. 
	*(volatile unsigned short *)0x04000204 = 0x4317; 

    for (x=0;x<64;x++){ 
		EEPROM_Write_512B(x,&data[x*8]);
		for(sleep=0;sleep<512000;sleep++);
	}
}
//-----------------------------------------------------------------------
// Routines for 8KB EEPROM
//-----------------------------------------------------------------------

void EEPROM_Read_8KB( volatile u16 offset, u8* dest ) // dest must point to 8 bytes 
{ 
   u16 packet[68]; 
   u8* out_pos; 
   u16* in_pos; 
   u8 out_byte; 
   int byte, bit; 

   // Read request 
   packet[0] = 1; 
   packet[1] = 1; 

   // 14 bits eeprom address (MSB first) 
   packet[2] = offset>>13; 
   packet[3] = offset>>12; 
   packet[4] = offset>>11; 
   packet[5] = offset>>10; 
   packet[6] = offset>>9; 
   packet[7] = offset>>8;
   packet[8] = offset>>7; 
   packet[9] = offset>>6; 
   packet[10] = offset>>5; 
   packet[11] = offset>>4; 
   packet[12] = offset>>3; 
   packet[13] = offset>>2;
   packet[14] = offset>>1;
   packet[15] = offset;

   // End of request 
   packet[16] = 0; 

   // Do transfers 
   EEPROM_SendPacket( packet, 17 ); 
   EEPROM_ReceivePacket( packet, 68 ); 
   
   // Extract data 
   in_pos = &packet[4]; 
   out_pos = dest; 
   for( byte = 7; byte >= 0; --byte ) 
   { 
      out_byte = 0; 
      for( bit = 7; bit >= 0; --bit ) 
      { 
//         out_byte += (*in_pos++)<<bit; 
		 out_byte += ((*in_pos++)&1)<<bit;
      } 
      *out_pos++ = out_byte; 
   } 

} 

void EEPROM_Write_8KB( volatile u16 offset, u8* source ) // source must point to 8 bytes 
{ 
   u16 packet[81]; 
   u8* in_pos; 
   u16* out_pos; 
   u8 in_byte; 
   int byte, bit; 

   // Write request 
   packet[0] = 1; 
   packet[1] = 0; 

   // 14 bits eeprom address (MSB first) 
   packet[2] = offset>>13; 
   packet[3] = offset>>12; 
   packet[4] = offset>>11; 
   packet[5] = offset>>10; 
   packet[6] = offset>>9; 
   packet[7] = offset>>8;
   packet[8] = offset>>7; 
   packet[9] = offset>>6; 
   packet[10] = offset>>5; 
   packet[11] = offset>>4; 
   packet[12] = offset>>3; 
   packet[13] = offset>>2;
   packet[14] = offset>>1;
   packet[15] = offset;

   // Extract data 
   in_pos = source; 
   out_pos = &packet[16]; 
   for( byte = 7; byte >= 0; --byte ) 
   { 
      in_byte = *in_pos++; 
      for( bit = 7; bit >= 0; --bit ) 
      { 
         *out_pos++ = in_byte>>bit; 
      } 
   } 

   // End of request 
   packet[80] = 0; 

   // Do transfers 
   EEPROM_SendPacket( packet, 81 ); 

   // Wait for EEPROM to finish (should timeout after 10 ms) 
   while( (REG_EEPROM & 1) == 0 ); 
} 

//---------------------------------------------------------------------------------
void GetSave_EEPROM_8KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u16 x;
	u32 sleep;

	// Set up waitstates for EEPROM access etc. 
	*(volatile unsigned short *)0x04000204 = 0x4317; 

    for (x=0;x<1024;x++){ 
		EEPROM_Read_8KB(x,&data[x*8]);
		for(sleep=0;sleep<512000;sleep++);
	}

}

//---------------------------------------------------------------------------------
void PutSave_EEPROM_8KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u16 x;
	u32 sleep;

	// Set up waitstates for EEPROM access etc. 
	*(volatile unsigned short *)0x04000204 = 0x4317; 

    for (x=0;x<1024;x++){ 
		EEPROM_Write_8KB(x,&data[x*8]);
		for(sleep=0;sleep<512000;sleep++);
	}
}

//---------------------------------------------------------------------------------
void GetSave_SRAM_32KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 *sram= (u8*) 0x0E000000;
	volatile u16 x;

	for (x = 0; x < 0x8000; ++x)
	{
		data[x] = sram[x];
	}

}

//---------------------------------------------------------------------------------
void PutSave_SRAM_32KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 *sram= (u8*) 0x0E000000;
	volatile u16 x;

	for (x = 0; x < 0x8000; ++x)
	{
		sram[x] = data[x];
	}
}

//---------------------------------------------------------------------------------
void GetSave_FLASH_64KB(u8* data)
//---------------------------------------------------------------------------------
{
	volatile u8 *sram= (u8*) 0x0E000000;
	volatile u32 x;

	for (x = 0; x < 0x10000; ++x)
	{
		data[x] = sram[x];
	}
}

//---------------------------------------------------------------------------------
void PutSave_FLASH_64KB(u8* foo)
//---------------------------------------------------------------------------------
{
	volatile u8 *fctrl0 = (u8*) 0xE005555; 
	volatile u8 *fctrl1 = (u8*) 0xE002AAA; 
	volatile u8 *fctrl2 = (u8*) 0xE000000; 

	//init flash 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x90; 
	*fctrl2 = 0xF0; 

	//erase chip 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x80; 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x10; 

	//wait for erase done 
	u8 val1;
	u8 val2; 
	val1 = *fctrl2;
	val2 = *fctrl2; 
	while (val1 != val2) {
		val1 = *fctrl2;
		val2 = *fctrl2;
	}
	val1 = *fctrl2;
	val2 = *fctrl2; 
	while (val1 != val2) {
		val1 = *fctrl2;
		val2 = *fctrl2; 
	}

	volatile u8 *data = fctrl2; 
	u32 i; 
	//write data 
	for (i=0; i<65536; i++) { 
		*fctrl0 = 0xAA; 
		*fctrl1 = 0x55; 
		*fctrl0 = 0xA0; 
		data [i] = foo [ i ]; 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
	
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		}
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ];
			val2 = data [ i ]; 
		} 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		} 
	}
}

//---------------------------------------------------------------------------------
void GetSave_FLASH_128KB(u8* data)
//---------------------------------------------------------------------------------
{
	const u32 size = 0x10000;
	
	volatile u8 *fctrl0 = (u8*) 0xE005555; 
	volatile u8 *fctrl1 = (u8*) 0xE002AAA; 
	volatile u8 *fctrl2 = (u8*) 0xE000000; 
	volatile u32 i; 
	volatile u8 *sram= (u8*) 0x0E000000;
	
	//init flash 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x90; 
	*fctrl2 = 0xF0; 

	// read first bank
	*fctrl0 = 0xAA;
	*fctrl1 = 0x55;
	*fctrl0 = 0xB0;
	*fctrl2 = 0x00;
	
	for (i=0; i<size; i++){
		data[i] = sram[i];
	}

	// read second bank
	*fctrl0 = 0xAA;
	*fctrl1 = 0x55;
	*fctrl0 = 0xB0;
	*fctrl2 = 0x01;
	
	for (i=0; i<size; i++){
		data[i + size] = sram[i];
	}

}

//---------------------------------------------------------------------------------
void PutSave_FLASH_128KB(u8* foo)
//---------------------------------------------------------------------------------
{
	volatile u8 *fctrl0 = (u8*) 0xE005555; 
	volatile u8 *fctrl1 = (u8*) 0xE002AAA; 
	volatile u8 *fctrl2 = (u8*) 0xE000000; 

	u8 val1;
	u8 val2; 

	//init flash 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x90; 
	*fctrl2 = 0xF0; 

	//erase chip 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x80; 
	*fctrl0 = 0xAA; 
	*fctrl1 = 0x55; 
	*fctrl0 = 0x10; 
	
	//wait for erase done 
	val1 = *fctrl2;
	val2 = *fctrl2; 
	while (val1 != val2) {
		val1 = *fctrl2;
		val2 = *fctrl2;
	}
	val1 = *fctrl2;
	val2 = *fctrl2; 
	while (val1 != val2) {
		val1 = *fctrl2;
		val2 = *fctrl2; 
	}

	volatile u8 *data = fctrl2; 
	volatile u32 i; 
	// change to bank 0
	*fctrl0 = 0xAA;
	*fctrl1 = 0x55;
	*fctrl0 = 0xB0;
	*fctrl2 = 0x00;
		
	//write data 
	for (i=0; i<65536; i++) { 
		*fctrl0 = 0xAA; 
		*fctrl1 = 0x55; 
		*fctrl0 = 0xA0; 
		data [i] = foo [ i ]; 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
	
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		}
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ];
			val2 = data [ i ]; 
		} 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		} 
	}

	// Change to bank 1
	*fctrl0 = 0xAA;
	*fctrl1 = 0x55;
	*fctrl0 = 0xB0;
	*fctrl2 = 0x01;
	
	//write data 
	for (i=0; i<65536; i++) { 
		*fctrl0 = 0xAA; 
		*fctrl1 = 0x55; 
		*fctrl0 = 0xA0; 
		data [i] = foo [ i + 0x10000]; 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
	
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		}
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ];
			val2 = data [ i ]; 
		} 
		val1 = data [ i ]; 
		val2 = data [ i ]; 
		while (val1 != val2) {
			val1 = data [ i ]; 
			val2 = data [ i ]; 
		} 
	}

}

//---------------------------------------------------------------------------------
u32 SaveSize(void)
//---------------------------------------------------------------------------------
{
	u32 *pak= ((u32*)0x08000000);
	u32 x;
	u16 i;
	u32 size = 8388608;
	u8 temp1[8];
	u8 temp2[8];
	
	
	for (x=0;x< size;x++){
		switch (pak[x]) {
		case 0x53414C46:
			if (pak[x+1] == 0x5F4D3148){
				return 0x20000;					// FLASH_128KB
			} else if ((pak[x+1] & 0x0000FFFF) == 0x00005F48){
				return 0x10000;					// FLASH_64KB
			} else if (pak[x+1] == 0x32313548){
				return 0x10000;					// FLASH_64KB
			}
			break;
		case 0x52504545:
			if ((pak[x+1] & 0x00FFFFFF) == 0x005F4D4F){
				EEPROM_Read_8KB(0,temp1);
				for (i=0;i< 100; i ++){
					EEPROM_Read_8KB(i,temp2);
					if (temp1[0] != temp2[0]){
						return 0x2000;			//EEPROM_8KB
					}
				}
				return 0x200;					// EEPROM_512B
			}
			break;
		case 0x4D415253:
			if ((pak[x+1] & 0x000000FF) == 0x0000005F){
				return 0x8000;					// SRAM_32KB
			}
		}
	}
	return 0;
}


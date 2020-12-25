
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include "video_convert.h"

extern BPTR video_debug_out;

uint16 *lookup16bit = NULL;

extern uint16 *vpal16;
extern uint32 *vpal32;

struct video_convert_names vcn[] =	{

	{"convert_1bit_to_8bit",(void *) convert_1bit_to_8bit},	// obsolete
	{"convert_2bit_to_8bit",(void *) convert_2bit_to_8bit},	// obsolete
	{"convert_4bit_to_8bit",(void *) convert_4bit_to_8bit},	// obsolete
	{"convert_copy_8bit",(void *) convert_copy_8bit},		// obsolete

	{"convert_1bit_to_8bit_8pixels",(void *) convert_1bit_to_8bit_8pixels},
	{"convert_2bit_to_8bit_8pixels",(void *) convert_2bit_to_8bit_8pixels},
	{"convert_4bit_to_8bit_4pixels",(void *) convert_4bit_to_8bit_4pixels},

	{"convert_1bit_to_16bit",(void *) convert_1bit_to_16bit},
	{"convert_2bit_lookup_to_16bit",(void *) convert_2bit_lookup_to_16bit},
	{"convert_4bit_lookup_to_16bit",(void *) convert_4bit_lookup_to_16bit},
	{"convert_4bit_lookup_to_16bit_2pixels",(void *) convert_4bit_lookup_to_16bit_2pixels},
	{"convert_8bit_lookup_to_16bit",(void *) convert_8bit_lookup_to_16bit},
	{"convert_8bit_lookup_to_16bit_2pixels",(void *) convert_8bit_lookup_to_16bit_2pixels},
	{"convert_15bit_to_16bit_be",(void *) convert_15bit_to_16bit_be},
	{"convert_15bit_to_16bit_le",(void *) convert_15bit_to_16bit_le},
	{"convert_16bit_lookup_to_16bit",(void *) convert_16bit_lookup_to_16bit},
	{"convert_32bit_to_16bit_be",(void *) convert_32bit_to_16bit_be},
	{"convert_32bit_to_16bit_le",(void *) convert_32bit_to_16bit_le},
	{"convert_copy_16bit",(void *) convert_copy_16bit},

	{"convert_4bit_lookup_to_32bit_2pixels",(void *) convert_4bit_lookup_to_32bit_2pixels},
	{"convert_8bit_lookup_to_32bit_2pixels",(void *) convert_8bit_lookup_to_32bit_2pixels},

	{"convert_1bit_to_32bit",(void *) convert_1bit_to_32bit},
	{"convert_2bit_to_32bit",(void *) convert_2bit_to_32bit},
	{"convert_4bit_to_32bit",(void *) convert_4bit_to_32bit},
	{"convert_8bit_to_32bit",(void *) convert_8bit_to_32bit},
	{"convert_8bit_to_32bit_db",(void *) convert_8bit_to_32bit_db},
	{"convert_15bit_to_32bit",(void *) convert_15bit_to_32bit},
	{"convert_16bit_to_32bit",(void *) convert_16bit_to_32bit},
	{"convert_copy_32bit",(void *) convert_copy_32bit},

	{NULL,NULL}};


const char *get_name_converter_fn_ptr( void *fn_ptr)
{
	struct video_convert_names *i = vcn;
	while (i -> fn)
	{
		if (i -> fn == fn_ptr)	return i -> name;
		i ++;
	}
	return NULL;
}


/*
char convert_1bit_to_32bit_asm( char *from, uint32 *to,int  bytes )
{
	asm
	(
		"li 9,0 \n"

		"loop_1bit: \n"

			"lbz		0,0(%1) \n"

			"li		3,128 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip128 \n"
			"li		3,-1 \n"
			"skip128: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,64 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip64 \n"
			"li		3,-1 \n"
			"skip64: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,32 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip32 \n"
			"li		3,-1 \n"
			"skip32: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,16 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip16 \n"
			"li		3,-1 \n"
			"skip16: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,8 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip8 \n"
			"li		3,-1 \n"
			"skip8: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,4 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip4 \n"
			"li		3,-1 \n"
			"skip4: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,2 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip2 \n"
			"li		3,-1 \n"
			"skip2: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,2 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip1 \n"
			"li		3,-1 \n"
			"skip1: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"addi	%1,%1,1 \n"
			"addi	9,9,1 \n"

		"cmpw	cr7,9,%3 \n"
		"blt+	cr7,loop_1bit"

		::"r" (pal), "r" (from), "r" (to), "r" (bytes): "r9","r0","r3","r4"
	);
}
*/

void init_lookup_1bit_to_8bit_8pixels()
{
	int n;
	char *to;
	char *offset;

	if (lookup16bit != NULL) 
	{
		if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- lookup not freed as it should be\n",__FUNCTION__,__LINE__ );
	}

	lookup16bit = (uint16 *) malloc(256 * 8);	
	if (lookup16bit == NULL) return;

	to = (char *) lookup16bit;

	for (n=0; n<256;n++)
	{	
		offset = to + (n*8);

		offset[0] = (n & 0x0080)>>7;
		offset[1] = (n & 0x0040)>>6;
		offset[2] = (n & 0x0020)>>5;
		offset[3] = (n & 0x0010)>>4;
		offset[4] = (n & 0x0008)>>3;
		offset[5] = (n & 0x0004)>>2;
		offset[6] = (n & 0x0002)>>1;
		offset[7] = (n & 0x0001);
	}
}

void init_lookup_2bit_to_8bit_8pixels()
{
	uint16 n;
	char *to;
	char *offset;

	if (lookup16bit != NULL) 
	{
		if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- lookup not freed as it should be\n",__FUNCTION__,__LINE__ );
	}

	lookup16bit = (uint16 *) malloc(256 * 256 * 8);	
	if (lookup16bit == NULL) return;

	to = (char *) lookup16bit;

	n=0;
	do
	{	
		offset = to + (n*8);

		offset[0] = (n & 0xC000)>>14;
		offset[1] = (n & 0x3000)>>12;
		offset[2] = (n & 0x0C00)>>10;
		offset[3] = (n & 0x0300)>>8;
		offset[4] = (n & 0x00C0)>>6;
		offset[5] = (n & 0x0030)>>4;
		offset[6] = (n & 0x000C)>>2;
		offset[7] = (n & 0x0003);
		n++;
	} while (n != 0);
}

void init_lookup_4bit_to_8bit_4pixels()
{
	uint8 *source;
	uint16 n;
	uint8 *to;

	uint32 offset;

	if (lookup16bit != NULL) 
	{
		if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- lookup not freed as it should be\n",__FUNCTION__,__LINE__ );
	}

	lookup16bit = (uint16 *) malloc( (16*16*16*16) * 4);	
	if (lookup16bit == NULL) return;

	to = (uint8 *) lookup16bit;

	n =0;
	do
	{
		source = (uint8 *) &n;

		offset = n*4;

		to[offset + 0] = (*source & 0xF0)>>4;
		to[offset + 1] = (*source & 0x0F);

		source ++;

		to[offset + 2] = (*source & 0xF0)>>4;
		to[offset + 3] = (*source & 0x0F);
		n++;
	} while (n != 0);
}

void convert_1bit_to_8bit(  char *from, char *to,int  pixels )
{
	register int n;
	register char source;
	int bytes = pixels / 8;

	for (n=0; n<bytes;n++)
	{
		source = from[n];
		*to++ = (source & 128)>>7;
		*to++ = (source & 64)>>6 ;
		*to++ = (source & 32)>>5;
		*to++ = (source & 16)>>4;
		*to++ = (source & 8)>>3;
		*to++ = (source & 4)>>2;
		*to++ = (source & 2)>>1;
		*to++ = (source & 1);
	}
}

void convert_2bit_to_8bit(  char *from, char *to,int  pixels )
{
	register int n;
	register char source;
	int bpr = pixels / 4;

	for (n=0; n<bpr;n++)
	{
		source = from[n];
		*to++ = (source & 0xC0)>>6;
		*to++ = (source & 0x30)>>4 ;
		*to++ = (source & 0x0C)>>2;
		*to++ = (source & 0x03);
	}
}

void convert_4bit_to_8bit(  char *from, char *to,int  pixels )
{
	register int n;
	register char source;
	int bpr = pixels / 2;

	for (n=0; n<bpr;n++)
	{
		source = from[n];
		*to++ = (source & 0xF0)>>4;
		*to++ = (source & 0x0F);
	}
}


void convert_1bit_to_16bit( char *from, uint16 *to,int  pixels )
{
	register int n;
	int bytes = pixels / 8;

	for (n=0; n<bytes;n++)
	{
		*to++ = from[n] & 128 ? 0x000000 : 0xFFFFFF;
		*to++ = from[n] & 64 ? 0x000000 : 0xFFFFFF;
		*to++ = from[n] & 32 ? 0x000000 : 0xFFFFFF;
		*to++ = from[n] & 16 ? 0x000000 : 0xFFFFFF;
		*to++ = from[n] & 8 ? 0x000000 : 0xFFFFFF;
		*to++ = from[n] & 4 ? 0x000000 : 0xFFFFFF;
		*to++ = from[n] & 2 ? 0x000000 : 0xFFFFFF;
		*to++ = from[n] & 1 ? 0x000000 : 0xFFFFFF;
	}
}


void convert_1bit_to_32bit( char *from, uint32 *to,int pixels )
{
	register int n;
	int bytes = pixels / 8;

	for (n=0; n<bytes;n++)
	{
		*to++ = from[n] & 128 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 64 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 32 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 16 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 8 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 4 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 2 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 1 ? 0xFF000000 : 0xFFFFFFFF;
	}
}


void convert_1bit_to_8bit_8pixels( char *from, double *to,int  pixels )
{
	register int n;
	register double *lookup64bit = (double *) lookup16bit;
	register uint32 units = pixels / 8;

	for (n=0; n<units;n++)
	{
		*to++ = lookup64bit[from[n]];
	}
}

void convert_2bit_to_8bit_8pixels( uint16 *from, double *to,int  pixels )
{
	register int n;
	register double *lookup64bit = (double *) lookup16bit;
	register uint32 units = pixels / 8;

	for (n=0; n<units;n++)
	{
		*to++ = lookup64bit[from[n]];
	}
}

void convert_4bit_to_8bit_4pixels( uint16 *from, uint32 *to,int  pixels )		// 4 pixels, 4 bytes, == uint32
{
	register int n;
	register uint32 *lookup32bit = (uint32 *) lookup16bit;
	register uint32 units = pixels / 4;

	for (n=0; n<units;n++)
	{
		*to++ = lookup32bit[from[n]];
	}
}


void convert_8bit_to_32bit_db( char *from, uint32 *to,int  pixels )
{
	register int n;
	register int v;

	for (n=0; n<pixels;n++)
	{
		v = vpal32[from[n]];
		*to++=v;
		*to++=v;
	}
}

/*
void convert_8bit_to_32bit_asm( char *from, uint32 *to,int  pixels )
{
	asm
	(
		"li 9,0 \n"

		"loop: \n"
			"lbz		0,0(%1) \n"
			"slwi		0,0,2\n"
			"add		3,%0,0 \n"
			"lwz		0,0(3) \n"
			"stw		0,0(%2) \n"

			"lbz		0,1(%1) \n"
			"slwi		0,0,2\n"
			"add		3,%0,0 \n"
			"lwz		0,0(3) \n"
			"stw		0,4(%2) \n"

			"lbz		0,2(%1) \n"
			"slwi		0,0,2\n"
			"add		3,%0,0 \n"
			"lwz		0,0(3) \n"
			"stw		0,8(%2) \n"

			"lbz		0,3(%1) \n"
			"slwi		0,0,2\n"
			"add		3,%0,0 \n"
			"lwz		0,0(3) \n"
			"stw		0,12(%2) \n"

			"addi	%2,%2,16 \n"
			"addi	%1,%1,4 \n"
			"addi	9,9,4 \n"
		"cmpw	cr7,9,%3 \n"
		"blt+	cr7,loop"

		::"r" (pal), "r" (from), "r" (to), "r" (pixels): "r9","r0","r3"
	);
}
*/

void convert_8bit_to_16bit( char *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		to[n] = vpal16[from[n]];
	}
}

void convert_2bit_lookup_to_16bit(  char *from, uint16 *to,int  pixels )
{
	int bpr = pixels/4;
	int n;
	register unsigned short source;

	for (n=0; n<bpr;n++)
	{
		source = from[n];

		*to++ = vpal16[ (source & 0xC0) >>6];
		*to++ = vpal16[ (source & 0x30) >> 4];
		*to++ = vpal16[ (source & 0x0C) >>2];
		*to++ = vpal16[ source & 0x03];

	}
}



void convert_4bit_lookup_to_16bit(  char *from, uint16 *to,int  pixels )
{
	int bpr = pixels/2;
	register char *from_to;
	register unsigned int source;

	from_to = from + bpr;

	for (; from<from_to;from++)
	{
		source = *from;
		*to++ = vpal16[ (source & 0xF0) >>4];
		*to++ = vpal16[ source & 0x0F];
	}
}

void convert_4bit_lookup_to_16bit_2pixels(  char *from, uint32 *to,int  pixels )
{
	int bpr = pixels/2;
	register char *from_to;
	register unsigned int source;

	from_to = from + bpr;

	for (; from<from_to;from++)
	{
		*to++ = vpal32[ *from ];
	}
}

void convert_8bit_lookup_to_16bit(  char *from, uint16 *to,int  pixels )
{
	int n;

	for (n=0; n<pixels;n++)
	{
		to[n] = vpal16[ from[n] ];
	}
}

void convert_8bit_lookup_to_16bit_2pixels(  uint16 *from, uint32 *to,int  pixels )
{
	int packs = pixels / 2;
	int n;

	for (n=0; n<packs;n++)
	{
		to[n] = vpal32[ from[n] ];
	}
}

void convert_4bit_lookup_to_32bit_2pixels(  unsigned char *from, double *to,int  pixels )
{
	double *vpal64 = (double *) vpal32;	// need to use doubles to get true 64bit registers.
	int bpr = pixels/2;
	register unsigned char *from_to;

	from_to = from + bpr;

	for (; from<from_to;from++)
	{
		*to++ = vpal64[ *from ];
	}
}

void convert_8bit_lookup_to_32bit_2pixels(  uint16 *from, double *to,int  pixels )
{
	double *vpal64 = (double *) vpal32;	// need to use doubles to get true 64bit registers.
	int packs = pixels / 2;
	int n;

	for (n=0; n<packs;n++)
	{
		to[n] = vpal64[ from[n] ];
	}
}

void convert_15bit_to_16bit_le(  uint16 *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		
		//        R          G         B
		// %11111 11111 00000

		rg = (rgb & 0x007FE0) << 1;
		b = (rgb & 0x00001F) ;
		rgb =  rg | b;

		to[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void convert_15bit_to_16bit_be(  uint16 *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		rg = (rgb & 0x007FC0) << 1;
		b = (rgb & 0x00001F) ;
		to[n] =  rg | b;
	}
}


void init_lookup_15bit_to_16bit_le(  )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	if (lookup16bit == NULL) lookup16bit = (uint16 *) malloc(65535 * sizeof(uint16));
	if (lookup16bit == NULL) return;

	for (n=0; n<65535;n++)
	{
		rg = (n & 0x007FE0) << 1;
		b = (n & 0x00001F) ;
		rgb =  (rg | b); 

		lookup16bit[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void init_lookup_15bit_to_16bit_be(  )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	if (lookup16bit == NULL) lookup16bit = (uint16 *) malloc(65535 * sizeof(uint16));
	if (lookup16bit == NULL) return;

	for (n=0; n<65535;n++)
	{
		rg = (n & 0x007FE0) << 1;
		b = (n & 0x00001F) ;
		lookup16bit[n] = (rg | b);
	}
}

void convert_16bit_lookup_to_16bit(  uint16 *from, uint16 *to,int  pixels )
{
	register int n;

	for (n=0; n<pixels;n++)
	{
		to[n] = lookup16bit[ from[n] ];
	}
}



void convert_32bit_to_16bit_le( uint32 *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		r = (rgb & 0xF80000) >> 8;
		g = (rgb & 0x00FC00) >> 5;
		b = (rgb & 0x0000F8) >> 3;
		rgb =  r | g | b;

		to[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void convert_32bit_to_16bit_be( uint32 *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		r = (rgb & 0xF80000) >> 8;
		g = (rgb & 0x00FC00) >> 5;
		b = (rgb & 0x0000F8) >> 3;
		to[n] =  r | g | b;
	}
}

void convert_2bit_to_32bit(  char *from, uint32 *to,int  pixels )
{
	register char source;
	int n;
	int bpr = pixels /4 ;

	for (n=0; n<bpr;n++)
	{
		source = from[n];
		*to++ = vpal32[ (source & 0xC0) >> 6];
		*to++ = vpal32[ (source & 0x30) >> 4 ];
		*to++ = vpal32[ (source & 0xC) >> 2];
		*to++ = vpal32[ (source & 0x3) ];
	}
}

void convert_4bit_to_32bit(  char *from, uint32 *to,int  pixels )
{
	register char source;
	int n;
	int bpr = pixels /2 ;

	for (n=0; n<bpr;n++)
	{
		source = from[n];
		*to++ = vpal32[ source & 0xF0 >> 4];
		*to++ = vpal32[ source & 0xF ];
	}
}


void convert_8bit_to_32bit(  char *from, uint32 *to,int  pixels )
{
	int n;

	for (n=0; n<pixels;n++)
	{
		to[n] = vpal32[from[n]];
	}
}

void convert_15bit_to_32bit( uint16 *from, uint32 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		r = (rgb & 0x007C00) << 9;
		g = (rgb & 0x0003E0) << 6;
		b = (rgb & 0x00001F) << 3;
		to[n] = 0xFF000000 | r | g | b;
	}
}

void convert_16bit_to_32bit( uint16 *from, uint32 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		r = (rgb & 0x00F800) << 8;
		g = (rgb & 0x0007E0) << 5;
		b = (rgb & 0x00001F) << 3;
		to[n] = 0xFF000000 | r | g | b;
	}
}

void convert_copy_8bit( char *from, char *to,int  pixels )
{
	CopyMemQuick( from,  to,  pixels );
}

void convert_copy_16bit(  char *from, char *to,int  pixels )
{
	CopyMemQuick( from,  to,  pixels*2 );
}

void convert_copy_32bit( char *from, char *to,int  pixels )
{
	CopyMemQuick( from,  to,  pixels*4 );
}



#include <proto/exec.h>
#include <proto/dos.h>

extern BPTR video_debug_out;

char convert_1bit_to_32bit_asm( ULONG *pal, char *from, uint32 *to,int  bytes )
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


void convert_1bit_to_8bit( ULONG *pal, char *from, char *to,int  pixels )
{
	register int n;
	int bytes = pixels / 8;

	for (n=0; n<bytes;n++)
	{
		*to++ = (from[n] & 128) >>7;
		*to++ = (from[n] & 64)>>6;
		*to++ = (from[n] & 32)>>5;
		*to++ = (from[n] & 16)>>4;
		*to++ = (from[n] & 8)>>3;
		*to++ = (from[n] & 4)>>2;
		*to++ = (from[n] & 2)>>1;
		*to++ = (from[n] & 1);
	}
}

void convert_1bit_to_16bit( ULONG *pal, char *from, uint16 *to,int  pixels )
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


void convert_1bit_to_32bit( ULONG *pal, char *from, uint32 *to,int pixels )
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

void convert_8bit_to_32bit_db( ULONG *pal, char *from, uint32 *to,int  pixels )
{
	register int n;
	register int v;

	for (n=0; n<pixels;n++)
	{
		v = pal[from[n]];
		*to++=v;
		*to++=v;
	}
}

void convert_8bit_to_32bit_asm( ULONG *pal, char *from, uint32 *to,int  pixels )
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

void convert_8bit_to_16bit( ULONG *pal, char *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = pal[from[n]];
		r = (rgb & 0xF80000) >> 8;
		g = (rgb & 0x00FC00) >> 5;
		b = (rgb & 0x0000F8) >> 3;
		rgb =  r | g | b;

		to[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void convert_15bit_to_16bit( ULONG *pal, uint16 *from, uint16 *to,int  pixels )
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
		rgb =  rg | b;

		to[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}


void convert_32bit_to_16bit( ULONG *pal, uint32 *from, uint16 *to,int  pixels )
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


void convert_8bit_to_32bit( ULONG *pal, char *from, uint32 *to,int  pixels )
{
	int n;

	for (n=0; n<pixels;n++)
	{
		to[n] = pal[from[n]];
	}
}

void convert_16bit_to_32bit( ULONG *pal, uint16 *from, uint32 *to,int  pixels )
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

void convert_15bit_to_32bit( ULONG *pal, uint16 *from, uint32 *to,int  pixels )
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


void convert_copy_8bit( ULONG *pal, char *from, char *to,int  pixels )
{
	CopyMemQuick( from,  to,  pixels );
}

void convert_copy_16bit( ULONG *pal, char *from, char *to,int  pixels )
{
	CopyMemQuick( from,  to,  pixels*2 );
}

void convert_copy_32bit( ULONG *pal, char *from, char *to,int  pixels )
{
	CopyMemQuick( from,  to,  pixels*4 );
}

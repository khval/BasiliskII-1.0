
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>


void set_fn_set_palette2( uint32 macMode, uint32 PixelFormat)
{
	if (vpal16)
	{
		FreeVec(vpal16);
		vpal16 = NULL;
	}

	if (vpal32) 
	{
		FreeVec(vpal32);
		vpal32 = NULL;
	}

	switch (PixelFormat)
	{
		case PIXF_NONE:	// not RTG format.
		case PIXF_CLUT: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = NULL;		// not supported yet.
				break;

		case PIXF_R5G6B5:
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
	
				switch (macMode)
				{
					case VDEPTH_4BIT:
						set_palette_fn = set_vpal_4bit_to_16bit_be_2pixels;
						vpal32 = (uint32 *) AllocShared ( 4 * 16 * 16 );	// (input 2 x 4bit pixels) , 256 combos. (output 2 x 16bit pixels).
						if (vpal32) memset( vpal32, 0, 4 * 16 * 16 );
						break;

					case VDEPTH_8BIT:
						set_palette_fn = set_vpal_8bit_to_16bit_be_2pixels;
						vpal32 = (uint32 *) AllocShared ( 4 * 256 * 256 );	// (input 2 x 8bit pixels) , 65536 combos. (output 2 x 16bit pixels).
						if (vpal32) memset( vpal32, 0, 4 * 256 * 256 );
						break;

					default:
						set_palette_fn = set_vpal_16bit_be;	
						vpal16 = (uint16 *) AllocShared ( 2 * 256);	// 2 pixels , 256 colors, 1 x 16 bit pixel
				}
				break;

		case PIXF_R5G6B5PC:	
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);

				switch (macMode)
				{
					case VDEPTH_4BIT:
						set_palette_fn = set_vpal_4bit_to_16bit_le_2pixels;
						vpal32 = (uint32 *) AllocShared ( 4 * 16 * 16 );	// (input 2 x 4bit pixels) , 256 combos. (output 2 x 16bit pixels).
						if (vpal32) memset( vpal32, 0, 4 * 16 * 16 );
						break;

					case VDEPTH_8BIT:
						set_palette_fn = set_vpal_8bit_to_16bit_le_2pixels;
						vpal32 = (uint32 *) AllocShared ( 4 * 256 * 256 );	// (input 2 x 8bit pixels) , 65536 combos. (output 2 x 16bit pixels).
						if (vpal32) memset( vpal32, 0, 4 * 256 * 256 );
						break;

					default:
						set_palette_fn = set_vpal_16bit_le;	
						vpal16 = (uint16 *) AllocShared ( 2 * 256);	// 1 pixels , 256 colors, 1 x 16 bit pixel
						break;
				}
				break;

		case PIXF_A8R8G8B8: 
			if (video_debug_out) D(FPrintf)( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);

				switch (macMode)
				{
					case VDEPTH_1BIT:
						set_palette_fn = set_vpal_1bit_to_32bit_8pixels;
						vpal32 = (uint32 *) AllocShared (256 * 4 * 8 );	// (input 8 x 1bit pixels) , 256 combos. (output 8 x 32bit pixels).
						if (vpal32) memset( vpal32, 0, 256 * 4  * 8 );
						break;

					case VDEPTH_2BIT:
						set_palette_fn = set_vpal_2bit_to_32bit_be_4pixels;
						vpal32 = (uint32 *) AllocShared (256 * 4 * 4 );	// (input 4 x 2bit pixels) , 256 combos. (output 4 x 32bit pixels).
						if (vpal32) memset( vpal32, 0, 256 * 4  * 4 );
						break;

					case VDEPTH_4BIT:
						set_palette_fn = set_vpal_4bit_to_32bit_be_2pixels;
						vpal32 = (uint32 *) AllocShared (8 * 256 );	// (input 2 x 4bit pixels) , 256 combos. (output 2 x 32bit pixels).
						if (vpal32) memset( vpal32, 0, 8 * 256 );
						break;

					case VDEPTH_8BIT:
						set_palette_fn = set_vpal_8bit_to_32bit_be_2pixels;
						vpal32 = (uint32 *) AllocShared ( 8 * 256 * 256  );	// 2 input pixel , 256 colors,  2 x 32bit output pixel. (0.5Mb)
						break;

					default:
						set_palette_fn = set_vpal_32bit_be;
						vpal32 = (uint32 *) AllocShared ( 4 * 256);	// 1 pixels , 256 colors, 1 x 32bit pixel
						break;
				}
				break;

		case PIXF_B8G8R8A8: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);

				switch (macMode)
				{
					case VDEPTH_1BIT:
						set_palette_fn = set_vpal_1bit_to_32bit_8pixels;
						vpal32 = (uint32 *) AllocShared (256 * 4 * 8 );	// (input 2 x 4bit pixels) , 256 combos. (output 8 x 32bit pixels).
						if (vpal32) memset( vpal32, 0, 256 * 4  * 8 );
						break;

					case VDEPTH_2BIT:
						set_palette_fn = set_vpal_2bit_to_32bit_le_4pixels;
						vpal32 = (uint32 *) AllocShared (256 * 4 * 4 );	// (input 4 x 2bit pixels) , 256 combos. (output 4 x 32bit pixels).
						if (vpal32) memset( vpal32, 0, 256 * 4  * 4 );
						break;

					case VDEPTH_4BIT:
						set_palette_fn = set_vpal_4bit_to_32bit_le_2pixels;
						vpal32 = (uint32 *) AllocShared (8 * 256 );	// (input 2 x 4bit pixels) , 256 combos. (output 2 x 32bit pixels).
						if (vpal32) memset( vpal32, 0, 8 * 256 );
						break;

					case VDEPTH_8BIT:
						set_palette_fn = set_vpal_8bit_to_32bit_le_2pixels;
						vpal32 = (uint32 *) AllocShared ( 8 * 256 * 256  );	// 2 input pixel , 256 colors,  2 x 32bit output pixel. (0.5Mb)
						break;

					default:
						set_palette_fn = set_vpal_32bit_le;
						vpal32 = (uint32 *) AllocShared ( 4 * 256);	// 1 pixels , 256 colors, 1 x 32bit pixel
						break;
				}
				break;

		default:
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = NULL;
	}
}


inline uint16 __pal_to_16bit(uint8 *pal, int num)
{
	register unsigned int n;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	n = num *3;
	r = pal[n] & 0xF8;		// 4+1 = 5 bit
	g = pal[n+1]  & 0xFC;	// 4+2 = 6 bit
	b = pal[n+2]  & 0xF8;	// 4+1 = 5 bit
	return ( (r << 8) | (g << 3) | (b >> 3));
}



void set_vpal_16bit_le(uint8 *pal, uint32 num)
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	// Convert palette to 32 bits virtual buffer.

	for (num=0;num<256;num++)
	{
		rgb = __pal_to_16bit(pal, num);
		vpal16[num] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}
}

void set_vpal_8bit_to_16bit_le_2pixels(uint8 *pal, uint32 num1)
{
	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	if (video_debug_out) FPrintf( video_debug_out, "%s\n",__FUNCTION__);

	// pixel [0..0],[0..255]

	for (num1=0;num1<256;num1++)
	{
		rgb = __pal_to_16bit(pal, num1);
		vpal32[num1] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}

	// pixel [0..255],[0..255]

	for (index=0;index<256*256;index++)
	{
		num2 = (index & 0x00FF);
		num1 = (index & 0xFF00) >> 8;

		vpal32[index] =  ((vpal32[num1] & 0xFFFF) << 16) | (vpal32[num2] & 0xFFFF) ;
	}

}

 void set_vpal_4bit_to_16bit_le_2pixels(uint8 *pal, uint32 num1)
{
	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	if (video_debug_out) FPrintf( video_debug_out, "%s\n",__FUNCTION__);

	// pixel [0..0],[0..15]

	for (num1=0;num1<16;num1++)
	{
		rgb = __pal_to_16bit(pal, num1);
		vpal32[num1] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}

	// pixel [0..15],[0..15]

	for (index=0;index<256;index++)		// because color 0 is not always black... (we need to redo the first 16 colors also)
	{
		num1 = (index & 0xF0) >> 4;
		num2 = (index & 0x0F);

		vpal32[index] =  ((vpal32[num1] & 0xFFFF) << 16) | (vpal32[num2] & 0xFFFF) ;
	}
}

 void set_vpal_4bit_to_16bit_be_2pixels(uint8 *pal, uint32 num1)
{
	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	if (video_debug_out) FPrintf( video_debug_out, "%s\n",__FUNCTION__);

	// pixel [0..0],[0..15]

	for (num1=0;num1<16;num1++)
	{
		vpal32[num1] = __pal_to_16bit(pal, num1);
	}

	// pixel [0..15],[0..15]

	for (index=0;index<256;index++)		// because color 0 is not always black... (we need to redo the first 16 colors also)
	{
		num1 = (index & 0xF0) >> 4;
		num2 = (index & 0x0F);

		vpal32[index] =  ((vpal32[num1] & 0xFFFF) << 16) | (vpal32[num2] & 0xFFFF) ;
	}
}


void set_vpal_16bit_be(uint8 *pal, uint32 num)
{
	// Convert palette to 32 bits virtual buffer.

	for (num=0;num<256;num++)
	{
		vpal16[num] = __pal_to_16bit(pal, num);
	}
}

void set_vpal_8bit_to_16bit_be_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..256]

	for (num1=0;num1<256;num1++)
	{
		vpal32[num1] = __pal_to_16bit(pal, num1);
	}

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num2 = index & 255;
		num1 = (index >> 8) & 255;

		vpal32[index] =  vpal32[num1] << 16 | vpal32[num2];
	}
}


#define _BW32(n) (n ? 0xFF000000 : 0xFFFFFFFF)  
#define _LE_ARGB(n) 0xFF + (pal[(n)] << 8) +  (pal[(n)+1] << 16) + (pal[(n)+2] << 24) 
#define _BE_ARGB(n) 0xFF000000 + (pal[(n)] << 16) +  (pal[(n)+1] << 8) + pal[(n)+2]   

void set_vpal_1bit_to_32bit_8pixels(uint8 *pal, uint32 num)
{
	int x;
	int n;
	uint32 *d;

	pal[0]=0xFF;
	pal[1]=0xFF;
	pal[2]=0xFF;
	pal[3]=0x00;
	pal[4]=0x00;
	pal[5]=0x00;

	for (num=0;num<256;num++)
	{
		n=num*3;
		d = vpal32 + num*8;

		d[0]= _BW32( (num>>7)&1 );
		d[1]= _BW32( (num>>6)&1 );
		d[2]= _BW32( (num>>5)&1 );
		d[3]= _BW32( (num>>4)&1 );
		d[4]= _BW32( (num>>3)&1 );
		d[5]= _BW32( (num>>2)&1 );
		d[6]= _BW32( (num>>1)&1 );
		d[7]= _BW32( num&1);
	}
}

void set_vpal_2bit_to_32bit_le_4pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2,num3,num4;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..256]

	for (num1=0;num1<256;num1++)
	{
		vpal32[num1*2] = _LE_ARGB( num1 );
	}

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num1 = ( (index >> 6) & 0x3)*3;
		num2 = ( (index >> 4) & 0x3)*3;
		num3 = ( (index >> 2) & 0x3)*3;
		num4 = ( (index) & 0x3)*3;

		vpal32[index*4] = vpal32[num1*2+1];
		vpal32[index*4+1] = vpal32[num2*2+1];
		vpal32[index*4+2] = vpal32[num2*2+1];
		vpal32[index*4+3] = vpal32[num2*2+1];
	}
}


void set_vpal_2bit_to_32bit_be_4pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2,num3,num4;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256;index++)
	{
		num1 = ( (index >> 6) & 0x3)*3;
		num2 = ( (index >> 4) & 0x3)*3;
		num3 = ( (index >> 2) & 0x3)*3;
		num4 = ( (index) & 0x3)*3;

		vpal32[index*4] = 0xFF000000 + (pal[num1] << 16) +  (pal[num1+1] << 8) + pal[num1+2]  ;  // ARGB

		vpal32[index*4+1] = 0xFF000000 + (pal[num2] << 16) +  (pal[num2+1] << 8) + pal[num2+2]  ;  // ARGB
		vpal32[index*4+2] = 0xFF000000 + (pal[num3] << 16) +  (pal[num3+1] << 8) + pal[num3+2]  ;  // ARGB
		vpal32[index*4+3] = 0xFF000000 + (pal[num4] << 16) +  (pal[num4+1] << 8) + pal[num4+2]  ;  // ARGB
	}
}

void set_vpal_8bit_to_32bit_le_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..256]

	for (num1=0;num1<256;num1++)
	{
		vpal32[num1*2] = _LE_ARGB( num1 );
	}

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num1 = (index >> 8) & 255;
		num2 = index & 255;

		vpal32[index*2] = vpal32[num1*2+1];
		vpal32[index*2+1] = vpal32[num2*2+1];
	}
}


void set_vpal_4bit_to_32bit_le_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256;index++)
	{
		num1 = ((index >> 4) & 0xF)*3;
		num2 = (index & 0xF)*3;

		vpal32[index*2] = 0xFF000000 + (pal[num1] << 16) +  (pal[num1+1] << 8) + pal[num1+2]  ;  // ARGB
		vpal32[index*2+1] = 0xFF000000 + (pal[num2] << 16) +  (pal[num2+1] << 8) + pal[num2+2]  ;  // ARGB
	}
}


void set_vpal_4bit_to_32bit_be_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256;index++)
	{
		num1 = ((index >> 4) & 0xF)*3;
		num2 = (index & 0xF)*3;

		vpal32[index*2] = 0xFF000000 + (pal[num1] << 16) +  (pal[num1+1] << 8) + pal[num1+2]  ;  // ARGB
		vpal32[index*2+1] = 0xFF000000 + (pal[num2] << 16) +  (pal[num2+1] << 8) + pal[num2+2]  ;  // ARGB
	}
}


void set_vpal_8bit_to_32bit_be_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num1 = ((index >> 8) & 255)*3;
		num2 = (index & 255)*3;

		vpal32[index*2] = 0xFF000000 + (pal[num1] << 16) +  (pal[num1+1] << 8) + pal[num1+2]  ;  // ARGB
		vpal32[index*2+1] = 0xFF000000 + (pal[num2] << 16) +  (pal[num2+1] << 8) + pal[num2+2]  ;  // ARGB
	}
}

void set_vpal_32bit_le(uint8 *pal, uint32 num)
{
	int n;

	for (num=0;num<256;num++)
	{
		n = num *3;		// BGRA
		vpal32[num]=0xFF + (pal[n] << 8) +  (pal[n+1] << 16) + (pal[n+2] << 24) ;
	}
}

void set_vpal_32bit_be(uint8 *pal, uint32 num)
{
	int n;

	for (num=0;num<256;num++)
	{
		n = num *3;		// ARGB
		vpal32[num]=0xFF000000 + (pal[n] << 16) +  (pal[n+1] << 8) + pal[n+2]  ;
	}
}



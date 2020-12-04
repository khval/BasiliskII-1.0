
struct video_convert_names
{
	const char *name;
	void *fn;
};

const char *get_name_converter_fn_ptr( void *fn_ptr);

void init_lookup_15bit_to_16bit_le();
void init_lookup_15bit_to_16bit_be();

void convert_copy_8bit( char *from, char *to,int  pixels );
void convert_copy_16bit( char *from, char *to,int  pixels );
void convert_copy_32bit( char *from, char *to,int  pixels );

void convert_1bit_to_8bit( char *from, char *to,int  pixels );
void convert_2bit_to_8bit( char *from, char *to,int  pixels );
void convert_4bit_to_8bit( char *from, char *to,int  pixels );

void convert_15bit_to_16bit_le( uint16 *from, uint16 *to,int  pixels );
void convert_32bit_to_16bit_le( uint32 *from, uint16 *to,int  pixels );

void convert_15bit_to_16bit_be( uint16 *from, uint16 *to,int  pixels );
void convert_32bit_to_16bit_be( uint32 *from, uint16 *to,int  pixels );

void convert_1bit_to_16bit( char *from, uint16 *to,int  pixels );
void convert_2bit_lookup_to_16bit( char *from, uint16 *to,int  pixels );
void convert_4bit_lookup_to_16bit( char *from, uint16 *to,int  pixels );
void convert_4bit_lookup_to_16bit_2pixels( char *from, uint32 *to,int  pixels );		// from 2x 4bit pixels to 2x 16bit pixels.
void convert_8bit_lookup_to_16bit( char *from, uint16 *to,int  pixels );
void convert_8bit_lookup_to_16bit_2pixels( uint16 *from, uint32 *to,int  pixels );		// from 2x 8bit pixels to 2x 16bit pixels.
void convert_16bit_lookup_to_16bit( uint16 *from, uint16 *to,int  pixels );

void convert_1bit_to_32bit( char *from, uint32 *to,int  pixels );
void convert_2bit_to_32bit( char *from, uint32 *to,int  pixels );
void convert_4bit_to_32bit( char *from, uint32 *to,int  pixels );
void convert_8bit_to_32bit_db( char *from, uint32 *to,int  pixels );
void convert_8bit_to_32bit_asm( char *from, uint32 *to,int  pixels );
void convert_8bit_to_32bit( char *from, uint32 *to,int  pixels );
void convert_15bit_to_32bit( uint16 *from, uint32 *to,int  pixels );
void convert_16bit_to_32bit( uint16 *from, uint32 *to,int  pixels );
extern uint16 *lookup16bit;



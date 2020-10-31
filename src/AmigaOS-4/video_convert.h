
void convert_copy_8bit( ULONG *pal, char *from, char *to,int  pixels );
void convert_copy_16bit( ULONG *pal, char *from, char *to,int  pixels );
void convert_copy_32bit( ULONG *pal, char *from, char *to,int  pixels );

void convert_1bit_to_8bit( ULONG *pal, char *from, char *to,int  pixels );

void convert_1bit_to_16bit( ULONG *pal, char *from, uint16 *to,int  pixels );
void convert_8bit_to_16bit( ULONG *pal, char *from, uint16 *to,int  pixels );
void convert_32bit_to_16bit( ULONG *pal, uint32 *from, uint16 *to,int  pixels );

void convert_1bit_to_32bit( ULONG *pal, char *from, uint32 *to,int  pixels );
void convert_8bit_to_32bit_db( ULONG *pal, char *from, uint32 *to,int  pixels );
void convert_8bit_to_32bit_asm( ULONG *pal, char *from, uint32 *to,int  pixels );
void convert_8bit_to_32bit( ULONG *pal, char *from, uint32 *to,int  pixels );
void convert_16bit_to_32bit( ULONG *pal, uint16 *from, uint32 *to,int  pixels );




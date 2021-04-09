
extern char *last_asl_path;

typedef struct 
{
	int modeID;
	int width,height;
} ASL_ModeID;

void imagefile_asl( int win_id, int str_gad_id, BOOL opt );
// void DO_ASL(int win_id, int str_gad_id, BOOL opt);

void imagefile_asl( void (*action) (const char *name), BOOL opt );
void DO_ASL( const char *(*current_str) (), void (*action) ( const char *value ), BOOL drawers_only );
void DO_ASL_MODE_ID(  	int (*current_ModeID) (), 	void (*action) (ASL_ModeID  *asl_modeid) );


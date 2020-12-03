
class Amiga_monitor_desc : public monitor_desc {
public:
	Amiga_monitor_desc(const vector<video_mode> &available_modes, video_depth default_depth, uint32 default_id, int default_display_type) 
		:  monitor_desc(available_modes, default_depth, default_id), display_type(default_display_type) {};
	~Amiga_monitor_desc() {};

	virtual void switch_to_current_mode(void);
	virtual void set_palette(uint8 *pal, int num);

	bool video_open(void);
	void video_close(void);
public:
	int display_type;		// See enum above
};

/*
 *  Display "driver" classes
 */

class driver_base
{

	public:
		driver_base(Amiga_monitor_desc &m);
		virtual ~driver_base();

		virtual void set_palette(uint8 *pal, int num) {};
		virtual int draw() { return 1;}
		virtual void kill_gfx_output();
		virtual void restore_gfx_output();

	public:
		Amiga_monitor_desc &monitor;	// Associated video monitor
		const video_mode &mode;		// Video mode handled by the driver
		BOOL init_ok;			// Initialization succeeded (we can't use exceptions because of -fomit-frame-pointer)
		struct Window *the_win;
		char *VIDEO_BUFFER;
		video_depth depth;

		void (*convert)( char *from, char *to,int  bytes );
		struct Screen *the_screen;
		struct BitMap *the_bitmap;
};

class driver_window : public driver_base {
public:
	driver_window(Amiga_monitor_desc &m, const video_mode &mode);
	~driver_window();

	virtual void set_palette(uint8 *pal, int num);
	virtual int draw();
	virtual void kill_gfx_output();
	virtual void restore_gfx_output();

private:
	LONG black_pen, white_pen;

};

class driver_window_comp : public driver_base {
public:
	driver_window_comp(Amiga_monitor_desc &m, const video_mode &mode );
	~driver_window_comp();

	virtual void set_palette(uint8 *pal, int num);
	virtual int draw();
	virtual void kill_gfx_output();
	virtual void restore_gfx_output();

private:
	LONG black_pen, white_pen;

};

class driver_screen : public driver_base {
public:
	driver_screen(Amiga_monitor_desc &m, const video_mode &mode, ULONG mode_id);
	~driver_screen();
	virtual int draw();

	void set_palette(uint8 *pal, int num);

private:

	struct BitMap *the_bitmap;
	char *to_mem;
	int to_bpr;
};

extern void (*do_draw) ( driver_base *drv );

enum 
{
	rm_internal,
	rm_wpa,
	rm_direct
};

#define convert_type void (*)( char*, char*, int)

extern void *get_convert( uint32_t scr_depth, uint32_t depth );
extern void *get_convert_v2( uint32_t scr_depth, uint32_t depth );

extern uint32 *vpal32;
extern uint16 *vpal16;

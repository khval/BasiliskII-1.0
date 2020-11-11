
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
		virtual struct BitMap *get_bitmap() { return NULL; };
		virtual int draw() { return 1;}
		virtual int get_width() { return mode.x; }
		virtual int get_height() { return mode.y; }
		virtual void kill_gfx_output();
		virtual void restore_gfx_output();

	public:
		Amiga_monitor_desc &monitor;	// Associated video monitor
		const video_mode &mode;		// Video mode handled by the driver
		BOOL init_ok;			// Initialization succeeded (we can't use exceptions because of -fomit-frame-pointer)
		struct Window *the_win;
		char *VIDEO_BUFFER;
		video_depth depth;

		void (*convert)( ULONG *pal, char *from, char *to,int  bytes );
		struct Screen *the_screen;
		struct BitMap *the_bitmap;
};

class driver_window : public driver_base {
public:
	driver_window(Amiga_monitor_desc &m, int width, int height);
	~driver_window();

	virtual void set_palette(uint8 *pal, int num);
	virtual struct BitMap *get_bitmap() { return the_bitmap; };
	virtual int draw();
	virtual void kill_gfx_output();
	virtual void restore_gfx_output();

private:
	LONG black_pen, white_pen;

};

class driver_window_comp : public driver_base {
public:
	driver_window_comp(Amiga_monitor_desc &m, int width, int height);
	~driver_window_comp();
	virtual int draw();

	virtual void set_palette(uint8 *pal, int num);
	virtual struct BitMap *get_bitmap() { return the_bitmap; };
	virtual void kill_gfx_output();
	virtual void restore_gfx_output();

private:
	LONG black_pen, white_pen;
	struct BitMap *the_bitmap;

	char *to_mem;
	int to_bpr;

};

class driver_screen : public driver_base {
public:
	driver_screen(Amiga_monitor_desc &m, ULONG mode_id, int w, int h);
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

#define convert_type void (*)(ULONG*, char*, char*, int)

extern void *get_convert( uint32_t scr_depth, uint32_t depth );

extern uint32 vpal32[256];
extern uint16 vpal16[256];

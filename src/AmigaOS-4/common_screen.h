// ------------------------------------------------------------------------
//    Copyright: Kjetil Hvalstrand (LiveForIt), license MIT.
//    this file is used many different projects,
//-------------------------------------------------------------------------

struct windowclass
{
	struct Window *win;
	ULONG window_left;
	ULONG window_top;
	ULONG window_width;
	ULONG window_height;
	ULONG ModeID;
};

extern struct windowclass window_save_state;

extern void open_fullscreen(ULONG ModeID);
extern void save_window_attr(windowclass *self);
extern void close_engine_window();
extern bool open_engine_window( int window_left, int window_top, int window_width, int window_height );

extern void enable_Iconify( struct Window *My_Window );
extern void dispose_Iconify();
extern void empty_que(struct MsgPort *win_port);

extern APTR video_mutex ;

// deviceClip.cpp.
//
// is written by Kjetil Hvalstrand, (C) 2021,
// this code snippet is under MIT license

class DeviceClip
{
	public:

		struct MsgPort *mp ;
		struct IOClipReq *io;
		bool open ;

		void Init();
		DeviceClip(int unit);
		~DeviceClip();
		void initIO();
		int WriteFTXT(char *txt);
		int WriteFTXT(char *txt, int slen);
		int QuaryFTXT();	// returns length if true :-)
		bool  writeDone();
		bool write(uint8 * ptr, int len);
		bool read(uint8 *ptr,int len);
		void readDone();	// read until everything is read.
};


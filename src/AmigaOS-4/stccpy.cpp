
	void stccpy( char * des , char * src, int len ) 
	{
		int n;
		for (n=0;n<len;n++)
		{
			des[n] = src[n];
			if (src[n]==0) break;
		}
	}
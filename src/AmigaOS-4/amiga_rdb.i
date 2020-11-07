#include <devices/hardblocks.h>
#include <exec/errors.h>

#define env_blocks_per_track		5
#define env_res_blocks			6
#define env_LowCyl				9
#define env_HiCyl				10


void print_error(int error)
{
	switch (error)
	{
		case IOERR_OPENFAIL:
			printf(" device/unit failed to open\n" ); 
			break;

		case IOERR_ABORTED:
			printf(" request terminated early [after AbortIO()]\n" ); 
			break;

		case IOERR_NOCMD:
			printf(" command not supported by device\n" ); 
			break;

		case IOERR_BADLENGTH:
			printf(" not a valid length (usually IO_LENGTH)\n" );
			break;

		case IOERR_BADADDRESS:
			printf(" invalid address (misaligned or bad range)\n" );
			break;

		case IOERR_UNITBUSY:
			printf(" device opens ok, but requested unit is busy\n" );
			break;

		case IOERR_SELFTEST:
			printf(" hardware failed self-test\n" ); 
			break;
	}
};

void dump_diskinfo( struct RigidDiskBlock *diskinfo )
{
	char *rdb_id;

	rdb_id = (char *) diskinfo;

	printf("\n%c%c%c%c:\n\n", rdb_id[0], rdb_id[1], rdb_id[2], rdb_id[3]);
  
	switch ( diskinfo -> rdb_HostID ) 
 	{ 
		case 7 : printf("Host_type:  Harddrive / zip\n"); 
			break;
		default: printf("Host ID:    %lu\n", diskinfo -> rdb_HostID ); 
	}
  
	printf("Block Size:	%lu\n", diskinfo -> rdb_BlockBytes );
	printf("Flags:		%lu\n", diskinfo -> rdb_Flags );  
  
	if ( ~ diskinfo -> rdb_PartitionList )
	{ printf("Partitionlist:	%lu\n", diskinfo -> rdb_PartitionList ); }
  
	if ( ~ diskinfo -> rdb_FileSysHeaderList )
	{ printf("file sys hdr list:	%lu\n", diskinfo -> rdb_FileSysHeaderList ); }
}

void dump_diskgeo( struct RigidDiskBlock *diskgeo )
{
	printf("Cylinders		%lu\n", diskgeo -> rdb_Cylinders ) ;
	printf("sectors			%lu\n", diskgeo -> rdb_Sectors ) ;
	printf("heads			%lu\n", diskgeo -> rdb_Heads ) ;
	printf("interleave		%lu\n", diskgeo -> rdb_Interleave ) ;
	printf("parking_zone	%lu\n", diskgeo -> rdb_Park ) ;
	printf("write_pri_comp	%lu\n", diskgeo -> rdb_WritePreComp ) ;
	printf("reduced_write	%lu\n", diskgeo -> rdb_ReducedWrite ) ;
	printf("step rate		%lu\n", diskgeo -> rdb_StepRate ) ;
}

void readblock(struct IOExtTD *tio,char *buffer,uint32 blocksize, long long int block)
{
	long long int offset;

	offset = (long long int) block * (long long int) blocksize;

	tio -> iotd_Req.io_Command	= NSCMD_TD_READ64;
	tio -> iotd_Req.io_Length	= blocksize;
	tio -> iotd_Req.io_Actual		= offset >> 32 ;
	tio -> iotd_Req.io_Offset		= offset;
	tio -> iotd_Req.io_Data		= buffer;

/* 	tio -> iotd_Req.io_Flags		= IOF_QUICK; */

	DoIO( (struct IORequest *) tio);

//	printf("Read byte %lld blocksize %lu to buffer 0x%X error %d\n",offset, blocksize, buffer, tio -> iotd_Req.io_Error );

	print_error(tio -> iotd_Req.io_Error);
}

int get_blockinfo(struct IOExtTD *tio, void *block, uint32 blocksize, long long int offset)
{ 
	int okey = 1;

	readblock(tio,(char *) block,blocksize,offset);
  	return okey;
}

struct RigidDiskBlock *get_diskinfo(struct IOExtTD *tio, uint32 blocksize )
{ 
	struct RigidDiskBlock *diskinfo;

 	diskinfo = (struct RigidDiskBlock *) AllocVec(blocksize, MEMF_SHARED);
	if (!diskinfo) return NULL;

	readblock(tio,(char *) diskinfo, blocksize, 0);
	return diskinfo;
}

int get_blocks(char *device,unsigned int unit,char *devicename,ULONG *startblock,ULONG *sizeblocks,ULONG *blocksize)
{
	int error = true;
	int part_block = 0;
	int count = 0;
	int found = 0;
	uint32 blocks_per_cyl = 0;
 
	struct RigidDiskBlock	*diskinfo;
	struct PartitionBlock	*part = NULL;

	char			*block = NULL;
	struct MsgPort *msgport = NULL;
	struct IOExtTD	*tio = NULL;
 
	msgport	= CreateMsgPort();

	if (msgport)
	{
		tio		= (struct IOExtTD *) CreateIORequest(msgport, sizeof(struct IOExtTD));
		if (tio) error = OpenDevice(device, unit, (struct IORequest *) tio, 0);
	}

//	printf("devicename: %s\n",devicename);
//	printf("device: %s\n",device);
//	printf("unit: %d\n",unit);
//	printf("blocksize: %lu\n", *blocksize);
//	printf("Error: 0x%x\n\n", error);

	if ( !error )
	{
		diskinfo = get_diskinfo( tio, *blocksize );
		part_block = diskinfo -> rdb_PartitionList ;
		*blocksize = diskinfo -> rdb_BlockBytes;

//		dump_diskinfo( diskinfo );
//		dump_diskgeo( diskinfo );

		// memoryBlock = AllocVec(byteSize, attributes)

		block = (char *) AllocVec(*blocksize, MEMF_SHARED | MEMF_CLEAR );
		if (block)
		{
			count = 90000;
			do
			{
				readblock( tio, block, *blocksize, part_block );
				part = (struct PartitionBlock *) block;

				switch (part -> pb_ID)
				{
					case 0x50415254: // 'PART'
				
						if (strcasecmp(devicename,&part -> pb_DriveName[1])==0)
						{
						 	blocks_per_cyl = diskinfo -> rdb_Sectors * diskinfo -> rdb_Heads ;

							*startblock = part -> pb_Environment[env_LowCyl] * blocks_per_cyl  ;

							*sizeblocks =	(part -> pb_Environment[env_HiCyl] 
										-part -> pb_Environment[env_LowCyl])
										*blocks_per_cyl;

//							printf("startblock: %lu\n",*startblock);
//							printf("sizeblocks: %lu\n",*sizeblocks);
							found = 1;
						}

						part_block = part -> pb_Next;
						break;

					case 0x5244534b:

						printf("\nRDB block? retry!\n");
						break;

					default:

						printf("Part ID: %x\n",part -> pb_ID);
						part_block = 0;
						break;
				}
				count--;

			} while ((part_block>0)&&(count>0));
		}

		if (diskinfo)	{	FreeVec(diskinfo); diskinfo = NULL;	}
		if (block)		{	FreeVec(block); block = NULL;		}

	}

	if (tio)
	{
		if (!error) CloseDevice( (IORequest*) tio);
		DeleteIORequest( (struct IORequest *) tio);
		tio = NULL;
	}

	if (msgport)	
	{
		DeleteMsgPort(msgport); 
		msgport = NULL;
	}

	return found;
}

long long int get_rdb_partition_size(char *str)
{
	long long int size;
	char dev_name[256];
	char vol_devicename[256];
	ULONG dev_unit = 0, dev_flags = 0, dev_start = 0, dev_size = 16, dev_bsize = 512;

	if (sscanf(str, "/dev/%[^/]/%ld/%[^/]", dev_name, &dev_unit, vol_devicename) < 2)
			return NULL;

	size = 0;

	if (get_blocks(dev_name,dev_unit,vol_devicename,&dev_start,&dev_size,&dev_bsize)) 
	{
		size = ((long long int) dev_size) * ( (long long int) dev_bsize );
	}

	return size;
}

long long int get_file_size(char *name)
{
	BPTR fd;
	long long int size;

	size = 0;
	if (fd = Open(name,MODE_OLDFILE))
	{
		size = GetFileSize(fd);		
		Close(fd);
	}
	return size;
}

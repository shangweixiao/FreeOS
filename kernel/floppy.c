#include <type.h>
#include <8259A.h>
#include <klibs.h>
#include <klibc.h>
#include <floppy.h>

/*
 * globals used by 'result()'
 */
#define MAX_REPLIES 7
static U8 reply_buffer[MAX_REPLIES];
#define ST0 (reply_buffer[0])
#define ST1 (reply_buffer[1])
#define ST2 (reply_buffer[2])
#define ST3 (reply_buffer[3])

#define FLOPPY_RESET_OK  1
#define FLOPPY_READ_OK 2
#define FLOPPY_SEEK_OK 3
#define FLOPPY_RECAL_OK 4
volatile U8 floppy_int_ok=0;

static U8 current_drive = 0;
static U8 current_track = 255;
static U8 current_DOR =  0x1C;
static U8 cur_spec1 = 0x44;
static U8 seek_track=0;
static struct floppy_pram{
	U32 size,sect,head,track,stretch;
	U8 gap,rate,spec1;
}floppy_type={2880,18,2,80,0,0x1B,0x00,0xCF};

static void floppy_output_byte(U8 byte);
static void floppy_setup_dma(S8 *buf);
static void floppy_setup_rw(S8 head,S8 track,S8 sector,S8 *buf);

static void floppy_delay(void)
{
	delay(10000);
}
static S32 result(void)
{
	S32 i = 0, counter, status;

	for (counter = 0 ; counter < 10000 ; counter++)
	{
		status = in_byte((U32)FD_STATUS)&(STATUS_DIR|STATUS_READY|STATUS_BUSY);
		if (status == STATUS_READY)
		{
			return i;
		}
		if (status == (STATUS_DIR|STATUS_READY|STATUS_BUSY))
		{
			if (i >= MAX_REPLIES)
			{
				break;
			}
			reply_buffer[i++] = in_byte((U32)FD_DATA);
		}
	}
	put_str("Getstatus times out");
	return -1;
}
void floppy_rw_int(void)
{
	floppy_int_ok = FLOPPY_READ_OK;

	put_str("read finish!\n");
	if (result() != 7 || (ST0 & 0xf8) || (ST1 & 0xbf) || (ST2 & 0x73))
	{
		put_num(ST0,1);put_char(' ');
		put_num(ST1,1);put_char(' ');
		put_num(ST2,1);put_char(' ');
	}
}
void floppy_seek_int(void)
{
	floppy_int_ok = FLOPPY_SEEK_OK;
	floppy_output_byte(FD_SENSEI);
	if(2 != result() || (ST0 & 0xF8) != 0x20 || ST1 != seek_track)
	{
		put_str("seek error\n");
		return;
	}
	put_str("seek ok!TRACK:");
	put_num(ST1,2);
	current_track = ST1;
	put_char('\n');
}
void floppy_recal_int(void)
{
	floppy_output_byte(FD_SENSEI);
	if(result() != 2 || (ST0 & 0xE0) == 0x60)
	{
		put_str("Need reset");
	}
	floppy_int_ok = FLOPPY_RECAL_OK;
	put_str("recal OK\n");
}
void floppy_reset_int(void)
{
	floppy_output_byte(FD_SENSEI);
	(void) result();
	floppy_output_byte(FD_SPECIFY);
	floppy_output_byte(cur_spec1);		/* hut etc */
	floppy_output_byte(4);			/* Head load time =6ms, DMA */
	put_str("reset ok\n");
	floppy_int_ok = FLOPPY_RESET_OK;
}

void (*do_irq)(void) = NULL;
void floppy_irq(S32 no)
{
	if(do_irq)
	{
		(*do_irq)();
	}
}

void init_floppy(void)
{
	S32 type;
	type = CMOS_READ(0x10);
	out_byte((U32)FD_DOR,current_DOR);
	floppy_delay();
	set_irq_handler(FLOPPY_IRQ,floppy_irq);
	enable_irq(FLOPPY_IRQ);
}
static void floppy_output_byte(U8 byte)
{
	S32 c;
	U8 sta=0;

	for(c=0;c<10000;c++)
	{
		sta = in_byte((U32)FD_STATUS) & (STATUS_READY | STATUS_DIR);
		if(STATUS_READY == sta)
		{
			out_byte((U32)FD_DATA,byte);
			return;
		}
	}
	put_num(sta,1);
	put_str("floppy_output_byte error\n");
}
#define FLOPPY_DMA 2
#define DMA_MODE_READ 0x46
static void floppy_setup_dma(S8 *buf)
{
	U32 addr = (U32)buf;

	disable_int();
	out_byte((U32)0x0A,4|FLOPPY_DMA);
	out_byte((U32)0x0C,0);
	out_byte((U32)0x0C,DMA_MODE_READ);
	out_byte((U32)0x0B,DMA_MODE_READ);
	out_byte((U32)0x81,(addr>>16)&0x0F);
	out_byte((U32)0x04,addr&0xFF);
	out_byte((U32)0x04,(addr>>8)&0xFF);
	out_byte((U32)0x05,1023&0xFF);
	out_byte((U32)0x05,(1023>>8)&0xFF);
	out_byte((U32)0x0A,FLOPPY_DMA);
	floppy_delay();
	enable_int();
}

static void floppy_setup_rw(S8 head,S8 track,S8 sector,S8 *buf)
{
	floppy_setup_dma(buf);
	do_irq = floppy_rw_int;
	floppy_output_byte(FD_READ);
	floppy_output_byte(current_drive);
	floppy_output_byte(track);
	floppy_output_byte(head);
	floppy_output_byte(sector);
	floppy_output_byte(2);
	floppy_output_byte(floppy_type.sect);
	floppy_output_byte(floppy_type.gap);
	floppy_output_byte(0xFF);
}
void floppy_seek(S32 block)
{
	S8 head,track,sector;

	floppy_recalibrate();

	sector = block % floppy_type.sect;
	block /= floppy_type.sect;
	track = block / floppy_type.head;
	head = block % floppy_type.head;

	put_num(head,1);put_char(' ');
	put_num(track,1);put_char(' ');
	put_num(sector,1);put_char(' ');
	put_char('\n');

	seek_track = track;
	do_irq = floppy_seek_int;
	floppy_output_byte(FD_SEEK);
	floppy_output_byte((head<<2)|current_drive);
	floppy_output_byte(track);

	while(FLOPPY_SEEK_OK != floppy_int_ok)
	{
		floppy_delay();
	}
}
void floppy_reset(void)
{
	disable_int();
	do_irq = floppy_reset_int;
	out_byte((U32)FD_DOR,current_DOR & ~0x04);
	floppy_delay();

	out_byte((U32)FD_DOR,current_DOR);
	enable_int();
	while(FLOPPY_RESET_OK != floppy_int_ok)
	{
		floppy_delay();
	}

}
void floppy_read(S32 block,S32 size,S8 *buf)
{
	S8 head,track,sector;

	sector = block % floppy_type.sect;
	block /= floppy_type.sect;
	track = block / floppy_type.head;
	head = block % floppy_type.head;

	put_num(head,1);put_char(' ');
	put_num(track,1);put_char(' ');
	put_num(sector,1);put_char(' ');
	put_char('\n');

	sector += 1;
	floppy_setup_rw(head,track,sector,buf);
	while(FLOPPY_READ_OK != floppy_int_ok)
	{
		floppy_delay();
	}
}

void floppy_recalibrate(void)
{
	do_irq = floppy_recal_int;
	out_byte((U32)FD_DATA,FD_RECALIBRATE);
	out_byte((U32)FD_DATA,0);
	while(FLOPPY_RECAL_OK != floppy_int_ok)
	{
		floppy_delay();
	}
}


#include <type.h>
#include <klibs.h>
#include <klibc.h>
#include <8259A.h>
#include <protect.h>
#include <keyboard.h>
#include <floppy.h>
#include <keymap.h>
#include <process.h>

static KB_INPUT kb_in;
U8		code_with_E0	= FALSE;
U8		shift_l;		/* l shift state	*/
U8		shift_r;		/* r shift state	*/
U8		alt_l;			/* l alt state		*/
U8		alt_r;			/* r left state		*/
U8		ctrl_l;			/* l ctrl state		*/
U8		ctrl_r;			/* l ctrl state		*/
U8		caps_lock;		/* Caps Lock		*/
U8		num_lock;		/* Num Lock		*/
U8		scroll_lock;		/* Scroll Lock		*/
U32		column		= 0;	/* keyrow[column]*/

void keyboard_read(void);
void task_tty(void);
U8 get_byte_from_kb_buf(void);
void key_process(U32 key);
void kb_ack(void);
void kb_wait(void);
void set_leds(void);

void init_keyboard(void)
{
	kb_in.count = 0;
	kb_in.p_head = kb_in.p_tail = kb_in.buf;

	set_irq_handler(KEYBOARD_IRQ,keyboard_irq);
	enable_irq(KEYBOARD_IRQ);

	creat_process(task_tty,4096);
}
void keyboard_irq(S32 no)
{
	U8 scan_code = in_byte(KB_DATA);

	if(kb_in.count < KB_IN_BYTES)
	{
		*kb_in.p_head++ = scan_code;
		if(kb_in.p_head == (kb_in.buf+KB_IN_BYTES))
		{
			kb_in.p_head = kb_in.buf;
		}
		kb_in.count += 1;
	}
}

void task_tty(void)
{
	while(1)
	{
		keyboard_read();
	}
}

void keyboard_read(void)
{
	U8 scan_code;
	U8 make,caps;
	U32 key;
	U32 *keyrow;

	if(kb_in.count > 0)
	{
		scan_code = get_byte_from_kb_buf();
		if(0xE1 == scan_code)
		{
		}
		else if(0xE0 == scan_code)
		{
			code_with_E0 = TRUE;
		}
		if ((key != PAUSEBREAK) && (key != PRINTSCREEN))
		{
			make = (scan_code & FLAG_BREAK ? FALSE : TRUE);
			keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];

			column = 0;

			caps = shift_l || shift_r;
			if (caps_lock)
			{
				if ((keyrow[0] >= 'a') && (keyrow[0] <= 'z'))
				{
					caps = !caps;
				}
			}
			if (caps)
			{
				column = 1;
			}

			if (code_with_E0)
			{
				column = 2;
				code_with_E0 = FALSE;
			}

			key = keyrow[column];

			switch(key) 
			{
				case SHIFT_L:
					shift_l	= make;
					break;
				case SHIFT_R:
					shift_r	= make;
					break;
				case CTRL_L:
					ctrl_l	= make;
					break;
				case CTRL_R:
					ctrl_r	= make;
					break;
				case ALT_L:
					alt_l	= make;
					break;
				case ALT_R:
					alt_l	= make;
					break;
				case CAPS_LOCK:
					if (make)
					{
						caps_lock   = !caps_lock;
						set_leds();
					}
					break;
				case NUM_LOCK:
					if (make)
					{
						num_lock    = !num_lock;
						set_leds();
					}
					break;
				case SCROLL_LOCK:
					if (make)
					{
						scroll_lock = !scroll_lock;
						set_leds();
					}
					break;
				default:
					break;
			}
		}
		if(make)
		{
			U8 pad = FALSE;

			if ((key >= PAD_SLASH) && (key <= PAD_9))
			{
				pad = TRUE;
				switch(key) 
				{	/* '/', '*', '-', '+', and 'Enter' in num pad  */
					case PAD_SLASH:
						key = '/';
						break;
					case PAD_STAR:
						key = '*';
						break;
					case PAD_MINUS:
						key = '-';
						break;
					case PAD_PLUS:
						key = '+';
						break;
					case PAD_ENTER:
						key = ENTER;
						break;
					default:	/* keys whose value depends on the NumLock */
						if (num_lock)
						{	/* '0' ~ '9' and '.' in num pad */
							if ((key >= PAD_0) && (key <= PAD_9))
							{
								key = key - PAD_0 + '0';
							}
							else if (key == PAD_DOT)
							{
								key = '.';
							}
						}
						else
						{
							switch(key)
							{
								case PAD_HOME:
									key = HOME;
									break;
								case PAD_END:
									key = END;
									break;
								case PAD_PAGEUP:
									key = PAGEUP;
									break;
								case PAD_PAGEDOWN:
									key = PAGEDOWN;
									break;
								case PAD_INS:
									key = INSERT;
									break;
								case PAD_UP:
									key = UP;
									break;
								case PAD_DOWN:
									key = DOWN;
									break;
								case PAD_LEFT:
									key = LEFT;
									break;
								case PAD_RIGHT:
									key = RIGHT;
									break;
								case PAD_DOT:
									key = DELETE;
									break;
								default:
									break;
							}
						}
						break;
				}
			}
			key |= shift_l	? FLAG_SHIFT_L	: 0;
			key |= shift_r	? FLAG_SHIFT_R	: 0;
			key |= ctrl_l	? FLAG_CTRL_L	: 0;
			key |= ctrl_r	? FLAG_CTRL_R	: 0;
			key |= alt_l	? FLAG_ALT_L	: 0;
			key |= alt_r	? FLAG_ALT_R	: 0;
			key |= pad	? FLAG_PAD	: 0;

			key_process(key);
		}
	}
}

void key_process(U32 key)
{
	char output[2]={0,0};
	if(!(key & FLAG_EXT))
	{
		output[0] = key & 0xFF;
		put_str(output);
	}
	put_num(key,4);put_char(' ');
	put_num((FLAG_CTRL_L|FLAG_ALT_L)+DELETE,4);put_char(' ');

	if((key & (FLAG_CTRL_L | FLAG_CTRL_R)) && (key & (FLAG_ALT_L | FLAG_ALT_R)))
	{
		reboot();
	}
}
U8 get_byte_from_kb_buf(void)
{
	U8 scan_code;
	if(kb_in.count > 0)
	{
		disable_int();
		scan_code = *(kb_in.p_tail);
		kb_in.p_tail += 1;
		if(kb_in.p_tail == kb_in.buf + KB_IN_BYTES)
		{
			kb_in.p_tail = kb_in.buf;
		}
		kb_in.count -= 1;
		enable_int();
	}
	return(scan_code);
}
void set_leds(void)
{
	U8 leds = (caps_lock << 2) | (num_lock << 1) | scroll_lock;

	kb_wait();
	out_byte(KB_DATA, LED_CODE);
	kb_ack();

	kb_wait();
	out_byte(KB_DATA, leds);
	kb_ack();
}
void kb_wait(void)
{
	U8 kb_stat;

	do
	{
		kb_stat = in_byte(KB_CMD);
	} while (kb_stat & 0x02);
}


void kb_ack(void)
{
	U8 kb_read;

	do
	{
		kb_read = in_byte(KB_DATA);
	} while (kb_read != KB_ACK);
}


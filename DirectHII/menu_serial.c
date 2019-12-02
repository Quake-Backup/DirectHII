
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"

//=============================================================================
/* SERIAL CONFIG MENU */

// yuck, this bullshit
extern int m_multiplayer_cursor;
extern int m_net_cursor;
extern qboolean	m_return_onerror;
extern char		m_return_reason [32];
extern m_state_t m_return_state;


int		serialConfig_cursor;
int		serialConfig_cursor_table[] = {48, 64, 80, 96, 112, 132};
#define	NUM_SERIALCONFIG_CMDS	6

static int ISA_uarts[]	= {0x3f8, 0x2f8, 0x3e8, 0x2e8};
static int ISA_IRQs[]	= {4, 3, 4, 3};
int serialConfig_baudrate[] = {9600, 14400, 19200, 28800, 38400, 57600};

int		serialConfig_comport;
int		serialConfig_irq ;
int		serialConfig_baud;
char	serialConfig_phone[16];

void M_Menu_SerialConfig_f (void)
{
	int		n;
	int		port;
	int		baudrate;
	qboolean	useModem;

	key_dest = key_menu;
	m_state = m_serialconfig;
	m_soundlevel = m_sound_menu2;

	if (JoiningGame && SerialConfig)
		serialConfig_cursor = 4;
	else
		serialConfig_cursor = 5;

	(*GetComPortConfig) (0, &port, &serialConfig_irq, &baudrate, &useModem);

	// map uart's port to COMx
	for (n = 0; n < 4; n++)
		if (ISA_uarts[n] == port)
			break;

	if (n == 4)
	{
		n = 0;
		serialConfig_irq = 4;
	}

	serialConfig_comport = n + 1;

	// map baudrate to index
	for (n = 0; n < 6; n++)
		if (serialConfig_baudrate[n] == baudrate)
			break;

	if (n == 6)
		n = 5;

	serialConfig_baud = n;

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_SerialConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*directModem;

	ScrollTitle ("gfx/menu/title4.lmp");

	p = Draw_CachePic ("gfx/menu/title4.lmp");
	basex = (320 - p->width) / 2;

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";

	if (SerialConfig)
		directModem = "Modem";
	else
		directModem = "Direct Connect";

	M_Print (basex, 32, va ("%s - %s", startJoin, directModem));
	basex += 8;

	M_Print (basex, serialConfig_cursor_table[0], "Port");
	M_DrawTextBox (160, 40, 4, 1);
	M_Print (168, serialConfig_cursor_table[0], va ("COM%u", serialConfig_comport));

	M_Print (basex, serialConfig_cursor_table[1], "IRQ");
	M_DrawTextBox (160, serialConfig_cursor_table[1] - 8, 1, 1);
	M_Print (168, serialConfig_cursor_table[1], va ("%u", serialConfig_irq));

	M_Print (basex, serialConfig_cursor_table[2], "Baud");
	M_DrawTextBox (160, serialConfig_cursor_table[2] - 8, 5, 1);
	M_Print (168, serialConfig_cursor_table[2], va ("%u", serialConfig_baudrate[serialConfig_baud]));

	if (SerialConfig)
	{
		M_Print (basex, serialConfig_cursor_table[3], "Modem Setup...");

		if (JoiningGame)
		{
			M_Print (basex, serialConfig_cursor_table[4], "Phone number");
			M_DrawTextBox (160, serialConfig_cursor_table[4] - 8, 16, 1);
			M_Print (168, serialConfig_cursor_table[4], serialConfig_phone);
		}
	}

	if (JoiningGame)
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5] - 8, 7, 1);
		M_Print (basex + 8, serialConfig_cursor_table[5], "Connect");
	}
	else
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5] - 8, 2, 1);
		M_Print (basex + 8, serialConfig_cursor_table[5], "OK");
	}

	M_DrawCharacter (basex - 8, serialConfig_cursor_table [serialConfig_cursor], 12 + ((int) (realtime * 4) & 1));

	if (serialConfig_cursor == 4)
		M_DrawCharacter (168 + 8 * strlen (serialConfig_phone), serialConfig_cursor_table [serialConfig_cursor], 10 + ((int) (realtime * 4) & 1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_SerialConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;
		serialConfig_cursor--;

		if (serialConfig_cursor < 0)
			serialConfig_cursor = NUM_SERIALCONFIG_CMDS - 1;

		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;
		serialConfig_cursor++;

		if (serialConfig_cursor >= NUM_SERIALCONFIG_CMDS)
			serialConfig_cursor = 0;

		break;

	case K_LEFTARROW:

		if (serialConfig_cursor > 2)
			break;

		m_soundlevel = m_sound_menu3;

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport--;

			if (serialConfig_comport == 0)
				serialConfig_comport = 4;

			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq--;

			if (serialConfig_irq == 6)
				serialConfig_irq = 5;

			if (serialConfig_irq == 1)
				serialConfig_irq = 7;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud--;

			if (serialConfig_baud < 0)
				serialConfig_baud = 5;
		}

		break;

	case K_RIGHTARROW:

		if (serialConfig_cursor > 2)
			break;

forward:
		m_soundlevel = m_sound_menu3;

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport++;

			if (serialConfig_comport > 4)
				serialConfig_comport = 1;

			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq++;

			if (serialConfig_irq == 6)
				serialConfig_irq = 7;

			if (serialConfig_irq == 8)
				serialConfig_irq = 2;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud++;

			if (serialConfig_baud > 5)
				serialConfig_baud = 0;
		}

		break;

	case K_ENTER:

		if (serialConfig_cursor < 3)
			goto forward;

		m_soundlevel = m_sound_menu2;

		if (serialConfig_cursor == 3)
		{
			(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

			M_Menu_ModemConfig_f ();
			break;
		}

		if (serialConfig_cursor == 4)
		{
			serialConfig_cursor = 5;
			break;
		}

		// serialConfig_cursor == 5 (OK/CONNECT)
		(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

		M_ConfigureNetSubsystem ();

		if (StartingGame)
		{
			M_Menu_GameOptions_f ();
			break;
		}

		m_return_state = m_state;
		m_return_onerror = true;
		key_dest = key_game;
		m_state = m_none;

		if (SerialConfig)
			Cbuf_AddText (va ("connect \"%s\"\n", serialConfig_phone));
		else
			Cbuf_AddText ("connect\n");

		break;

	case K_BACKSPACE:

		if (serialConfig_cursor == 4)
		{
			if (strlen (serialConfig_phone))
				serialConfig_phone[strlen (serialConfig_phone)-1] = 0;
		}

		break;

	default:

		if (key < 32 || key > 127)
			break;

		if (serialConfig_cursor == 4)
		{
			l = strlen (serialConfig_phone);

			if (l < 15)
			{
				serialConfig_phone[l+1] = 0;
				serialConfig_phone[l] = key;
			}
		}
	}

	if (DirectConfig && (serialConfig_cursor == 3 || serialConfig_cursor == 4))
		if (key == K_UPARROW)
			serialConfig_cursor = 2;
		else
			serialConfig_cursor = 5;

	if (SerialConfig && StartingGame && serialConfig_cursor == 4)
		if (key == K_UPARROW)
			serialConfig_cursor = 3;
		else
			serialConfig_cursor = 5;
}


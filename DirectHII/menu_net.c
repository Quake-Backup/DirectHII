
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"

//=============================================================================
/* NET MENU */

int	m_net_cursor = 0;
int m_net_items;
int m_net_saveHeight;

char *net_helpMessage [] =
{
	/* .........1.........2.... */
	"                        ",
	" Two computers connected",
	"   through two modems.  ",
	"                        ",

	"                        ",
	" Two computers connected",
	" by a null-modem cable. ",
	"                        ",

	" Novell network LANs    ",
	" or Windows 95 DOS-box. ",
	"                        ",
	"(LAN=Local Area Network)",

	" Commonly used to play  ",
	" over the Internet, but ",
	" also used on a Local   ",
	" Area Network.          "
};

void M_Menu_Net_f (void)
{
	key_dest = key_menu;
	m_state = m_net;
	m_soundlevel = m_sound_menu2;
	m_net_items = 4;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;

	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;

	ScrollTitle ("gfx/menu/title4.lmp");

	f = 32;

#ifndef _WIN32

	if (serialAvailable)
	{
		p = Draw_CachePic ("gfx/netmen1.lmp");
	}
	else
	{
		p = Draw_CachePic ("gfx/dim_modm.lmp");
	}

	if (p)
		M_DrawTransPic (72, f, p);

#endif


	f += 19;

#ifndef _WIN32

	if (serialAvailable)
	{
		p = Draw_CachePic ("gfx/netmen2.lmp");
	}
	else
	{
		p = Draw_CachePic ("gfx/dim_drct.lmp");
	}

	if (p)
		M_DrawTransPic (72, f, p);

#endif


	f += 19;
	/*	rjr
		if (ipxAvailable)
			p = Draw_CachePic ("gfx/netmen3.lmp");
		else
			p = Draw_CachePic ("gfx/dim_ipx.lmp");
	*/
	//	M_DrawTransPic (72, f, p);
	M_DrawBigString (72, f, "IPX");


	f += 19;
	/* rjr
		if (tcpipAvailable)
			p = Draw_CachePic ("gfx/netmen4.lmp");
		else
			p = Draw_CachePic ("gfx/dim_tcp.lmp");
	*/
	//	M_DrawTransPic (72, f, p);
	M_DrawBigString (72, f, "TCP/IP");

	f = (320 - 26 * 8) / 2;
	M_DrawTextBox (f, 134, 24, 4);
	f += 8;
	M_Print (f, 142, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 150, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 158, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 166, net_helpMessage[m_net_cursor*4+3]);

	f = (int) (realtime * 10) % 8;
	M_DrawTransPic (43, 24 + m_net_cursor * 20, Draw_CachePic (va ("gfx/menu/menudot%i.lmp", f + 1)));
}

void M_Net_Key (int k)
{
again:

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_DOWNARROW:

		// Tries to re-draw the menu here, and m_net_cursor could be set to -1
		//		m_soundlevel = m_sound_menu1;
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;

		break;

	case K_UPARROW:

		//		m_soundlevel = m_sound_menu1;
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;

		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;

		switch (m_net_cursor)
		{
		case 0:
			M_Menu_SerialConfig_f ();
			break;

		case 1:
			M_Menu_SerialConfig_f ();
			break;

		case 2:
			M_Menu_LanConfig_f ();
			break;

		case 3:
			M_Menu_LanConfig_f ();
			break;

		case 4:
			// multiprotocol
			break;
		}
	}

	if (m_net_cursor == 0 && !serialAvailable)
		goto again;

	if (m_net_cursor == 1 && !serialAvailable)
		goto again;

	if (m_net_cursor == 2 && !ipxAvailable)
		goto again;

	if (m_net_cursor == 3 && !tcpipAvailable)
		goto again;

	switch (k)
	{
	case K_DOWNARROW:
	case K_UPARROW:
		m_soundlevel = m_sound_menu1;
		break;
	}
}




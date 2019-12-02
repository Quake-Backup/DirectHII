
void M_Menu_Main_f (void);
void M_Main_Draw (void);
void M_Main_Key (int key);

void M_Menu_Difficulty_f (void);
void M_Difficulty_Draw (void);
void M_Difficulty_Key (int key);

void M_Menu_Class_f (void);
void M_Menu_Class2_f (void);
void M_Class_Draw (void);
void M_Class_Key (int key);

void M_Menu_SinglePlayer_f (void);
void M_SinglePlayer_Draw (void);
void M_SinglePlayer_Key (int key);

void M_Menu_Load_f (void);
void M_Load_Draw (void);
void M_Load_Key (int key);

void M_Menu_Save_f (void);
void M_Save_Draw (void);
void M_Save_Key (int key);

void M_Menu_MLoad_f (void);
void M_MLoad_Draw (void);
void M_MLoad_Key (int key);

void M_Menu_MSave_f (void);
void M_MSave_Draw (void);
void M_MSave_Key (int key);

void M_Menu_MultiPlayer_f (void);
void M_Menu_Setup_f (void);
void M_Menu_Net_f (void);
void M_Menu_Options_f (void);
void M_Menu_Keys_f (void);
void M_Menu_Video_f (void);
void M_Menu_Help_f (void);
void M_Menu_Quit_f (void);
void M_Menu_SerialConfig_f (void);
void M_Menu_ModemConfig_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_Search_f (void);
void M_Menu_ServerList_f (void);

void M_MultiPlayer_Draw (void);
void M_Setup_Draw (void);
void M_Net_Draw (void);
void M_Options_Draw (void);
void M_Keys_Draw (void);
void M_Video_Draw (void);
void M_Help_Draw (void);
void M_Quit_Draw (void);
void M_SerialConfig_Draw (void);
void M_ModemConfig_Draw (void);
void M_LanConfig_Draw (void);
void M_GameOptions_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);

void M_MultiPlayer_Key (int key);
void M_Setup_Key (int key);
void M_Net_Key (int key);
void M_Options_Key (int key);
void M_Keys_Key (int key);
void M_Video_Key (int key);
void M_Help_Key (int key);
void M_Quit_Key (int key);
void M_SerialConfig_Key (int key);
void M_ModemConfig_Key (int key);
void M_LanConfig_Key (int key);
void M_GameOptions_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);


void M_Menu_Options_f (void);
void M_Print (int cx, int cy, char *str);
void M_PrintWhite (int cx, int cy, char *str);
void M_DrawBigString (int x, int y, char *string);
void M_DrawCharacter (int cx, int line, int num);
void M_DrawTransPic (int x, int y, qpic_t *pic);
void M_DrawPic (int x, int y, qpic_t *pic);
void ScrollTitle (char *name);

typedef enum {
	m_none, m_main, m_singleplayer, m_load, m_save, m_multiplayer, m_setup, m_net, m_options, m_video,
	m_keys, m_help, m_quit, m_serialconfig, m_modemconfig, m_lanconfig, m_gameoptions, m_search, m_slist,
	m_class, m_difficulty, m_mload, m_msave
} m_state_t;

extern m_state_t m_state;

typedef enum {
	m_sound_none,
	m_sound_menu1,
	m_sound_menu2,
	m_sound_menu3,
	m_sound_steve
} m_soundlevel_t;

extern m_soundlevel_t m_soundlevel;

void CL_RemoveGIPFiles (char *path);
void Host_WriteConfiguration (char *fname);

extern char *ClassNames[NUM_CLASSES];
extern char *ClassNamesU[NUM_CLASSES];
extern char *DiffNames[NUM_CLASSES][4];

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#define SerialConfig	(m_net_cursor == 0)
#define DirectConfig	(m_net_cursor == 1)
#define	IPXConfig		(m_net_cursor == 2)
#define	TCPIPConfig		(m_net_cursor == 3)

#define NUM_DIFFLEVELS	4
extern int setup_class;

extern char *message, *message2;
extern double message_time;

void M_ConfigureNetSubsystem (void);
void M_Menu_Class_f (void);

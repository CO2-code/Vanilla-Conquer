//
// Spectator and Spawner support for CnCNet compatibility
// Based on ra-patches assembly code, ported to C++
//

#ifndef SPECTATOR_H
#define SPECTATOR_H

#include "house.h"
#include "session.h"

//---------------------------------------------------------------------------
// Spawner data structures - per-player info from SPAWN.INI
//---------------------------------------------------------------------------
struct SpawnerPlayerInfo
{
    char Name[12];
    int Side;          // 0=allies, 1=soviet
    int Color;         // color index
    int Handicap;      // handicap value
    int SpawnLocation; // spawn location override (-1 = default)
    bool IsSpectator;  // whether this player is a spectator
    char Ip[32];       // IP address string
    int Port;          // port number
    bool Ignored;      // whether this player is ignored
};

//---------------------------------------------------------------------------
// Spawner settings from [Settings] section of SPAWN.INI
//---------------------------------------------------------------------------
struct SpawnerSettings
{
    char Name[12];
    int Side;
    int Color;
    int Port;
    bool Bases;
    int Credits;
    bool OreRegenerates;
    bool Crates;
    char Scenario[32];
    int UnitCount;
    int AIPlayers;
    int AIDifficulty;
    int TechLevel;
    bool CaptureTheFlag;
    bool ShroudRegrows;
    int Seed;
    bool SlowUnitBuild;
    int GameSpeed;
    bool IsHost;
    int GameID;
    int NetworkVersionProtocol;
    int MaxAhead;
    int FrameSendRate;
    bool QuickMatch;
    int WOLGameID;
    bool LoadSaveGame;
    int SaveGameNumber;
    bool IsSinglePlayer;
    bool AutoHarvesting;
    int RecordingMode; // 1=record, 4=playback
    char MapHash[64];
    char DebugMapHash[64];

    // Tunnel
    char TunnelIp[32];
    int TunnelPort;
    int TunnelId;

    // Fix flags from SPAWN.INI
    bool SuperTeslaFix;
    bool ChatMuted;
    bool AutoHarvestingEnabled;
    bool AftermathFastBuildSpeed;
    bool FixFormationSpeed;
    bool ReduceSovietMaxAircrafts;
    bool FixRangeExploit;
    bool FixMagicBuild;
    bool ParaBombsInMultiplayer;
    bool FixAIAlly;
    bool MCVUndeploy;
    bool AllyReveal;
    bool ForcedAlliances;
    bool TechCenterBugFix;
    bool BuildOffAlly;
    bool SouthAdvantageFix;
    bool NoScreenShake;
    bool NoTeslaZapEffectDelay;
    bool ShortGame;
    bool DeadPlayersRadar;
    bool PlayerIsGameHost;
};

//---------------------------------------------------------------------------
// Global spawner state
//---------------------------------------------------------------------------
extern bool Spawner_Is_Active;
extern int Spectator_Count;
extern bool Spectators_Array[HOUSE_COUNT];
extern SpawnerPlayerInfo SpawnerPlayers[8]; // Multi1-Multi8
extern SpawnerSettings SpawnerCfg;
extern int HumanPlayers; // number of human players
extern char SpawnerMapHash[64];

//---------------------------------------------------------------------------
// Main spawner functions
//---------------------------------------------------------------------------

// Check if -SPAWN command line argument is present
bool Spawner_Check_Command_Line(void);

// Initialize the spawner from SPAWN.INI. Returns true on success.
// This is the main entry point called from Select_Game().
bool Spawner_Initialize(void);

// Initialize spectator data for a house during game setup
void Spectator_Init_House(HouseClass* house);

// Draw live stats overlay on the sidebar for spectators
void Draw_Live_Stats(void);

// Skip dead/spectator houses when reading object INI data
bool Should_Skip_House(HousesType house);

// Check if the local player is a spectator
bool Is_Local_Player_Spectator(void);

// Get spawn location for a given house
int Spawner_Get_Spawn_Location(HousesType house);

// Apply spawner settings to the current game session
void Spawner_Apply_Session_Settings(void);

// Get the human player count
int Spawner_Get_Human_Player_Count(void);

//---------------------------------------------------------------------------
// Replay spectator support
//---------------------------------------------------------------------------

// Enter spectator mode when watching a replay.
// Marks the local player as spectator and reveals the entire map.
void Spectator_Enter_Replay_Mode(void);

// Called each frame during replay to update spectator overlay.
void Spectator_Replay_Render(void);

#endif // SPECTATOR_H

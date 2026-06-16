//
// Spectator and Spawner support for CnCNet compatibility
// Based on ra-patches assembly code, ported to C++
//
// The CnCNet XNA client launches the game with -SPAWN and writes a SPAWN.INI
// file containing all game settings, player info, and network configuration.
// This module reads that file and sets up the game accordingly.
//

#include "function.h"
#include "spectator.h"
#include <cstring>
#include <cstdlib>

//---------------------------------------------------------------------------
// Global spawner state
//---------------------------------------------------------------------------
bool Spawner_Is_Active = false;
int Spectator_Count = 0;
bool Spectators_Array[HOUSE_COUNT] = {};
SpawnerPlayerInfo SpawnerPlayers[8] = {};
SpawnerSettings SpawnerCfg = {};
int HumanPlayers = 0;
char SpawnerMapHash[64] = {};

//---------------------------------------------------------------------------
// Internal helpers
//---------------------------------------------------------------------------
static bool Spawner_Ini_Loaded = false;

// Forward declarations for INIClass wrapper
static CCINIClass* SpawnIni = nullptr;

static void Spawner_Load_INI(void);
static void Spawner_Read_Settings(void);
static void Spawner_Read_Players(void);
static void Spawner_Read_Spectators(void);
static void Spawner_Read_Spawn_Locations(void);
static void Spawner_Read_House_Colours(void);
static void Spawner_Read_House_Countries(void);
static void Spawner_Read_House_Handicaps(void);
static void Spawner_Read_Tunnel(void);
static void Spawner_Read_Fix_Flags(void);

/***********************************************************************************************
 * Spawner_Check_Command_Line -- Check if -SPAWN argument is present
 *=============================================================================================*/
bool Spawner_Check_Command_Line(void)
{
#ifdef _WIN32
    const char* cmdline = GetCommandLineA();
#else
    // On non-Windows, check argv or environment
    extern int __argc;
    extern char** __argv;
    for (int i = 1; i < __argc; i++) {
        if (_stricmp(__argv[i], "-SPAWN") == 0) {
            return true;
        }
    }
    return false;
#endif

    if (cmdline == nullptr) {
        return false;
    }

    // Search for -SPAWN or /SPAWN in command line
    const char* spawn_arg = strstr(cmdline, "-SPAWN");
    if (spawn_arg == nullptr) {
        spawn_arg = strstr(cmdline, "/SPAWN");
    }
    if (spawn_arg == nullptr) {
        spawn_arg = strstr(cmdline, "-spawn");
    }
    if (spawn_arg == nullptr) {
        spawn_arg = strstr(cmdline, "/spawn");
    }

    return (spawn_arg != nullptr);
}

/***********************************************************************************************
 * Spawner_Load_INI -- Load SPAWN.INI file into memory
 *=============================================================================================*/
static void Spawner_Load_INI(void)
{
    if (SpawnIni != nullptr) {
        return; // already loaded
    }

    CCFileClass file("SPAWN.INI");

    if (!file.Is_Available()) {
        return;
    }

    SpawnIni = new CCINIClass();
    SpawnIni->Load(file, false);
    file.Close();

    Spawner_Ini_Loaded = true;
}

/***********************************************************************************************
 * Spawner_Initialize -- Main spawner initialization entry point
 *
 * Called from Select_Game() when -SPAWN is detected on command line.
 * Reads SPAWN.INI and configures the game for CnCNet play.
 * Returns true if initialization succeeded.
 *=============================================================================================*/
bool Spawner_Initialize(void)
{
    // Check if -SPAWN is on the command line
    if (!Spawner_Check_Command_Line()) {
        return false;
    }

    // Load SPAWN.INI
    Spawner_Load_INI();

    if (!Spawner_Ini_Loaded || SpawnIni == nullptr) {
        return false;
    }

    // Set spawner as active
    Spawner_Is_Active = true;

    // Clear spectator data
    Spectator_Count = 0;
    memset(Spectators_Array, 0, sizeof(Spectators_Array));
    memset(SpawnerPlayers, 0, sizeof(SpawnerPlayers));
    memset(SpawnerMapHash, 0, sizeof(SpawnerMapHash));

    // Read all settings from SPAWN.INI
    Spawner_Read_Settings();
    Spawner_Read_Players();
    Spawner_Read_Spectators();
    Spawner_Read_Spawn_Locations();
    Spawner_Read_House_Colours();
    Spawner_Read_House_Countries();
    Spawner_Read_House_Handicaps();
    Spawner_Read_Tunnel();
    Spawner_Read_Fix_Flags();

    // Count human players
    HumanPlayers = 0;
    for (int i = 0; i < 8; i++) {
        if (SpawnerPlayers[i].Name[0] != '\0') {
            HumanPlayers++;
        }
    }

    // Apply settings to the game session
    Spawner_Apply_Session_Settings();

    return true;
}

/***********************************************************************************************
 * Spawner_Read_Settings -- Read [Settings] section from SPAWN.INI
 *=============================================================================================*/
static void Spawner_Read_Settings(void)
{
    if (SpawnIni == nullptr) return;

    // Player name
    char buf[128];
    memset(buf, 0, sizeof(buf));
    SpawnIni->Get_String("Settings", "Name", "", buf, sizeof(buf));
    strncpy(SpawnerCfg.Name, buf, sizeof(SpawnerCfg.Name) - 1);

    // Side (0=allies, 1=soviet)
    SpawnerCfg.Side = SpawnIni->Get_Int("Settings", "Side", 0);

    // Color
    SpawnerCfg.Color = SpawnIni->Get_Int("Settings", "Color", 0);

    // Port
    SpawnerCfg.Port = SpawnIni->Get_Int("Settings", "Port", 1234);

    // Game options
    SpawnerCfg.Bases = SpawnIni->Get_Bool("Settings", "Bases", true);
    SpawnerCfg.Credits = SpawnIni->Get_Int("Settings", "Credits", 10000);
    SpawnerCfg.OreRegenerates = SpawnIni->Get_Bool("Settings", "OreRegenerates", false);
    SpawnerCfg.Crates = SpawnIni->Get_Bool("Settings", "Crates", false);
    SpawnerCfg.UnitCount = SpawnIni->Get_Int("Settings", "UnitCount", 0);
    SpawnerCfg.AIPlayers = SpawnIni->Get_Int("Settings", "AIPlayers", 0);
    SpawnerCfg.AIDifficulty = SpawnIni->Get_Int("Settings", "AIDifficulty", 2);
    SpawnerCfg.TechLevel = SpawnIni->Get_Int("Settings", "TechLevel", 10);
    SpawnerCfg.CaptureTheFlag = SpawnIni->Get_Bool("Settings", "CaptureTheFlag", false);
    SpawnerCfg.ShroudRegrows = SpawnIni->Get_Bool("Settings", "ShroudRegrows", false);
    SpawnerCfg.Seed = SpawnIni->Get_Int("Settings", "Seed", 0);
    SpawnerCfg.SlowUnitBuild = SpawnIni->Get_Bool("Settings", "SlowUnitBuild", false);
    SpawnerCfg.GameSpeed = SpawnIni->Get_Int("Settings", "GameSpeed", 1);
    SpawnerCfg.IsHost = SpawnIni->Get_Bool("Settings", "IsHost", false);
    SpawnerCfg.GameID = SpawnIni->Get_Int("Settings", "GameID", 0);
    SpawnerCfg.NetworkVersionProtocol = SpawnIni->Get_Int("Settings", "NetworkVersionProtocol", 0);
    SpawnerCfg.MaxAhead = SpawnIni->Get_Int("Settings", "MaxAhead", 0);
    SpawnerCfg.FrameSendRate = SpawnIni->Get_Int("Settings", "FrameSendRate", 4);
    SpawnerCfg.QuickMatch = SpawnIni->Get_Bool("Settings", "QuickMatch", false);
    SpawnerCfg.WOLGameID = SpawnIni->Get_Int("Settings", "WOLGameID", 0);

    // Save game
    SpawnerCfg.LoadSaveGame = SpawnIni->Get_Bool("Settings", "LoadSaveGame", false);
    SpawnerCfg.SaveGameNumber = SpawnIni->Get_Int("Settings", "SaveGameNumber", 1000);
    SpawnerCfg.IsSinglePlayer = SpawnIni->Get_Bool("Settings", "IsSinglePlayer", false);

    // Recording mode
    SpawnerCfg.RecordingMode = SpawnIni->Get_Int("Settings", "recording_mode", 0);

    // Map hash
    memset(buf, 0, sizeof(buf));
    SpawnIni->Get_String("Settings", "MapHash", "", buf, sizeof(buf));
    strncpy(SpawnerMapHash, buf, sizeof(SpawnerMapHash) - 1);
    strncpy(SpawnerCfg.MapHash, buf, sizeof(SpawnerCfg.MapHash) - 1);

    // Debug map hash fallback
    memset(buf, 0, sizeof(buf));
    SpawnIni->Get_String("Debug", "MapHash", "", buf, sizeof(buf));
    strncpy(SpawnerCfg.DebugMapHash, buf, sizeof(SpawnerCfg.DebugMapHash) - 1);

    // Scenario name - try spawnmap.ini first (written by CnCNet XNA client)
    {
        CCFileClass spawnmapFile("spawnmap.ini");
        if (spawnmapFile.Is_Available()) {
            CCINIClass spawnmapIni;
            spawnmapIni.Load(spawnmapFile, false);
            spawnmapFile.Close();
            memset(buf, 0, sizeof(buf));
            spawnmapIni.Get_String("Settings", "Scenario", "", buf, sizeof(buf));
            if (buf[0] == '\0') {
                // Fallback to SPAWN.INI
                SpawnIni->Get_String("Settings", "Scenario", "", buf, sizeof(buf));
            }
            strncpy(SpawnerCfg.Scenario, buf, sizeof(SpawnerCfg.Scenario) - 1);
        } else {
            // No spawnmap.ini - read from SPAWN.INI
            SpawnIni->Get_String("Settings", "Scenario", "", buf, sizeof(buf));
            strncpy(SpawnerCfg.Scenario, buf, sizeof(SpawnerCfg.Scenario) - 1);
        }
    }

    // Auto harvesting
    SpawnerCfg.AutoHarvesting = SpawnIni->Get_Bool("Settings", "AutoHarvesting", true);
}

/***********************************************************************************************
 * Spawner_Read_Players -- Read [Other1]..[Other8] sections for opponent info
 *=============================================================================================*/
static void Spawner_Read_Players(void)
{
    if (SpawnIni == nullptr) return;

    static const char* section_names[] = {
        "Other1", "Other2", "Other3", "Other4",
        "Other5", "Other6", "Other7", "Other8"
    };

    char buf[128];

    for (int i = 0; i < 8; i++) {
        // Check if this section has a Name entry
        memset(buf, 0, sizeof(buf));
        SpawnIni->Get_String(section_names[i], "Name", "", buf, sizeof(buf));

        if (buf[0] == '\0') {
            // No more players
            break;
        }

        strncpy(SpawnerPlayers[i].Name, buf, sizeof(SpawnerPlayers[i].Name) - 1);

        // Side
        SpawnerPlayers[i].Side = SpawnIni->Get_Int(section_names[i], "Side", -1);
        if (SpawnerPlayers[i].Side == -1) {
            continue; // Skip if no side specified
        }

        // Color
        SpawnerPlayers[i].Color = SpawnIni->Get_Int(section_names[i], "Color", -1);
        if (SpawnerPlayers[i].Color == -1) {
            continue;
        }

        // IP
        memset(buf, 0, sizeof(buf));
        SpawnIni->Get_String(section_names[i], "Ip", "", buf, sizeof(buf));
        strncpy(SpawnerPlayers[i].Ip, buf, sizeof(SpawnerPlayers[i].Ip) - 1);

        // Port
        SpawnerPlayers[i].Port = SpawnIni->Get_Int(section_names[i], "Port", 1234);

        // Ignored
        SpawnerPlayers[i].Ignored = SpawnIni->Get_Bool(section_names[i], "Ignored", false);
    }
}

/***********************************************************************************************
 * Spawner_Read_Spectators -- Read [IsSpectator] section
 *=============================================================================================*/
static void Spawner_Read_Spectators(void)
{
    if (SpawnIni == nullptr) return;

    static const char* multi_names[] = {
        "Multi1", "Multi2", "Multi3", "Multi4",
        "Multi5", "Multi6", "Multi7", "Multi8"
    };

    for (int i = 0; i < 8; i++) {
        bool is_spectator = SpawnIni->Get_Bool("IsSpectator", multi_names[i], false);
        SpawnerPlayers[i].IsSpectator = is_spectator;

        if (is_spectator) {
            Spectators_Array[HOUSE_MULTI1 + i] = true;
            Spectator_Count++;
        }
    }
}

/***********************************************************************************************
 * Spawner_Read_Spawn_Locations -- Read [SpawnLocations] section
 *=============================================================================================*/
static void Spawner_Read_Spawn_Locations(void)
{
    if (SpawnIni == nullptr) return;

    static const char* multi_names[] = {
        "Multi1", "Multi2", "Multi3", "Multi4",
        "Multi5", "Multi6", "Multi7", "Multi8"
    };

    for (int i = 0; i < 8; i++) {
        SpawnerPlayers[i].SpawnLocation = SpawnIni->Get_Int("SpawnLocations", multi_names[i], -1);
    }
}

/***********************************************************************************************
 * Spawner_Read_House_Colours -- Read [HouseColours] section
 *=============================================================================================*/
static void Spawner_Read_House_Colours(void)
{
    if (SpawnIni == nullptr) return;

    static const char* multi_names[] = {
        "Multi1", "Multi2", "Multi3", "Multi4",
        "Multi5", "Multi6", "Multi7", "Multi8"
    };

    for (int i = 0; i < 8; i++) {
        int color = SpawnIni->Get_Int("HouseColours", multi_names[i], 0xFF);
        if (color != 0xFF) {
            SpawnerPlayers[i].Color = color;
        }
    }
}

/***********************************************************************************************
 * Spawner_Read_House_Countries -- Read [HouseCountries] section
 *=============================================================================================*/
static void Spawner_Read_House_Countries(void)
{
    if (SpawnIni == nullptr) return;

    static const char* multi_names[] = {
        "Multi1", "Multi2", "Multi3", "Multi4",
        "Multi5", "Multi6", "Multi7", "Multi8"
    };

    for (int i = 0; i < 8; i++) {
        int country = SpawnIni->Get_Int("HouseCountries", multi_names[i], 0xFF);
        if (country != 0xFF) {
            SpawnerPlayers[i].Side = country;
        }
    }
}

/***********************************************************************************************
 * Spawner_Read_House_Handicaps -- Read [HouseHandicaps] section
 *=============================================================================================*/
static void Spawner_Read_House_Handicaps(void)
{
    if (SpawnIni == nullptr) return;

    static const char* multi_names[] = {
        "Multi1", "Multi2", "Multi3", "Multi4",
        "Multi5", "Multi6", "Multi7", "Multi8"
    };

    for (int i = 0; i < 8; i++) {
        int handicap = SpawnIni->Get_Int("HouseHandicaps", multi_names[i], 0xFF);
        if (handicap != 0xFF) {
            SpawnerPlayers[i].Handicap = handicap;
        }
    }
}

/***********************************************************************************************
 * Spawner_Read_Tunnel -- Read [Tunnel] section
 *=============================================================================================*/
static void Spawner_Read_Tunnel(void)
{
    if (SpawnIni == nullptr) return;

    char buf[128];
    memset(buf, 0, sizeof(buf));
    SpawnIni->Get_String("Tunnel", "Ip", "", buf, sizeof(buf));
    strncpy(SpawnerCfg.TunnelIp, buf, sizeof(SpawnerCfg.TunnelIp) - 1);

    SpawnerCfg.TunnelPort = SpawnIni->Get_Int("Tunnel", "Port", 0);
    SpawnerCfg.TunnelId = SpawnIni->Get_Int("Settings", "Port", 0);
}

/***********************************************************************************************
 * Spawner_Read_Fix_Flags -- Read fix flags from [Settings] section
 *=============================================================================================*/
static void Spawner_Read_Fix_Flags(void)
{
    if (SpawnIni == nullptr) return;

    SpawnerCfg.SuperTeslaFix = SpawnIni->Get_Bool("Settings", "SuperTeslaFix", true);
    SpawnerCfg.ChatMuted = SpawnIni->Get_Bool("Settings", "Diff", false);
    SpawnerCfg.AftermathFastBuildSpeed = SpawnIni->Get_Bool("Settings", "AftermathFastBuildSpeed", false);
    SpawnerCfg.FixFormationSpeed = SpawnIni->Get_Bool("Settings", "FixFormationSpeed", false);
    SpawnerCfg.ReduceSovietMaxAircrafts = SpawnIni->Get_Bool("Settings", "ReduceSovietMaxAircrafts", true);
    SpawnerCfg.FixRangeExploit = SpawnIni->Get_Bool("Settings", "FixRangeExploit", false);
    SpawnerCfg.FixMagicBuild = SpawnIni->Get_Bool("Settings", "FixMagicBuild", false);
    SpawnerCfg.ParaBombsInMultiplayer = SpawnIni->Get_Bool("Settings", "ParaBombsInMultiplayer", false);
    SpawnerCfg.FixAIAlly = SpawnIni->Get_Bool("Settings", "FixAIAlly", false);
    SpawnerCfg.MCVUndeploy = SpawnIni->Get_Bool("Settings", "MCVUndeploy", false);
    SpawnerCfg.AllyReveal = SpawnIni->Get_Bool("Settings", "AllyReveal", false);
    SpawnerCfg.ForcedAlliances = SpawnIni->Get_Bool("Settings", "ForcedAlliances", false);
    SpawnerCfg.TechCenterBugFix = SpawnIni->Get_Bool("Settings", "TechCenterBugFix", false);
    SpawnerCfg.BuildOffAlly = SpawnIni->Get_Bool("Settings", "BuildOffAlly", false);
    SpawnerCfg.SouthAdvantageFix = SpawnIni->Get_Bool("Settings", "SouthAdvantageFix", false);
    SpawnerCfg.NoScreenShake = SpawnIni->Get_Bool("Settings", "NoScreenShake", false);
    SpawnerCfg.NoTeslaZapEffectDelay = SpawnIni->Get_Bool("Settings", "NoTeslaZapEffectDelay", false);
    SpawnerCfg.ShortGame = SpawnIni->Get_Bool("Settings", "ShortGame", false);
    SpawnerCfg.DeadPlayersRadar = SpawnIni->Get_Bool("Settings", "DeadPlayersRadar", false);
    SpawnerCfg.PlayerIsGameHost = SpawnIni->Get_Bool("Settings", "PlayerIsGameHost", true);
}

/***********************************************************************************************
 * Spawner_Apply_Session_Settings -- Apply spawner settings to the game session
 *=============================================================================================*/
void Spawner_Apply_Session_Settings(void)
{
    // Set session type
    if (HumanPlayers > 1) {
        Session.Type = GAME_IPX; // online multiplayer (type 4 in asm)
    } else {
        Session.Type = GAME_SKIRMISH; // skirmish (type 5 in asm)
    }

    // Set protocol
    Session.CommProtocol = (CommProtocolType)SpawnerCfg.NetworkVersionProtocol;

    // Apply game settings
    Session.Options.Bases = SpawnerCfg.Bases;
    Session.Options.Credits = SpawnerCfg.Credits;
    Session.Options.UnitCount = SpawnerCfg.UnitCount;
    Session.Options.AIPlayers = SpawnerCfg.AIPlayers;

    // Apply player name
    strncpy(Session.Handle, SpawnerCfg.Name, sizeof(Session.Handle) - 1);

    // Apply color
    Session.PrefColor = (PlayerColorType)SpawnerCfg.Color;
    Session.ColorIdx = (PlayerColorType)SpawnerCfg.Color;

    // Apply side
    Session.House = (HousesType)SpawnerCfg.Side;

    // Apply timing
    Session.MaxAhead = SpawnerCfg.MaxAhead;
    Session.FrameSendRate = SpawnerCfg.FrameSendRate;
    Session.DesiredFrameRate = SpawnerCfg.GameSpeed;

    // Apply scenario name - set both Session.ScenarioFileName and Scen.ScenarioName
    // Start_Scenario() uses Scen.ScenarioName, not Session.ScenarioFileName
    strncpy(Session.ScenarioFileName, SpawnerCfg.Scenario, sizeof(Session.ScenarioFileName) - 1);
    strncpy(Scen.ScenarioName, SpawnerCfg.Scenario, sizeof(Scen.ScenarioName) - 1);
    Scen.ScenarioName[sizeof(Scen.ScenarioName) - 1] = '\0';

    // Set up players list for the session
    Session.Players.Clear();
    for (int i = 0; i < 8; i++) {
        if (SpawnerPlayers[i].Name[0] != '\0' && SpawnerPlayers[i].Side != -1) {
            NodeNameType* node = new NodeNameType;
            memset(node, 0, sizeof(NodeNameType));
            strncpy(node->Name, SpawnerPlayers[i].Name, sizeof(node->Name) - 1);
            node->Player.House = (HousesType)(HOUSE_MULTI1 + i);
            node->Player.Color = (PlayerColorType)SpawnerPlayers[i].Color;
            node->Player.ID = (HousesType)(HOUSE_MULTI1 + i);
            Session.Players.Add(node);
        }
    }
    Session.NumPlayers = Session.Players.Count();

    // Set up max players
    Session.MaxPlayers = 8;
}

/***********************************************************************************************
 * Spectator_Init_House -- Initialize spectator data for a house
 *
 * Called during house initialization when the spawner is active.
 * Marks spectator houses as dead and sets the IsSpectator flag.
 *=============================================================================================*/
void Spectator_Init_House(HouseClass* house)
{
    if (!Spawner_Is_Active || house == NULL) {
        return;
    }

    int house_id = house->Class->House;

    // Check if this house is marked as a spectator in the spawner config
    if (house_id >= HOUSE_MULTI1 && house_id <= HOUSE_MULTI8) {
        int index = house_id - HOUSE_MULTI1;

        if (index < 8 && Spectators_Array[house_id]) {
            Spectator_Count++;

            // Mark house as dead (no units/buildings)
            house->IsDefeated = true;

            // Set the spectator flag
            house->IsSpectator = true;

            // Spectators don't produce anything
            house->IsBaseBuilding = false;
            house->IsStarted = false;
        }
    }
}

/***********************************************************************************************
 * Draw_Live_Stats -- Draw live game statistics on the sidebar
 *
 * When the local player is a spectator, this draws vehicle/infantry/building/aircraft
 * counts (owned and killed) for each human player on the sidebar area.
 *=============================================================================================*/
void Draw_Live_Stats(void)
{
    if (!Is_Local_Player_Spectator()) {
        return;
    }

    // Position stats on the left side of the sidebar panel
    int y_start = 177;
    int x_left = 32; // Adjust based on sidebar position
    int x_right = 175;
    int back_color = BLACK;
    int row_height = 12;
    int row = 0;

    // Header: Units Owned
    Fancy_Text_Print("Veh/Inf/Bld/Air Owned",
                     x_left,
                     y_start,
                     &ColorRemaps[PCOLOR_GREY],
                     back_color,
                     TPF_6PT_GRAD | TPF_NOSHADOW);

    // Draw stats for each human player
    for (int i = 0; i < Session.MaxPlayers; i++) {
        HousesType house_type = (HousesType)(HOUSE_MULTI1 + i);
        HouseClass* house = HouseClass::As_Pointer(house_type);

        if (house == NULL || !house->IsActive || house->IsSpectator || house->IsDefeated) {
            continue;
        }

        row++;

        // Get the remap color for this player
        RemapControlType* color = &ColorRemaps[house->RemapColor];

        // Draw player name
        Fancy_Text_Print(house->IniName,
                         x_left,
                         (row_height * row) + y_start,
                         color,
                         back_color,
                         TPF_6PT_GRAD | TPF_NOSHADOW);

        // Draw unit counts: Units/Infantry/Buildings/Aircraft
        char buffer[64];
        sprintf(buffer,
                " %d/%d/%d/%d",
                house->CurUnits,
                house->CurInfantry,
                house->CurBuildings,
                house->CurAircraft);
        Fancy_Text_Print(buffer,
                         x_right,
                         (row_height * row) + y_start,
                         color,
                         back_color,
                         TPF_6PT_GRAD | TPF_RIGHT | TPF_NOSHADOW);
    }

    // Header: Units Killed
    row++;
    Fancy_Text_Print("Veh/Inf/Bld/Air Killed",
                     x_left,
                     (row_height * row) + y_start,
                     &ColorRemaps[PCOLOR_GREY],
                     back_color,
                     TPF_6PT_GRAD | TPF_NOSHADOW);

    // Draw kill stats for each human player
    for (int i = 0; i < Session.MaxPlayers; i++) {
        HousesType house_type = (HousesType)(HOUSE_MULTI1 + i);
        HouseClass* house = HouseClass::As_Pointer(house_type);

        if (house == NULL || !house->IsActive || house->IsSpectator || house->IsDefeated) {
            continue;
        }

        row++;

        RemapControlType* color = &ColorRemaps[house->RemapColor];

        // Draw player name
        Fancy_Text_Print(house->IniName,
                         x_left,
                         (row_height * row) + y_start,
                         color,
                         back_color,
                         TPF_6PT_GRAD | TPF_NOSHADOW);

        // Sum up kills across all enemy houses
        int vehicles_killed = 0;
        int infantry_killed = 0;
        int buildings_killed = 0;
        int aircraft_killed = 0;

        for (HousesType enemy = HOUSE_FIRST; enemy < HOUSE_COUNT; enemy++) {
            vehicles_killed += house->UnitsKilled[enemy];
            buildings_killed += house->BuildingsKilled[enemy];
        }

        // Note: DestroyedAircraft and DestroyedInfantry are UnitTrackerClass objects
        // For simplicity, we use the direct kill counters available
        char buffer[64];
        sprintf(buffer,
                " %d/%d/%d/%d",
                vehicles_killed,
                infantry_killed,
                buildings_killed,
                aircraft_killed);
        Fancy_Text_Print(buffer,
                         x_right,
                         (row_height * row) + y_start,
                         color,
                         back_color,
                         TPF_6PT_GRAD | TPF_RIGHT | TPF_NOSHADOW);
    }
}

/***********************************************************************************************
 * Should_Skip_House -- Check if objects from this house should be skipped during INI loading
 *
 * When a house is a spectator or dead, its objects should be skipped during
 * scenario loading to prevent creating units for non-participating houses.
 *=============================================================================================*/
bool Should_Skip_House(HousesType house)
{
    if (!Spawner_Is_Active) {
        return false;
    }

    HouseClass* hptr = HouseClass::As_Pointer(house);
    if (hptr != NULL && hptr->IsSpectator) {
        return true;
    }

    return false;
}

/***********************************************************************************************
 * Is_Local_Player_Spectator -- Check if the local player is a spectator
 *=============================================================================================*/
bool Is_Local_Player_Spectator(void)
{
    if (PlayerPtr != NULL && PlayerPtr->IsSpectator) {
        return true;
    }
    return false;
}

/***********************************************************************************************
 * Spawner_Get_Spawn_Location -- Get spawn location for a given house
 *
 * Returns the spawn location index from SPAWN.INI, or -1 if not set.
 *=============================================================================================*/
int Spawner_Get_Spawn_Location(HousesType house)
{
    if (!Spawner_Is_Active) {
        return -1;
    }

    if (house >= HOUSE_MULTI1 && house <= HOUSE_MULTI8) {
        int index = house - HOUSE_MULTI1;
        if (index < 8) {
            return SpawnerPlayers[index].SpawnLocation;
        }
    }

    return -1;
}

/***********************************************************************************************
 * Spawner_Get_Human_Player_Count -- Get the number of human players
 *=============================================================================================*/
int Spawner_Get_Human_Player_Count(void)
{
    return HumanPlayers;
}

/***********************************************************************************************
 * Spectator_Enter_Replay_Mode -- Enter spectator mode for replay watching
 *
 * Called after a replay file is loaded and the game scenario starts.
 * Marks the local player as a spectator, reveals the entire map,
 * and forces the radar to be active.
 *=============================================================================================*/
void Spectator_Enter_Replay_Mode(void)
{
    if (PlayerPtr == NULL) {
        return;
    }

    // Set local player as spectator
    PlayerPtr->IsSpectator = true;
    PlayerPtr->IsDefeated = true;
    PlayerPtr->IsBaseBuilding = false;
    PlayerPtr->IsStarted = false;

    // Reveal the entire map for the spectator
    // Sight_From with a huge radius reveals all cells visible to the player
    CELL center_cell = XY_Cell(Map.MapCellX + Map.MapCellWidth / 2,
                                Map.MapCellY + Map.MapCellHeight / 2);
    int max_radius = max(Map.MapCellWidth, Map.MapCellHeight) + 20; // cover entire map
    Map.Sight_From(center_cell, max_radius, PlayerPtr, false);

    // Also reveal all cells directly by setting their mapped/visible state
    for (int y = 0; y < Map.MapCellHeight; y++) {
        for (int x = 0; x < Map.MapCellWidth; x++) {
            CELL cell = XY_Cell(Map.MapCellX + x, Map.MapCellY + y);
            CellClass* cellptr = &Map[cell];
            cellptr->Set_Mapped(PlayerPtr, true);
            cellptr->Set_Visible(PlayerPtr, true);
        }
    }

    // Force radar to be active for spectator
    PlayerPtr->Radar = RADAR_ON;

    // Flag map for redraw to show revealed shroud
    Map.Flag_To_Redraw(true);
}

/***********************************************************************************************
 * Spectator_Replay_Render -- Called each frame to overlay stats during replay
 *
 * Must be called after Map.Render() in the replay playback path.
 * Draws live unit/building/aircraft counts on the sidebar.
 *=============================================================================================*/
void Spectator_Replay_Render(void)
{
    if (!Session.Play) {
        return;
    }

    if (PlayerPtr == NULL || !PlayerPtr->IsSpectator) {
        return;
    }

    // Force radar active every frame (in case it gets toggled off)
    PlayerPtr->Radar = RADAR_ON;

    // Draw the live stats overlay
    Draw_Live_Stats();
}

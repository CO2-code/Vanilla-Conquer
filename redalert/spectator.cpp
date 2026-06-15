//
// Spectator and Spawner support for CnCNet compatibility
// Based on ra-patches assembly code, ported to C++
//

#include "function.h"
#include "spectator.h"

// Global spawner state
bool Spawner_Is_Active = false;
int Spectator_Count = 0;
bool Spectators_Array[HOUSE_COUNT] = {};

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

        if (index < HOUSE_COUNT && Spectators_Array[house_id]) {
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
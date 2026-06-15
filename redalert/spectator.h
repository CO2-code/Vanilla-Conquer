//
// Spectator and Spawner support for CnCNet compatibility
// Based on ra-patches assembly code, ported to C++
//

#ifndef SPECTATOR_H
#define SPECTATOR_H

#include "house.h"

// Global flag indicating if the CnCNet spawner is active
extern bool Spawner_Is_Active;

// Spectator count across all houses
extern int Spectator_Count;

// Array indicating which houses are spectators (indexed by HousesType)
extern bool Spectators_Array[HOUSE_COUNT];

// Initialize spectator data for a house during game setup
void Spectator_Init_House(HouseClass* house);

// Draw live stats overlay on the sidebar for spectators
void Draw_Live_Stats(void);

// Skip dead/spectator houses when reading object INI data
bool Should_Skip_House(HousesType house);

// Check if the local player is a spectator
bool Is_Local_Player_Spectator(void);

#endif // SPECTATOR_H
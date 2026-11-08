#pragma once

#include "common.h"

namespace Natives
{
    // 1. Engine Lifecycle
    cell AMX_NATIVE_CALL GroundAndreas_Init(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GroundAndreas_Unload(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GroundAndreas_IsLoaded(AMX* amx, cell* params);

    // 2. Core Elevation Queries
    cell AMX_NATIVE_CALL GetGroundZPos(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundZPosBatch(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundZPosArea(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundZPosRadius(AMX* amx, cell* params);

    // 3. Raycasting, Normals & Surface Alignment
    cell AMX_NATIVE_CALL IsWorldBlockingLine(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundNormal(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundObjectRotation(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundObjectPose(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundTwoPointRotation(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundLinePoints(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsGroundUnderwater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundClearance(AMX* amx, cell* params);

    // 4. Underground Detection Subsystem
    cell AMX_NATIVE_CALL IsPointUnderGround(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPlayerUnderGround(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsVehicleUnderGround(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPlayerOnGround(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsVehicleOnGround(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetPointGroundDistance(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetPlayerGroundDistance(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetVehicleGroundDistance(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GroundAndreas_SetVehicleInterior(AMX* amx, cell* params);

    // 5. Water Detection Subsystem
    cell AMX_NATIVE_CALL IsPointInWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPointUnderWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPlayerInWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPlayerUnderWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsVehicleInWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsVehicleUnderWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetWaterLevel(AMX* amx, cell* params);

    // 6. Dynamic Terrain & ColEditor
    cell AMX_NATIVE_CALL GroundAndreasRemoveBuilding(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL ApplyObjectToGroundAndreas(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL SaveGroundAndreasPatches(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL LoadGroundAndreasPatches(AMX* amx, cell* params);

    // 7. Terrain Analytics, Kinematics & Spawn Safety
    cell AMX_NATIVE_CALL IsGroundSafeForSpawn(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetSafeGroundSpawn(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundImpactTime(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPointInRadarShadow(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetAreaElevationStats(AMX* amx, cell* params);
}

extern AMX_NATIVE_INFO PluginNatives[];

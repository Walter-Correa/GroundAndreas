#pragma once

#include "common.h"

namespace Natives
{
    cell AMX_NATIVE_CALL GroundAndreas_Init(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GroundAndreas_Unload(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GroundAndreas_IsLoaded(AMX* amx, cell* params);

    cell AMX_NATIVE_CALL GetGroundZPos(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsWorldBlockingLine(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundNormal(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundObjectRotation(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsGroundUnderwater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundClearance(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetGroundZPosBatch(AMX* amx, cell* params);

    // Underground detection
    cell AMX_NATIVE_CALL IsPointUnderGround(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPlayerUnderGround(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsVehicleUnderGround(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetPointGroundDistance(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetPlayerGroundDistance(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetVehicleGroundDistance(AMX* amx, cell* params);

    // Water detection
    cell AMX_NATIVE_CALL IsPointInWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPointUnderWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPlayerInWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsPlayerUnderWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsVehicleInWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL IsVehicleUnderWater(AMX* amx, cell* params);
    cell AMX_NATIVE_CALL GetWaterLevel(AMX* amx, cell* params);
}

extern AMX_NATIVE_INFO PluginNatives[];

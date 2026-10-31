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
}

extern AMX_NATIVE_INFO PluginNatives[];

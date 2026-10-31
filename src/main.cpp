#include "common.h"
#include "natives.h"
#include "heightmap.h"

logprintf_t logprintf = nullptr;

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
    return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** ppData)
{
    pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
    logprintf = reinterpret_cast<logprintf_t>(ppData[PLUGIN_DATA_LOGPRINTF]);

    logprintf("--------------------------------------------------");
    logprintf("  GroundAndreas v1.0.0 by Walter Correa");
    logprintf("  Ultra-Fast Memory-Mapped Heightmap Engine");
    logprintf("--------------------------------------------------");

    // Auto-load heightmap if present in standard locations
    if (HeightMap::Instance().Load("scriptfiles/SAfull.hmap") ||
        HeightMap::Instance().Load("SAfull.hmap"))
    {
        logprintf("  [GroundAndreas] Map loaded: %s", HeightMap::Instance().GetLoadedPath().c_str());
    }
    else
    {
        logprintf("  [GroundAndreas] Notice: SAfull.hmap not found in scriptfiles/. Call GroundAndreas_Init() from Pawn if needed.");
    }

    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
    HeightMap::Instance().Unload();
    if (logprintf)
    {
        logprintf("  [GroundAndreas] Plugin unloaded.");
    }
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
    return amx_Register(amx, PluginNatives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX* amx)
{
    return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
}

#include "common.h"
#include "natives.h"
#include "heightmap.h"
#include "amx_helper.h"
#include "version.h"
#include <exception>

logprintf_t logprintf = nullptr;

typedef int (AMXAPI *amx_Register_t)(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number);
static amx_Register_t g_OrigAmxRegister = nullptr;

// T4 (A1): Intercept all native registrations to reliably cache server natives
static int AMXAPI Hooked_AmxRegister(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number)
{
    if (nativelist != nullptr)
    {
        for (int i = 0; (number >= 0) ? (i < number) : (nativelist[i].name != nullptr); ++i)
        {
            if (nativelist[i].name != nullptr && nativelist[i].func != nullptr)
            {
                AmxHelper::CacheNative(nativelist[i].name, nativelist[i].func);
            }
        }
    }
    if (g_OrigAmxRegister != nullptr)
    {
        return g_OrigAmxRegister(amx, nativelist, number);
    }
    return AMX_ERR_NONE;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
    return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** ppData)
{
    try
    {
        pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
        logprintf = reinterpret_cast<logprintf_t>(ppData[PLUGIN_DATA_LOGPRINTF]);

        // Hook amx_Register to capture natives reliably (T4)
        if (pAMXFunctions != nullptr)
        {
            void** amxExports = reinterpret_cast<void**>(pAMXFunctions);
            if (amxExports[PLUGIN_AMX_EXPORT_Register] != nullptr)
            {
                g_OrigAmxRegister = reinterpret_cast<amx_Register_t>(amxExports[PLUGIN_AMX_EXPORT_Register]);
                amxExports[PLUGIN_AMX_EXPORT_Register] = reinterpret_cast<void*>(Hooked_AmxRegister);
            }
        }

        // T13 (B9): Process environment sentinel to detect duplicate plugin/component loads
#if defined(_WIN32) || defined(WIN32)
        char envBuf[8] = { 0 };
        if (GetEnvironmentVariableA("GROUNDANDREAS_LOADED", envBuf, sizeof(envBuf)) > 0)
        {
            logprintf("  [GroundAndreas] WARNING: Another GroundAndreas instance (plugin or component) is already active in this process!");
        }
        SetEnvironmentVariableA("GROUNDANDREAS_LOADED", "1");
#else
        if (getenv("GROUNDANDREAS_LOADED") != nullptr)
        {
            logprintf("  [GroundAndreas] WARNING: Another GroundAndreas instance (plugin or component) is already active in this process!");
        }
        setenv("GROUNDANDREAS_LOADED", "1", 1);
#endif

        logprintf("--------------------------------------------------");
        logprintf("  GroundAndreas v%s by Walter Correa", GROUNDANDREAS_VERSION_STRING);
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
    catch (const std::exception& e)
    {
        if (logprintf)
        {
            logprintf("[GroundAndreas] Fatal exception during Load: %s", e.what());
        }
        return false;
    }
    catch (...)
    {
        return false;
    }
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
    try
    {
        // Restore original amx_Register pointer (T4)
        if (pAMXFunctions != nullptr && g_OrigAmxRegister != nullptr)
        {
            void** amxExports = reinterpret_cast<void**>(pAMXFunctions);
            if (amxExports != nullptr && amxExports[PLUGIN_AMX_EXPORT_Register] == reinterpret_cast<void*>(Hooked_AmxRegister))
            {
                amxExports[PLUGIN_AMX_EXPORT_Register] = reinterpret_cast<void*>(g_OrigAmxRegister);
            }
        }

#if defined(_WIN32) || defined(WIN32)
        SetEnvironmentVariableA("GROUNDANDREAS_LOADED", nullptr);
#else
        unsetenv("GROUNDANDREAS_LOADED");
#endif

        HeightMap::Instance().Unload();
        if (logprintf)
        {
            logprintf("  [GroundAndreas] Plugin unloaded.");
        }
    }
    catch (...)
    {
    }
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
    try
    {
        return amx_Register(amx, PluginNatives, -1);
    }
    catch (...)
    {
        return AMX_ERR_INIT;
    }
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX* amx)
{
    return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
}

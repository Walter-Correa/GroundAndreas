#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <cstring>
#include <cmath>

namespace AmxHelper
{
    struct NativeCache
    {
        AMX_NATIVE fnGetPlayerPos = nullptr;
        AMX_NATIVE fnGetVehiclePos = nullptr;
        AMX_NATIVE fnGetVehicleModel = nullptr;
        AMX_NATIVE fnGetVehicleRotationQuat = nullptr;
        AMX_NATIVE fnGetPlayerInterior = nullptr;
        AMX_NATIVE fnGetPlayerVirtualWorld = nullptr;
        AMX_NATIVE fnGetVehicleVirtualWorld = nullptr;
        AMX_NATIVE fnGetPlayerVehicleID = nullptr;
        AMX_NATIVE fnGetWeather = nullptr;
        bool searchedGetWeather = false;
#if defined(OMP_COMPONENT)
        AMX* lastGetWeatherAmx = nullptr;
#endif
    };

    inline int (&GetVehicleInteriorsArray())[2001]
    {
        static int s_vehicleInteriors[2001] = { 0 };
        return s_vehicleInteriors;
    }

    inline void SetVehicleInterior(int vehicleId, int interiorId)
    {
        if (vehicleId >= 1 && vehicleId <= 2000)
        {
            GetVehicleInteriorsArray()[vehicleId] = interiorId;
        }
    }

    inline NativeCache& Cache()
    {
        static NativeCache s_cache;
        return s_cache;
    }

    inline void CacheNative(const char* name, AMX_NATIVE func)
    {
        if (name == nullptr || func == nullptr) return;
        if (std::strcmp(name, "GetPlayerPos") == 0) Cache().fnGetPlayerPos = func;
        else if (std::strcmp(name, "GetVehiclePos") == 0) Cache().fnGetVehiclePos = func;
        else if (std::strcmp(name, "GetVehicleModel") == 0) Cache().fnGetVehicleModel = func;
        else if (std::strcmp(name, "GetVehicleRotationQuat") == 0) Cache().fnGetVehicleRotationQuat = func;
        else if (std::strcmp(name, "GetPlayerInterior") == 0) Cache().fnGetPlayerInterior = func;
        else if (std::strcmp(name, "GetPlayerVirtualWorld") == 0) Cache().fnGetPlayerVirtualWorld = func;
        else if (std::strcmp(name, "GetVehicleVirtualWorld") == 0) Cache().fnGetVehicleVirtualWorld = func;
        else if (std::strcmp(name, "GetPlayerVehicleID") == 0) Cache().fnGetPlayerVehicleID = func;
        else if (std::strcmp(name, "GetWeather") == 0) Cache().fnGetWeather = func;
    }

    inline AMX_NATIVE FindNative(AMX* amx, const char* name)
    {
        if (amx == nullptr || amx->base == nullptr || name == nullptr)
        {
            return nullptr;
        }

        int index = -1;
        if (amx_FindNative(amx, name, &index) == AMX_ERR_NONE && index >= 0)
        {
            auto* hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
            auto* stub = reinterpret_cast<AMX_FUNCSTUB*>(reinterpret_cast<uint8_t*>(amx->base) + hdr->natives + (index * hdr->defsize));
            AMX_NATIVE func = reinterpret_cast<AMX_NATIVE>(stub->address);
            if (func != nullptr)
            {
                CacheNative(name, func);
            }
            return func;
        }
        return nullptr;
    }

    // T1 (C1): Safe AMX array range access validation
    inline bool GetArray(AMX* amx, cell addr, cell count, cell*& out)
    {
        out = nullptr;
        if (amx == nullptr || count <= 0 || count > 1000000)
        {
            return false;
        }

        const ucell uAddr = static_cast<ucell>(addr);
        const ucell uCount = static_cast<ucell>(count);
        const ucell endOffset = (uCount - 1) * sizeof(cell);
        const ucell uEndAddr = uAddr + endOffset;

        // Check for integer overflow
        if (uEndAddr < uAddr)
        {
            return false;
        }

        // Verify entire range is within Heap/Data [0, amx->hea) OR Stack [amx->stk, amx->stp)
        const bool inHeap = (addr >= 0 && uEndAddr < static_cast<ucell>(amx->hea));
        const bool inStack = (addr >= amx->stk && uEndAddr < static_cast<ucell>(amx->stp));
        if (!inHeap && !inStack)
        {
            return false;
        }

        cell* pStart = nullptr;
        cell* pEnd = nullptr;
        if (amx_GetAddr(amx, addr, &pStart) != AMX_ERR_NONE || pStart == nullptr)
        {
            return false;
        }
        if (amx_GetAddr(amx, static_cast<cell>(uEndAddr), &pEnd) != AMX_ERR_NONE || pEnd == nullptr)
        {
            return false;
        }

        out = pStart;
        return true;
    }

    // T12 (M8): Safe string param extraction replacing unchecked amx_StrParam
    inline bool GetString(AMX* amx, cell param, std::string& out, size_t maxLen = 260)
    {
        out.clear();
        cell* amxAddr = nullptr;
        if (amx_GetAddr(amx, param, &amxAddr) != AMX_ERR_NONE || amxAddr == nullptr)
        {
            return false;
        }

        int len = 0;
        if (amx_StrLen(amxAddr, &len) != AMX_ERR_NONE || len < 0)
        {
            return false;
        }

        if (static_cast<size_t>(len) > maxLen)
        {
            return false;
        }

        std::vector<char> buffer(len + 1, '\0');
        if (amx_GetString(buffer.data(), amxAddr, 0, len + 1) != AMX_ERR_NONE)
        {
            return false;
        }

        out = buffer.data();
        return true;
    }

    // T12 (M8, N17): Restrict filesystem paths to prevent directory traversal
    inline bool IsPathSafe(const std::string& path)
    {
        if (path.empty() || path.length() > 260)
        {
            return false;
        }

        // Reject NTFS Alternate Data Streams (ADS) and drive letters anywhere (N17)
        if (path.find(':') != std::string::npos)
        {
            return false;
        }

        // Reject absolute paths (/..., \...)
        if (path[0] == '/' || path[0] == '\\')
        {
            return false;
        }

        // Verify traversal per component without rejecting valid names like 'a..b' (N17)
        std::string component;
        for (size_t i = 0; i <= path.length(); ++i)
        {
            char c = (i < path.length()) ? path[i] : '/';
            if (c == '/' || c == '\\')
            {
                if (component == "..")
                {
                    return false;
                }
                component.clear();
            }
            else
            {
                component.push_back(c);
            }
        }

        return true;
    }

    inline std::string SanitizePatchPath(const std::string& path)
    {
        if (!IsPathSafe(path))
        {
            return "";
        }

        if (path.rfind("scriptfiles/", 0) == 0 || path.rfind("scriptfiles\\", 0) == 0)
        {
            return path;
        }
        return "scriptfiles/" + path;
    }

    // T4 (A1): Reliable player position lookup with native cache & amx_Allot
    inline bool GetPlayerPos(AMX* amx, int playerId, float& x, float& y, float& z)
    {
#if defined(OMP_COMPONENT)
        if (g_core != nullptr)
        {
            IPlayer* player = g_core->getPlayers().get(playerId);
            if (player != nullptr)
            {
                Vector3 pos = player->getPosition();
                x = pos.x;
                y = pos.y;
                z = pos.z;
                return true;
            }
        }
#endif
        AMX_NATIVE nativeFn = Cache().fnGetPlayerPos;
        if (nativeFn == nullptr)
        {
            nativeFn = FindNative(amx, "GetPlayerPos");
        }
        if (nativeFn == nullptr)
        {
            return false;
        }

        cell amxAddr = 0;
        cell* physAddr = nullptr;
        if (amx_Allot(amx, 3, &amxAddr, &physAddr) != AMX_ERR_NONE || physAddr == nullptr)
        {
            return false;
        }

        cell params[5];
        params[0] = 4 * static_cast<cell>(sizeof(cell));
        params[1] = static_cast<cell>(playerId);
        params[2] = amxAddr;
        params[3] = amxAddr + static_cast<cell>(sizeof(cell));
        params[4] = amxAddr + static_cast<cell>(2 * sizeof(cell));

        const cell result = nativeFn(amx, params);
        bool success = false;
        if (result != 0)
        {
            cell* pX = nullptr;
            cell* pY = nullptr;
            cell* pZ = nullptr;
            if (amx_GetAddr(amx, params[2], &pX) == AMX_ERR_NONE && pX != nullptr &&
                amx_GetAddr(amx, params[3], &pY) == AMX_ERR_NONE && pY != nullptr &&
                amx_GetAddr(amx, params[4], &pZ) == AMX_ERR_NONE && pZ != nullptr)
            {
                x = amx_ctof(*pX);
                y = amx_ctof(*pY);
                z = amx_ctof(*pZ);
                success = true;
            }
        }

        amx_Release(amx, amxAddr);
        return success;
    }

    inline int GetPlayerVehicleId(AMX* amx, int playerId)
    {
#if defined(OMP_COMPONENT)
        if (g_core != nullptr)
        {
            IPlayer* p = g_core->getPlayers().get(playerId);
            if (p != nullptr)
            {
                PlayerState state = p->getState();
                if (state == PlayerState_Driver || state == PlayerState_Passenger)
                {
                    IPlayerVehicleData* d = queryExtension<IPlayerVehicleData>(*p);
                    if (d != nullptr && d->getVehicle() != nullptr)
                    {
                        return d->getVehicle()->getID();
                    }
                }
            }
            return 0;
        }
#endif
        AMX_NATIVE nativeFn = Cache().fnGetPlayerVehicleID;
        if (nativeFn == nullptr)
        {
            nativeFn = FindNative(amx, "GetPlayerVehicleID");
        }
        if (nativeFn == nullptr)
        {
            return 0;
        }

        cell params[2];
        params[0] = 1 * static_cast<cell>(sizeof(cell));
        params[1] = static_cast<cell>(playerId);
        const cell res = nativeFn(amx, params);
        return (res > 0) ? static_cast<int>(res) : 0;
    }

    // T4 (A1): Reliable vehicle position lookup with native cache & amx_Allot
    inline bool GetVehiclePos(AMX* amx, int vehicleId, float& x, float& y, float& z)
    {
#if defined(OMP_COMPONENT)
        if (g_vehiclesComponent != nullptr)
        {
            IVehicle* veh = g_vehiclesComponent->get(vehicleId);
            if (veh != nullptr)
            {
                Vector3 pos = veh->getPosition();
                x = pos.x;
                y = pos.y;
                z = pos.z;
                return true;
            }
        }
#endif
        AMX_NATIVE nativeFn = Cache().fnGetVehiclePos;
        if (nativeFn == nullptr)
        {
            nativeFn = FindNative(amx, "GetVehiclePos");
        }
        if (nativeFn == nullptr)
        {
            return false;
        }

        cell amxAddr = 0;
        cell* physAddr = nullptr;
        if (amx_Allot(amx, 3, &amxAddr, &physAddr) != AMX_ERR_NONE || physAddr == nullptr)
        {
            return false;
        }

        cell params[5];
        params[0] = 4 * static_cast<cell>(sizeof(cell));
        params[1] = static_cast<cell>(vehicleId);
        params[2] = amxAddr;
        params[3] = amxAddr + static_cast<cell>(sizeof(cell));
        params[4] = amxAddr + static_cast<cell>(2 * sizeof(cell));

        const cell result = nativeFn(amx, params);
        bool success = false;
        if (result != 0)
        {
            cell* pX = nullptr;
            cell* pY = nullptr;
            cell* pZ = nullptr;
            if (amx_GetAddr(amx, params[2], &pX) == AMX_ERR_NONE && pX != nullptr &&
                amx_GetAddr(amx, params[3], &pY) == AMX_ERR_NONE && pY != nullptr &&
                amx_GetAddr(amx, params[4], &pZ) == AMX_ERR_NONE && pZ != nullptr)
            {
                x = amx_ctof(*pX);
                y = amx_ctof(*pY);
                z = amx_ctof(*pZ);
                success = true;
            }
        }

        amx_Release(amx, amxAddr);
        return success;
    }

    inline int GetVehicleModel(AMX* amx, int vehicleId)
    {
#if defined(OMP_COMPONENT)
        if (g_vehiclesComponent != nullptr)
        {
            IVehicle* veh = g_vehiclesComponent->get(vehicleId);
            if (veh != nullptr)
            {
                return veh->getModel();
            }
        }
#endif
        AMX_NATIVE nativeFn = Cache().fnGetVehicleModel;
        if (nativeFn == nullptr)
        {
            nativeFn = FindNative(amx, "GetVehicleModel");
        }
        if (nativeFn == nullptr)
        {
            return 0;
        }

        cell params[2];
        params[0] = 1 * static_cast<cell>(sizeof(cell));
        params[1] = static_cast<cell>(vehicleId);

        return static_cast<int>(nativeFn(amx, params));
    }

    // T10 (M6): Robust upside-down check using local UP vector Z component: upZ = 1 - 2*(qx*qx + qy*qy)
    inline bool IsVehicleUpsideDown(AMX* amx, int vehicleId)
    {
#if defined(OMP_COMPONENT)
        if (g_vehiclesComponent != nullptr)
        {
            IVehicle* veh = g_vehiclesComponent->get(vehicleId);
            if (veh != nullptr)
            {
                GTAQuat rot = veh->getRotation();
                const float upZ = 1.0f - 2.0f * (rot.q.x * rot.q.x + rot.q.y * rot.q.y);
                return (upZ < 0.0f);
            }
        }
#endif
        AMX_NATIVE nativeFn = Cache().fnGetVehicleRotationQuat;
        if (nativeFn == nullptr)
        {
            nativeFn = FindNative(amx, "GetVehicleRotationQuat");
        }
        if (nativeFn == nullptr)
        {
            return false;
        }

        cell amxAddr = 0;
        cell* physAddr = nullptr;
        if (amx_Allot(amx, 4, &amxAddr, &physAddr) != AMX_ERR_NONE || physAddr == nullptr)
        {
            return false;
        }

        cell params[6];
        params[0] = 5 * static_cast<cell>(sizeof(cell));
        params[1] = static_cast<cell>(vehicleId);
        params[2] = amxAddr;
        params[3] = amxAddr + static_cast<cell>(sizeof(cell));
        params[4] = amxAddr + static_cast<cell>(2 * sizeof(cell));
        params[5] = amxAddr + static_cast<cell>(3 * sizeof(cell));

        const cell result = nativeFn(amx, params);
        bool upsideDown = false;
        if (result != 0)
        {
            cell* pX = nullptr;
            cell* pY = nullptr;
            if (amx_GetAddr(amx, params[3], &pX) == AMX_ERR_NONE && pX != nullptr &&
                amx_GetAddr(amx, params[4], &pY) == AMX_ERR_NONE && pY != nullptr)
            {
                const float qx = amx_ctof(*pX);
                const float qy = amx_ctof(*pY);
                const float upZ = 1.0f - 2.0f * (qx * qx + qy * qy);
                upsideDown = (upZ < 0.0f);
            }
        }

        amx_Release(amx, amxAddr);
        return upsideDown;
    }

    // T8 (M1): Interior & Virtual World query helpers
    inline int GetPlayerInterior(AMX* amx, int playerId)
    {
#if defined(OMP_COMPONENT)
        if (g_core != nullptr)
        {
            IPlayer* player = g_core->getPlayers().get(playerId);
            if (player != nullptr)
            {
                return player->getInterior();
            }
        }
#endif
        AMX_NATIVE nativeFn = Cache().fnGetPlayerInterior;
        if (nativeFn == nullptr)
        {
            nativeFn = FindNative(amx, "GetPlayerInterior");
        }
        if (nativeFn == nullptr)
        {
            return 0;
        }

        cell params[2];
        params[0] = 1 * static_cast<cell>(sizeof(cell));
        params[1] = static_cast<cell>(playerId);
        return static_cast<int>(nativeFn(amx, params));
    }

    inline int GetPlayerVirtualWorld(AMX* amx, int playerId)
    {
#if defined(OMP_COMPONENT)
        if (g_core != nullptr)
        {
            IPlayer* player = g_core->getPlayers().get(playerId);
            if (player != nullptr)
            {
                return player->getVirtualWorld();
            }
        }
#endif
        AMX_NATIVE nativeFn = Cache().fnGetPlayerVirtualWorld;
        if (nativeFn == nullptr)
        {
            nativeFn = FindNative(amx, "GetPlayerVirtualWorld");
        }
        if (nativeFn == nullptr)
        {
            return 0;
        }

        cell params[2];
        params[0] = 1 * static_cast<cell>(sizeof(cell));
        params[1] = static_cast<cell>(playerId);
        return static_cast<int>(nativeFn(amx, params));
    }

    inline int GetVehicleVirtualWorld(AMX* amx, int vehicleId)
    {
#if defined(OMP_COMPONENT)
        if (g_vehiclesComponent != nullptr)
        {
            IVehicle* veh = g_vehiclesComponent->get(vehicleId);
            if (veh != nullptr)
            {
                return veh->getVirtualWorld();
            }
        }
#endif
        AMX_NATIVE nativeFn = Cache().fnGetVehicleVirtualWorld;
        if (nativeFn == nullptr)
        {
            nativeFn = FindNative(amx, "GetVehicleVirtualWorld");
        }
        if (nativeFn == nullptr)
        {
            return 0;
        }

        cell params[2];
        params[0] = 1 * static_cast<cell>(sizeof(cell));
        params[1] = static_cast<cell>(vehicleId);
        return static_cast<int>(nativeFn(amx, params));
    }

    // T4 (N4): Reliable vehicle interior query helper
    inline int GetVehicleInterior(AMX* amx, int vehicleId)
    {
#if defined(OMP_COMPONENT)
        if (g_vehiclesComponent != nullptr)
        {
            IVehicle* veh = g_vehiclesComponent->get(vehicleId);
            if (veh != nullptr)
            {
                return veh->getInterior();
            }
        }
#endif
        if (vehicleId >= 1 && vehicleId <= 2000)
        {
            // T5 (B1): In SA-MP, check if vehicle still exists via GetVehicleModel.
            // If vehicle does not exist (model == 0), reset entry to 0.
            int model = GetVehicleModel(amx, vehicleId);
            if (model == 0)
            {
                GetVehicleInteriorsArray()[vehicleId] = 0;
                return 0;
            }
            return GetVehicleInteriorsArray()[vehicleId];
        }
        return 0;
    }

    // Server weather query with dual caching: per-AMX retry on open.mp component until found (N1), process-wide on SA-MP plugin (C2)
    inline int GetServerWeather(AMX* amx, int playerId = -1)
    {
#if defined(OMP_COMPONENT)
        if (playerId >= 0 && g_core != nullptr)
        {
            IPlayer* player = g_core->getPlayers().get(playerId);
            if (player != nullptr)
            {
                return player->getWeather();
            }
        }

        auto& cache = Cache();
        if (cache.fnGetWeather == nullptr && cache.lastGetWeatherAmx != amx)
        {
            cache.lastGetWeatherAmx = amx;
            cache.fnGetWeather = FindNative(amx, "GetWeather");
        }
#else
        auto& cache = Cache();
        if (cache.fnGetWeather == nullptr && !cache.searchedGetWeather)
        {
            cache.fnGetWeather = FindNative(amx, "GetWeather");
            cache.searchedGetWeather = true;
        }
#endif

        if (cache.fnGetWeather != nullptr)
        {
            cell params[1] = { 0 };
            return static_cast<int>(cache.fnGetWeather(amx, params));
        }
        return -1;
    }
}

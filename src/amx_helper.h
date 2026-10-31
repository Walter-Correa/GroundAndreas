#pragma once

#include "common.h"

namespace AmxHelper
{
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
            auto* natives = reinterpret_cast<AMX_FUNCSTUB*>(amx->base + hdr->natives);
            return reinterpret_cast<AMX_NATIVE>(natives[index].address);
        }
        return nullptr;
    }

    inline bool GetPlayerPos(AMX* amx, int playerId, float& x, float& y, float& z)
    {
        AMX_NATIVE nativeFn = FindNative(amx, "GetPlayerPos");
        if (nativeFn == nullptr)
        {
            return false;
        }

        if (amx->stk - static_cast<cell>(3 * sizeof(cell)) <= amx->hea)
        {
            return false;
        }

        amx->stk -= 3 * sizeof(cell);
        const cell amxAddrX = amx->stk;
        const cell amxAddrY = amx->stk + sizeof(cell);
        const cell amxAddrZ = amx->stk + 2 * sizeof(cell);

        cell params[5];
        params[0] = 4 * sizeof(cell);
        params[1] = static_cast<cell>(playerId);
        params[2] = amxAddrX;
        params[3] = amxAddrY;
        params[4] = amxAddrZ;

        const cell result = nativeFn(amx, params);

        bool success = false;
        if (result != 0)
        {
            cell* pX = nullptr;
            cell* pY = nullptr;
            cell* pZ = nullptr;
            if (amx_GetAddr(amx, amxAddrX, &pX) == AMX_ERR_NONE && pX != nullptr &&
                amx_GetAddr(amx, amxAddrY, &pY) == AMX_ERR_NONE && pY != nullptr &&
                amx_GetAddr(amx, amxAddrZ, &pZ) == AMX_ERR_NONE && pZ != nullptr)
            {
                x = amx_ctof(*pX);
                y = amx_ctof(*pY);
                z = amx_ctof(*pZ);
                success = true;
            }
        }

        amx->stk += 3 * sizeof(cell);
        return success;
    }

    inline bool GetVehiclePos(AMX* amx, int vehicleId, float& x, float& y, float& z)
    {
        AMX_NATIVE nativeFn = FindNative(amx, "GetVehiclePos");
        if (nativeFn == nullptr)
        {
            return false;
        }

        if (amx->stk - static_cast<cell>(3 * sizeof(cell)) <= amx->hea)
        {
            return false;
        }

        amx->stk -= 3 * sizeof(cell);
        const cell amxAddrX = amx->stk;
        const cell amxAddrY = amx->stk + sizeof(cell);
        const cell amxAddrZ = amx->stk + 2 * sizeof(cell);

        cell params[5];
        params[0] = 4 * sizeof(cell);
        params[1] = static_cast<cell>(vehicleId);
        params[2] = amxAddrX;
        params[3] = amxAddrY;
        params[4] = amxAddrZ;

        const cell result = nativeFn(amx, params);

        bool success = false;
        if (result != 0)
        {
            cell* pX = nullptr;
            cell* pY = nullptr;
            cell* pZ = nullptr;
            if (amx_GetAddr(amx, amxAddrX, &pX) == AMX_ERR_NONE && pX != nullptr &&
                amx_GetAddr(amx, amxAddrY, &pY) == AMX_ERR_NONE && pY != nullptr &&
                amx_GetAddr(amx, amxAddrZ, &pZ) == AMX_ERR_NONE && pZ != nullptr)
            {
                x = amx_ctof(*pX);
                y = amx_ctof(*pY);
                z = amx_ctof(*pZ);
                success = true;
            }
        }

        amx->stk += 3 * sizeof(cell);
        return success;
    }

    inline int GetVehicleModel(AMX* amx, int vehicleId)
    {
        AMX_NATIVE nativeFn = FindNative(amx, "GetVehicleModel");
        if (nativeFn == nullptr)
        {
            return 0;
        }

        cell params[2];
        params[0] = 1 * sizeof(cell);
        params[1] = static_cast<cell>(vehicleId);

        return static_cast<int>(nativeFn(amx, params));
    }
}

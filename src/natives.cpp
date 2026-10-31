#include "natives.h"
#include "heightmap.h"
#include <vector>
#include <cstring>

namespace Natives
{
    // native bool:GroundAndreas_Init(const file_path[] = "scriptfiles/SAfull.hmap");
    cell AMX_NATIVE_CALL GroundAndreas_Init(AMX* amx, cell* params)
    {
        std::string filePath = "scriptfiles/SAfull.hmap";
        if (params[0] >= sizeof(cell))
        {
            char* pathStr = nullptr;
            amx_StrParam(amx, params[1], pathStr);
            if (pathStr != nullptr && std::strlen(pathStr) > 0)
            {
                filePath = pathStr;
            }
        }

        bool result = HeightMap::Instance().Load(filePath);
        if (!result && filePath == "scriptfiles/SAfull.hmap")
        {
            // Try fallback relative path
            result = HeightMap::Instance().Load("SAfull.hmap");
        }
        return result ? 1 : 0;
    }

    // native bool:GroundAndreas_Unload();
    cell AMX_NATIVE_CALL GroundAndreas_Unload(AMX* amx, cell* params)
    {
        HeightMap::Instance().Unload();
        return 1;
    }

    // native bool:GroundAndreas_IsLoaded();
    cell AMX_NATIVE_CALL GroundAndreas_IsLoaded(AMX* amx, cell* params)
    {
        return HeightMap::Instance().IsLoaded() ? 1 : 0;
    }

    // native GetGroundZPos(Float:x, Float:y, &Float:z, bool:interpolate = true, bool:water_surface = true);
    cell AMX_NATIVE_CALL GetGroundZPos(AMX* amx, cell* params)
    {
        if (params[0] < (3 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const bool interpolate = (params[0] >= 4 * sizeof(cell)) ? (params[4] != 0) : true;
        const bool waterSurface = (params[0] >= 5 * sizeof(cell)) ? (params[5] != 0) : true;

        float z = 0.0f;
        const bool success = HeightMap::Instance().GetZ(x, y, z, interpolate, waterSurface);

        cell* zAddr = nullptr;
        if (amx_GetAddr(amx, params[3], &zAddr) == AMX_ERR_NONE && zAddr != nullptr)
        {
            *zAddr = amx_ftoc(z);
        }

        return success ? 1 : 0;
    }

    // native bool:IsWorldBlockingLine(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2, &Float:hit_x = 0.0, &Float:hit_y = 0.0, &Float:hit_z = 0.0);
    cell AMX_NATIVE_CALL IsWorldBlockingLine(AMX* amx, cell* params)
    {
        if (params[0] < (6 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x1 = amx_ctof(params[1]);
        const float y1 = amx_ctof(params[2]);
        const float z1 = amx_ctof(params[3]);
        const float x2 = amx_ctof(params[4]);
        const float y2 = amx_ctof(params[5]);
        const float z2 = amx_ctof(params[6]);

        float hitX = 0.0f, hitY = 0.0f, hitZ = 0.0f;
        const bool hit = HeightMap::Instance().RaycastLine(x1, y1, z1, x2, y2, z2, hitX, hitY, hitZ);

        if (params[0] >= 7 * sizeof(cell))
        {
            cell* hxAddr = nullptr;
            if (amx_GetAddr(amx, params[7], &hxAddr) == AMX_ERR_NONE && hxAddr != nullptr)
            {
                *hxAddr = amx_ftoc(hitX);
            }
        }

        if (params[0] >= 8 * sizeof(cell))
        {
            cell* hyAddr = nullptr;
            if (amx_GetAddr(amx, params[8], &hyAddr) == AMX_ERR_NONE && hyAddr != nullptr)
            {
                *hyAddr = amx_ftoc(hitY);
            }
        }

        if (params[0] >= 9 * sizeof(cell))
        {
            cell* hzAddr = nullptr;
            if (amx_GetAddr(amx, params[9], &hzAddr) == AMX_ERR_NONE && hzAddr != nullptr)
            {
                *hzAddr = amx_ftoc(hitZ);
            }
        }

        return hit ? 1 : 0;
    }

    // native bool:GetGroundNormal(Float:x, Float:y, &Float:nx, &Float:ny, &Float:nz, &Float:slope_degrees);
    cell AMX_NATIVE_CALL GetGroundNormal(AMX* amx, cell* params)
    {
        if (params[0] < (6 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);

        float nx = 0.0f, ny = 0.0f, nz = 1.0f, slope = 0.0f;
        const bool success = HeightMap::Instance().GetNormal(x, y, nx, ny, nz, slope);

        cell* addr = nullptr;
        if (amx_GetAddr(amx, params[3], &addr) == AMX_ERR_NONE && addr != nullptr)
        {
            *addr = amx_ftoc(nx);
        }
        if (amx_GetAddr(amx, params[4], &addr) == AMX_ERR_NONE && addr != nullptr)
        {
            *addr = amx_ftoc(ny);
        }
        if (amx_GetAddr(amx, params[5], &addr) == AMX_ERR_NONE && addr != nullptr)
        {
            *addr = amx_ftoc(nz);
        }
        if (amx_GetAddr(amx, params[6], &addr) == AMX_ERR_NONE && addr != nullptr)
        {
            *addr = amx_ftoc(slope);
        }

        return success ? 1 : 0;
    }

    // native bool:GetGroundObjectRotation(Float:x, Float:y, Float:facing_angle, &Float:rx, &Float:ry);
    cell AMX_NATIVE_CALL GetGroundObjectRotation(AMX* amx, cell* params)
    {
        if (params[0] < (5 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float facingAngle = amx_ctof(params[3]);

        float rx = 0.0f, ry = 0.0f;
        const bool success = HeightMap::Instance().GetObjectRotation(x, y, facingAngle, rx, ry);

        cell* rxAddr = nullptr;
        if (amx_GetAddr(amx, params[4], &rxAddr) == AMX_ERR_NONE && rxAddr != nullptr)
        {
            *rxAddr = amx_ftoc(rx);
        }

        cell* ryAddr = nullptr;
        if (amx_GetAddr(amx, params[5], &ryAddr) == AMX_ERR_NONE && ryAddr != nullptr)
        {
            *ryAddr = amx_ftoc(ry);
        }

        return success ? 1 : 0;
    }

    // native bool:IsGroundUnderwater(Float:x, Float:y, &Float:water_depth = 0.0);
    cell AMX_NATIVE_CALL IsGroundUnderwater(AMX* amx, cell* params)
    {
        if (params[0] < (2 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);

        float waterDepth = 0.0f;
        const bool underwater = HeightMap::Instance().IsUnderwater(x, y, waterDepth);

        if (params[0] >= 3 * sizeof(cell))
        {
            cell* depthAddr = nullptr;
            if (amx_GetAddr(amx, params[3], &depthAddr) == AMX_ERR_NONE && depthAddr != nullptr)
            {
                *depthAddr = amx_ftoc(waterDepth);
            }
        }

        return underwater ? 1 : 0;
    }

    // native bool:GetGroundClearance(Float:x, Float:y, Float:z, &Float:clearance, &bool:is_underground = false);
    cell AMX_NATIVE_CALL GetGroundClearance(AMX* amx, cell* params)
    {
        if (params[0] < (4 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float z = amx_ctof(params[3]);

        float clearance = 0.0f;
        bool isUnderground = false;
        const bool success = HeightMap::Instance().GetClearance(x, y, z, clearance, isUnderground);

        cell* clAddr = nullptr;
        if (amx_GetAddr(amx, params[4], &clAddr) == AMX_ERR_NONE && clAddr != nullptr)
        {
            *clAddr = amx_ftoc(clearance);
        }

        if (params[0] >= 5 * sizeof(cell))
        {
            cell* ugAddr = nullptr;
            if (amx_GetAddr(amx, params[5], &ugAddr) == AMX_ERR_NONE && ugAddr != nullptr)
            {
                *ugAddr = isUnderground ? 1 : 0;
            }
        }

        return success ? 1 : 0;
    }

    // native GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points, bool:interpolate = true);
    cell AMX_NATIVE_CALL GetGroundZPosBatch(AMX* amx, cell* params)
    {
        if (params[0] < (4 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        cell* xAddr = nullptr;
        cell* yAddr = nullptr;
        cell* zAddr = nullptr;

        if (amx_GetAddr(amx, params[1], &xAddr) != AMX_ERR_NONE || xAddr == nullptr ||
            amx_GetAddr(amx, params[2], &yAddr) != AMX_ERR_NONE || yAddr == nullptr ||
            amx_GetAddr(amx, params[3], &zAddr) != AMX_ERR_NONE || zAddr == nullptr)
        {
            return 0;
        }

        const int totalPoints = params[4];
        if (totalPoints <= 0)
        {
            return 0;
        }

        const bool interpolate = (params[0] >= 5 * sizeof(cell)) ? (params[5] != 0) : true;

        std::vector<float> xVec(totalPoints);
        std::vector<float> yVec(totalPoints);
        std::vector<float> zVec(totalPoints, 0.0f);

        for (int i = 0; i < totalPoints; ++i)
        {
            xVec[i] = amx_ctof(xAddr[i]);
            yVec[i] = amx_ctof(yAddr[i]);
        }

        const int processed = HeightMap::Instance().GetZBatch(xVec.data(), yVec.data(), zVec.data(), totalPoints, interpolate);

        for (int i = 0; i < totalPoints; ++i)
        {
            zAddr[i] = amx_ftoc(zVec[i]);
        }

        return processed;
    }
}

AMX_NATIVE_INFO PluginNatives[] =
{
    { "GroundAndreas_Init",               Natives::GroundAndreas_Init },
    { "GroundAndreas_Unload",             Natives::GroundAndreas_Unload },
    { "GroundAndreas_IsLoaded",           Natives::GroundAndreas_IsLoaded },
    { "GetGroundZPos",              Natives::GetGroundZPos },
    { "GetGroundZ",                 Natives::GetGroundZPos },
    { "IsWorldBlockingLine",        Natives::IsWorldBlockingLine },
    { "GetGroundNormal",            Natives::GetGroundNormal },
    { "GetGroundObjectRotation",    Natives::GetGroundObjectRotation },
    { "IsGroundUnderwater",         Natives::IsGroundUnderwater },
    { "GetGroundClearance",         Natives::GetGroundClearance },
    { "GetGroundZPosBatch",         Natives::GetGroundZPosBatch },
    { "GetGroundZBatch",            Natives::GetGroundZPosBatch },
    { nullptr, nullptr }
};

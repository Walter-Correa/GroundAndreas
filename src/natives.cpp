#include "natives.h"
#include "heightmap.h"
#include "water.h"
#include "vehicle_data.h"
#include "amx_helper.h"
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
            *zAddr = FloatToCell(z);
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
                *hxAddr = FloatToCell(hitX);
            }
        }

        if (params[0] >= 8 * sizeof(cell))
        {
            cell* hyAddr = nullptr;
            if (amx_GetAddr(amx, params[8], &hyAddr) == AMX_ERR_NONE && hyAddr != nullptr)
            {
                *hyAddr = FloatToCell(hitY);
            }
        }

        if (params[0] >= 9 * sizeof(cell))
        {
            cell* hzAddr = nullptr;
            if (amx_GetAddr(amx, params[9], &hzAddr) == AMX_ERR_NONE && hzAddr != nullptr)
            {
                *hzAddr = FloatToCell(hitZ);
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
            *addr = FloatToCell(nx);
        }
        if (amx_GetAddr(amx, params[4], &addr) == AMX_ERR_NONE && addr != nullptr)
        {
            *addr = FloatToCell(ny);
        }
        if (amx_GetAddr(amx, params[5], &addr) == AMX_ERR_NONE && addr != nullptr)
        {
            *addr = FloatToCell(nz);
        }
        if (amx_GetAddr(amx, params[6], &addr) == AMX_ERR_NONE && addr != nullptr)
        {
            *addr = FloatToCell(slope);
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
            *rxAddr = FloatToCell(rx);
        }

        cell* ryAddr = nullptr;
        if (amx_GetAddr(amx, params[5], &ryAddr) == AMX_ERR_NONE && ryAddr != nullptr)
        {
            *ryAddr = FloatToCell(ry);
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
                *depthAddr = FloatToCell(waterDepth);
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
            *clAddr = FloatToCell(clearance);
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
            zAddr[i] = FloatToCell(zVec[i]);
        }

        return processed;
    }

    // native bool:IsPointUnderGround(Float:x, Float:y, Float:z);
    cell AMX_NATIVE_CALL IsPointUnderGround(AMX* amx, cell* params)
    {
        if (params[0] < (3 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float z = amx_ctof(params[3]);

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(x, y, groundZ, true, false))
        {
            return 0;
        }

        return (z < (groundZ - 0.05f)) ? 1 : 0;
    }

    // native bool:IsPlayerUnderGround(playerid);
    cell AMX_NATIVE_CALL IsPlayerUnderGround(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int playerId = params[1];
        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, px, py, pz))
        {
            return 0;
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(px, py, groundZ, true, false))
        {
            return 0;
        }

        const float feetZ = pz - 0.90f;
        return (feetZ < (groundZ - 0.30f)) ? 1 : 0;
    }

    // native bool:IsVehicleUnderGround(vehicleid);
    cell AMX_NATIVE_CALL IsVehicleUnderGround(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int vehicleId = params[1];
        float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, vx, vy, vz))
        {
            return 0;
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        const float sizeZ = GetVehicleModelSizeZ(modelId);

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(vx, vy, groundZ, true, false))
        {
            return 0;
        }

        const float wheelsZ = vz - (sizeZ * 0.5f);
        return (wheelsZ < (groundZ - 0.40f)) ? 1 : 0;
    }

    // native Float:GetPointGroundDistance(Float:x, Float:y, Float:z);
    cell AMX_NATIVE_CALL GetPointGroundDistance(AMX* amx, cell* params)
    {
        if (params[0] < (3 * static_cast<cell>(sizeof(cell))))
        {
            return FloatToCell(0.0f);
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float z = amx_ctof(params[3]);

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(x, y, groundZ, true, false))
        {
            return FloatToCell(0.0f);
        }

        return FloatToCell(z - groundZ);
    }

    // native Float:GetPlayerGroundDistance(playerid);
    cell AMX_NATIVE_CALL GetPlayerGroundDistance(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return FloatToCell(0.0f);
        }

        const int playerId = params[1];
        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, px, py, pz))
        {
            return FloatToCell(0.0f);
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(px, py, groundZ, true, false))
        {
            return FloatToCell(0.0f);
        }

        const float feetZ = pz - 0.90f;
        return FloatToCell(feetZ - groundZ);
    }

    // native Float:GetVehicleGroundDistance(vehicleid);
    cell AMX_NATIVE_CALL GetVehicleGroundDistance(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return FloatToCell(0.0f);
        }

        const int vehicleId = params[1];
        float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, vx, vy, vz))
        {
            return FloatToCell(0.0f);
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        const float sizeZ = GetVehicleModelSizeZ(modelId);

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(vx, vy, groundZ, true, false))
        {
            return FloatToCell(0.0f);
        }

        const float wheelsZ = vz - (sizeZ * 0.5f);
        return FloatToCell(wheelsZ - groundZ);
    }

    // native bool:IsPointInWater(Float:x, Float:y, Float:z, weatherid = -1);
    cell AMX_NATIVE_CALL IsPointInWater(AMX* amx, cell* params)
    {
        if (params[0] < (3 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float z = amx_ctof(params[3]);
        const int weatherid = (params[0] >= 4 * sizeof(cell)) ? params[4] : -1;

        return WaterSystem::IsPointInWater(x, y, z, weatherid) ? 1 : 0;
    }

    // native bool:IsPointUnderWater(Float:x, Float:y, Float:z, weatherid = -1);
    cell AMX_NATIVE_CALL IsPointUnderWater(AMX* amx, cell* params)
    {
        if (params[0] < (3 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float z = amx_ctof(params[3]);
        const int weatherid = (params[0] >= 4 * sizeof(cell)) ? params[4] : -1;

        return WaterSystem::IsPointUnderWater(x, y, z, weatherid) ? 1 : 0;
    }

    // native bool:IsPlayerInWater(playerid, weatherid = -1);
    cell AMX_NATIVE_CALL IsPlayerInWater(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int playerId = params[1];
        const int weatherid = (params[0] >= 2 * sizeof(cell)) ? params[2] : -1;

        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, px, py, pz))
        {
            return 0;
        }

        return WaterSystem::IsPlayerInWater(px, py, pz, weatherid) ? 1 : 0;
    }

    // native bool:IsPlayerUnderWater(playerid, weatherid = -1);
    cell AMX_NATIVE_CALL IsPlayerUnderWater(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int playerId = params[1];
        const int weatherid = (params[0] >= 2 * sizeof(cell)) ? params[2] : -1;

        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, px, py, pz))
        {
            return 0;
        }

        return WaterSystem::IsPlayerUnderWater(px, py, pz, weatherid) ? 1 : 0;
    }

    // native bool:IsVehicleInWater(vehicleid, weatherid = -1);
    cell AMX_NATIVE_CALL IsVehicleInWater(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int vehicleId = params[1];
        const int weatherid = (params[0] >= 2 * sizeof(cell)) ? params[2] : -1;

        float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, vx, vy, vz))
        {
            return 0;
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        const float sizeZ = GetVehicleModelSizeZ(modelId);

        return WaterSystem::IsVehicleInWater(vx, vy, vz, sizeZ, weatherid) ? 1 : 0;
    }

    // native bool:IsVehicleUnderWater(vehicleid, weatherid = -1);
    cell AMX_NATIVE_CALL IsVehicleUnderWater(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int vehicleId = params[1];
        const int weatherid = (params[0] >= 2 * sizeof(cell)) ? params[2] : -1;

        float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, vx, vy, vz))
        {
            return 0;
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        const float sizeZ = GetVehicleModelSizeZ(modelId);

        return WaterSystem::IsVehicleUnderWater(vx, vy, vz, sizeZ, weatherid) ? 1 : 0;
    }

    // native GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);
    cell AMX_NATIVE_CALL GetWaterLevel(AMX* amx, cell* params)
    {
        if (params[0] < (3 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const int weatherid = (params[0] >= 4 * sizeof(cell)) ? params[4] : -1;

        float surfaceZ = 0.0f;
        float bottomZ = 0.0f;
        const bool hasWater = WaterSystem::GetWaterLevelAt(x, y, surfaceZ, bottomZ, weatherid);

        cell* outAddr = nullptr;
        if (amx_GetAddr(amx, params[3], &outAddr) == AMX_ERR_NONE && outAddr != nullptr)
        {
            *outAddr = FloatToCell(hasWater ? surfaceZ : 0.0f);
        }

        return hasWater ? 1 : 0;
    }

    // native GroundAndreasRemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
    cell AMX_NATIVE_CALL GroundAndreasRemoveBuilding(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(5 * sizeof(cell)))
        {
            return 0;
        }

        const int modelId = params[1];
        const float x = CellToFloat(params[2]);
        const float y = CellToFloat(params[3]);
        const float z = CellToFloat(params[4]);
        const float radius = CellToFloat(params[5]);

        return HeightMap::Instance().RemoveBuilding(modelId, x, y, z, radius) ? 1 : 0;
    }

    // native ApplyObjectToGroundAndreas(modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, bool:force_replace = false, Float:custom_sizex = 0.0, Float:custom_sizey = 0.0, Float:custom_sizez = 0.0);
    cell AMX_NATIVE_CALL ApplyObjectToGroundAndreas(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(7 * sizeof(cell)))
        {
            return 0;
        }

        const int modelId = params[1];
        const float x = CellToFloat(params[2]);
        const float y = CellToFloat(params[3]);
        const float z = CellToFloat(params[4]);
        const float rx = CellToFloat(params[5]);
        const float ry = CellToFloat(params[6]);
        const float rz = CellToFloat(params[7]);

        bool forceReplace = false;
        if (params[0] >= static_cast<cell>(8 * sizeof(cell)))
        {
            forceReplace = (params[8] != 0);
        }

        float customSx = 0.0f, customSy = 0.0f, customSz = 0.0f;
        if (params[0] >= static_cast<cell>(9 * sizeof(cell)))
        {
            customSx = CellToFloat(params[9]);
        }
        if (params[0] >= static_cast<cell>(10 * sizeof(cell)))
        {
            customSy = CellToFloat(params[10]);
        }
        if (params[0] >= static_cast<cell>(11 * sizeof(cell)))
        {
            customSz = CellToFloat(params[11]);
        }

        return HeightMap::Instance().ApplyObject(modelId, x, y, z, rx, ry, rz, forceReplace, customSx, customSy, customSz) ? 1 : 0;
    }

    // native bool:IsGroundSafeForSpawn(Float:x, Float:y, Float:max_slope = 15.0);
    cell AMX_NATIVE_CALL IsGroundSafeForSpawn(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(2 * sizeof(cell)))
        {
            return 0;
        }

        const float x = CellToFloat(params[1]);
        const float y = CellToFloat(params[2]);
        const float maxSlope = (params[0] >= static_cast<cell>(3 * sizeof(cell))) ? CellToFloat(params[3]) : 15.0f;

        return HeightMap::Instance().IsSafeForSpawn(x, y, maxSlope) ? 1 : 0;
    }

    // native bool:GetSafeGroundSpawn(Float:search_x, Float:search_y, &Float:safe_x, &Float:safe_y, &Float:safe_z, Float:max_slope = 15.0, Float:search_radius = 50.0);
    cell AMX_NATIVE_CALL GetSafeGroundSpawn(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(5 * sizeof(cell)))
        {
            return 0;
        }

        const float searchX = CellToFloat(params[1]);
        const float searchY = CellToFloat(params[2]);
        const float maxSlope = (params[0] >= static_cast<cell>(6 * sizeof(cell))) ? CellToFloat(params[6]) : 15.0f;
        const float searchRadius = (params[0] >= static_cast<cell>(7 * sizeof(cell))) ? CellToFloat(params[7]) : 50.0f;

        float safeX = 0.0f, safeY = 0.0f, safeZ = 0.0f;
        bool result = HeightMap::Instance().GetSafeSpawn(searchX, searchY, safeX, safeY, safeZ, maxSlope, searchRadius);

        cell* addrX = nullptr;
        if (amx_GetAddr(amx, params[3], &addrX) == AMX_ERR_NONE && addrX != nullptr)
        {
            *addrX = FloatToCell(safeX);
        }

        cell* addrY = nullptr;
        if (amx_GetAddr(amx, params[4], &addrY) == AMX_ERR_NONE && addrY != nullptr)
        {
            *addrY = FloatToCell(safeY);
        }

        cell* addrZ = nullptr;
        if (amx_GetAddr(amx, params[5], &addrZ) == AMX_ERR_NONE && addrZ != nullptr)
        {
            *addrZ = FloatToCell(safeZ);
        }

        return result ? 1 : 0;
    }

    // native bool:GetGroundImpactTime(Float:x, Float:y, Float:z, Float:vz, &Float:seconds_to_impact, &Float:impact_z, Float:gravity = 9.8);
    cell AMX_NATIVE_CALL GetGroundImpactTime(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(6 * sizeof(cell)))
        {
            return 0;
        }

        const float x = CellToFloat(params[1]);
        const float y = CellToFloat(params[2]);
        const float z = CellToFloat(params[3]);
        const float vz = CellToFloat(params[4]);
        const float gravity = (params[0] >= static_cast<cell>(7 * sizeof(cell))) ? CellToFloat(params[7]) : 9.8f;

        float secondsToImpact = 0.0f, impactZ = 0.0f;
        bool result = HeightMap::Instance().GetImpactTime(x, y, z, vz, secondsToImpact, impactZ, gravity);

        cell* addrSec = nullptr;
        if (amx_GetAddr(amx, params[5], &addrSec) == AMX_ERR_NONE && addrSec != nullptr)
        {
            *addrSec = FloatToCell(secondsToImpact);
        }

        cell* addrImpZ = nullptr;
        if (amx_GetAddr(amx, params[6], &addrImpZ) == AMX_ERR_NONE && addrImpZ != nullptr)
        {
            *addrImpZ = FloatToCell(impactZ);
        }

        return result ? 1 : 0;
    }

    // native bool:IsPointInRadarShadow(Float:radar_x, Float:radar_y, Float:radar_z, Float:target_x, Float:target_y, Float:target_z);
    cell AMX_NATIVE_CALL IsPointInRadarShadow(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(6 * sizeof(cell)))
        {
            return 0;
        }

        const float rx = CellToFloat(params[1]);
        const float ry = CellToFloat(params[2]);
        const float rz = CellToFloat(params[3]);
        const float tx = CellToFloat(params[4]);
        const float ty = CellToFloat(params[5]);
        const float tz = CellToFloat(params[6]);

        return HeightMap::Instance().IsPointInRadarShadow(rx, ry, rz, tx, ty, tz) ? 1 : 0;
    }

    // native bool:GetAreaElevationStats(Float:minx, Float:miny, Float:maxx, Float:maxy, &Float:min_z, &Float:max_z, &Float:avg_z, &Float:max_slope_deg);
    cell AMX_NATIVE_CALL GetAreaElevationStats(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(8 * sizeof(cell)))
        {
            return 0;
        }

        const float minX = CellToFloat(params[1]);
        const float minY = CellToFloat(params[2]);
        const float maxX = CellToFloat(params[3]);
        const float maxY = CellToFloat(params[4]);

        float minZ = 0.0f, maxZ = 0.0f, avgZ = 0.0f, maxSlopeDeg = 0.0f;
        bool result = HeightMap::Instance().GetAreaElevationStats(minX, minY, maxX, maxY, minZ, maxZ, avgZ, maxSlopeDeg);

        cell* addrMinZ = nullptr;
        if (amx_GetAddr(amx, params[5], &addrMinZ) == AMX_ERR_NONE && addrMinZ != nullptr)
        {
            *addrMinZ = FloatToCell(minZ);
        }

        cell* addrMaxZ = nullptr;
        if (amx_GetAddr(amx, params[6], &addrMaxZ) == AMX_ERR_NONE && addrMaxZ != nullptr)
        {
            *addrMaxZ = FloatToCell(maxZ);
        }

        cell* addrAvgZ = nullptr;
        if (amx_GetAddr(amx, params[7], &addrAvgZ) == AMX_ERR_NONE && addrAvgZ != nullptr)
        {
            *addrAvgZ = FloatToCell(avgZ);
        }

        cell* addrSlope = nullptr;
        if (amx_GetAddr(amx, params[8], &addrSlope) == AMX_ERR_NONE && addrSlope != nullptr)
        {
            *addrSlope = FloatToCell(maxSlopeDeg);
        }

        return result ? 1 : 0;
    }

    // native bool:SaveGroundAndreasPatches(const filename[]);
    cell AMX_NATIVE_CALL SaveGroundAndreasPatches(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        char* pathStr = nullptr;
        amx_StrParam(amx, params[1], pathStr);
        if (pathStr == nullptr || std::strlen(pathStr) == 0)
        {
            return 0;
        }

        return HeightMap::Instance().SavePatches(pathStr) ? 1 : 0;
    }

    // native bool:LoadGroundAndreasPatches(const filename[]);
    cell AMX_NATIVE_CALL LoadGroundAndreasPatches(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        char* pathStr = nullptr;
        amx_StrParam(amx, params[1], pathStr);
        if (pathStr == nullptr || std::strlen(pathStr) == 0)
        {
            return 0;
        }

        return HeightMap::Instance().LoadPatches(pathStr) ? 1 : 0;
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
    { "IsPointUnderGround",         Natives::IsPointUnderGround },
    { "IsPlayerUnderGround",        Natives::IsPlayerUnderGround },
    { "IsVehicleUnderGround",       Natives::IsVehicleUnderGround },
    { "GetPointGroundDistance",     Natives::GetPointGroundDistance },
    { "GetPlayerGroundDistance",    Natives::GetPlayerGroundDistance },
    { "GetVehicleGroundDistance",   Natives::GetVehicleGroundDistance },
    { "IsPointInWater",             Natives::IsPointInWater },
    { "IsPointUnderWater",          Natives::IsPointUnderWater },
    { "IsPlayerInWater",            Natives::IsPlayerInWater },
    { "IsPlayerUnderWater",         Natives::IsPlayerUnderWater },
    { "IsVehicleInWater",           Natives::IsVehicleInWater },
    { "IsVehicleUnderWater",        Natives::IsVehicleUnderWater },
    { "GetWaterLevel",              Natives::GetWaterLevel },
    { "GroundAndreasRemoveBuilding",      Natives::GroundAndreasRemoveBuilding },
    { "ApplyObjectToGroundAndreas",       Natives::ApplyObjectToGroundAndreas },
    { "IsGroundSafeForSpawn",       Natives::IsGroundSafeForSpawn },
    { "GetSafeGroundSpawn",         Natives::GetSafeGroundSpawn },
    { "GetGroundImpactTime",        Natives::GetGroundImpactTime },
    { "IsPointInRadarShadow",       Natives::IsPointInRadarShadow },
    { "GetAreaElevationStats",      Natives::GetAreaElevationStats },
    { "SaveGroundAndreasPatches",         Natives::SaveGroundAndreasPatches },
    { "LoadGroundAndreasPatches",         Natives::LoadGroundAndreasPatches },
    { nullptr, nullptr }
};

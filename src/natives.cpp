#include "natives.h"
#include "heightmap.h"
#include "water.h"
#include "vehicle_data.h"
#include "amx_helper.h"
#include <vector>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <exception>

namespace Natives
{
    // native bool:GroundAndreas_Init(const file_path[] = "scriptfiles/SAfull.hmap");
    cell AMX_NATIVE_CALL GroundAndreas_Init(AMX* amx, cell* params)
    {
        std::string filePath = "scriptfiles/SAfull.hmap";
        if (params[0] >= sizeof(cell))
        {
            std::string userPath;
            if (AmxHelper::GetString(amx, params[1], userPath) && !userPath.empty())
            {
                if (!AmxHelper::IsPathSafe(userPath))
                {
                    return 0;
                }
                filePath = userPath;
            }
        }

        bool result = HeightMap::Instance().Load(filePath);
        if (!result && filePath == "scriptfiles/SAfull.hmap")
        {
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
        if (!std::isfinite(x) || !std::isfinite(y))
        {
            return 0;
        }

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

    // native GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points = sizeof(points_x), bool:interpolate = true, bool:water_surface = true);
    // T1 (C1): Safe bounds validation & zero std::vector allocations
    cell AMX_NATIVE_CALL GetGroundZPosBatch(AMX* amx, cell* params)
    {
        if (params[0] < (4 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const cell totalPoints = params[4];
        if (totalPoints <= 0 || totalPoints > 1000000)
        {
            return 0;
        }

        const bool interpolate = (params[0] >= 5 * sizeof(cell)) ? (params[5] != 0) : true;
        const bool waterSurface = (params[0] >= 6 * sizeof(cell)) ? (params[6] != 0) : true;

        cell* pX = nullptr;
        cell* pY = nullptr;
        cell* pZ = nullptr;

        if (!AmxHelper::GetArray(amx, params[1], totalPoints, pX) ||
            !AmxHelper::GetArray(amx, params[2], totalPoints, pY) ||
            !AmxHelper::GetArray(amx, params[3], totalPoints, pZ))
        {
            return 0;
        }

        for (cell i = 0; i < totalPoints; ++i)
        {
            const float x = amx_ctof(pX[i]);
            const float y = amx_ctof(pY[i]);
            float z = 0.0f;
            if (std::isfinite(x) && std::isfinite(y))
            {
                HeightMap::Instance().GetZ(x, y, z, interpolate, waterSurface);
            }
            pZ[i] = FloatToCell(z);
        }

        return totalPoints;
    }

    // native bool:GetGroundZPosArea(Float:minx, Float:miny, Float:maxx, Float:maxy, &Float:avg_z, &Float:max_z, &Float:min_z, bool:water_surface = true);
    cell AMX_NATIVE_CALL GetGroundZPosArea(AMX* amx, cell* params)
    {
        if (params[0] < (7 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float minX = amx_ctof(params[1]);
        const float minY = amx_ctof(params[2]);
        const float maxX = amx_ctof(params[3]);
        const float maxY = amx_ctof(params[4]);
        if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(maxX) || !std::isfinite(maxY))
        {
            return 0;
        }

        const bool waterSurface = (params[0] >= 8 * sizeof(cell)) ? (params[8] != 0) : true;

        float avgZ = 0.0f, maxZ = 0.0f, minZ = 0.0f;
        const bool success = HeightMap::Instance().GetGroundZPosArea(minX, minY, maxX, maxY, avgZ, maxZ, minZ, waterSurface);

        cell* pAvgZ = nullptr;
        cell* pMaxZ = nullptr;
        cell* pMinZ = nullptr;

        if (amx_GetAddr(amx, params[5], &pAvgZ) == AMX_ERR_NONE && pAvgZ &&
            amx_GetAddr(amx, params[6], &pMaxZ) == AMX_ERR_NONE && pMaxZ &&
            amx_GetAddr(amx, params[7], &pMinZ) == AMX_ERR_NONE && pMinZ)
        {
            *pAvgZ = FloatToCell(avgZ);
            *pMaxZ = FloatToCell(maxZ);
            *pMinZ = FloatToCell(minZ);
        }

        return success ? 1 : 0;
    }

    // native bool:GetGroundZPosRadius(Float:center_x, Float:center_y, Float:radius, &Float:avg_z, &Float:max_z, &Float:min_z, bool:water_surface = true);
    cell AMX_NATIVE_CALL GetGroundZPosRadius(AMX* amx, cell* params)
    {
        if (params[0] < (6 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float cx = amx_ctof(params[1]);
        const float cy = amx_ctof(params[2]);
        const float radius = amx_ctof(params[3]);
        if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(radius))
        {
            return 0;
        }

        const bool waterSurface = (params[0] >= 7 * sizeof(cell)) ? (params[7] != 0) : true;

        float avgZ = 0.0f, maxZ = 0.0f, minZ = 0.0f;
        const bool success = HeightMap::Instance().GetGroundZPosRadius(cx, cy, radius, avgZ, maxZ, minZ, waterSurface);

        cell* pAvgZ = nullptr;
        cell* pMaxZ = nullptr;
        cell* pMinZ = nullptr;

        if (amx_GetAddr(amx, params[4], &pAvgZ) == AMX_ERR_NONE && pAvgZ &&
            amx_GetAddr(amx, params[5], &pMaxZ) == AMX_ERR_NONE && pMaxZ &&
            amx_GetAddr(amx, params[6], &pMinZ) == AMX_ERR_NONE && pMinZ)
        {
            *pAvgZ = FloatToCell(avgZ);
            *pMaxZ = FloatToCell(maxZ);
            *pMinZ = FloatToCell(minZ);
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
        if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(z1) ||
            !std::isfinite(x2) || !std::isfinite(y2) || !std::isfinite(z2))
        {
            return 0;
        }

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
        if (!std::isfinite(x) || !std::isfinite(y))
        {
            return 0;
        }

        float nx = 0.0f, ny = 0.0f, nz = 1.0f, slope = 0.0f;
        const bool success = HeightMap::Instance().GetNormal(x, y, nx, ny, nz, slope);

        cell* addr = nullptr;
        if (amx_GetAddr(amx, params[3], &addr) == AMX_ERR_NONE && addr != nullptr) *addr = FloatToCell(nx);
        if (amx_GetAddr(amx, params[4], &addr) == AMX_ERR_NONE && addr != nullptr) *addr = FloatToCell(ny);
        if (amx_GetAddr(amx, params[5], &addr) == AMX_ERR_NONE && addr != nullptr) *addr = FloatToCell(nz);
        if (amx_GetAddr(amx, params[6], &addr) == AMX_ERR_NONE && addr != nullptr) *addr = FloatToCell(slope);

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
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(facingAngle))
        {
            return 0;
        }

        float rx = 0.0f, ry = 0.0f;
        const bool success = HeightMap::Instance().GetObjectRotation(x, y, facingAngle, rx, ry);

        cell* addrRX = nullptr;
        cell* addrRY = nullptr;
        if (amx_GetAddr(amx, params[4], &addrRX) == AMX_ERR_NONE && addrRX != nullptr &&
            amx_GetAddr(amx, params[5], &addrRY) == AMX_ERR_NONE && addrRY != nullptr)
        {
            *addrRX = FloatToCell(rx);
            *addrRY = FloatToCell(ry);
        }

        return success ? 1 : 0;
    }

    // native bool:GetGroundObjectPose(Float:x, Float:y, Float:facing_angle, Float:z_offset, &Float:z, &Float:rx, &Float:ry, bool:interpolate = true);
    cell AMX_NATIVE_CALL GetGroundObjectPose(AMX* amx, cell* params)
    {
        if (params[0] < (7 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float facingAngle = amx_ctof(params[3]);
        const float zOffset = amx_ctof(params[4]);
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(facingAngle) || !std::isfinite(zOffset))
        {
            return 0;
        }

        const bool interpolate = (params[0] >= (8 * static_cast<cell>(sizeof(cell)))) ? (params[8] != 0) : true;

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(x, y, groundZ, interpolate, true))
        {
            return 0;
        }

        float rx = 0.0f, ry = 0.0f;
        HeightMap::Instance().GetObjectRotation(x, y, facingAngle, rx, ry);

        cell* pZ = nullptr;
        cell* pRX = nullptr;
        cell* pRY = nullptr;
        if (amx_GetAddr(amx, params[5], &pZ) == AMX_ERR_NONE && pZ &&
            amx_GetAddr(amx, params[6], &pRX) == AMX_ERR_NONE && pRX &&
            amx_GetAddr(amx, params[7], &pRY) == AMX_ERR_NONE && pRY)
        {
            *pZ = FloatToCell(groundZ + zOffset);
            *pRX = FloatToCell(rx);
            *pRY = FloatToCell(ry);
            return 1;
        }

        return 0;
    }

    // native GetGroundTwoPointRotation(Float:x1, Float:y1, Float:x2, Float:y2, Float:z_offset, &Float:center_z, &Float:rx, &Float:ry, &Float:rz, bool:interpolate = true);
    cell AMX_NATIVE_CALL GetGroundTwoPointRotation(AMX* amx, cell* params)
    {
        if (params[0] < (9 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x1 = amx_ctof(params[1]);
        const float y1 = amx_ctof(params[2]);
        const float x2 = amx_ctof(params[3]);
        const float y2 = amx_ctof(params[4]);
        const float zOffset = amx_ctof(params[5]);
        if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2) || !std::isfinite(zOffset))
        {
            return 0;
        }

        const bool interpolate = (params[0] >= (10 * static_cast<cell>(sizeof(cell)))) ? (params[10] != 0) : true;

        float z1 = 0.0f, z2 = 0.0f;
        if (!HeightMap::Instance().GetZ(x1, y1, z1, interpolate, true) ||
            !HeightMap::Instance().GetZ(x2, y2, z2, interpolate, true))
        {
            return 0;
        }

        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float dz = z2 - z1;
        const float dist2D = std::sqrt(dx * dx + dy * dy);

        if (dist2D < 0.001f || !std::isfinite(dist2D))
        {
            return 0;
        }

        float rz = std::atan2(dy, dx) * RAD_TO_DEG;
        if (rz < 0.0f) rz += 360.0f;

        float pitch = std::atan2(dz, dist2D) * RAD_TO_DEG;

        // In RenderWare Euler angles (R = Rz * Ry * Rx), Ry tilts the longitudinal (X) axis
        // into pitch elevation along the 2D heading vector, while Rx is roll.
        float rx = 0.0f;
        float ry = -pitch;

        const float centerZ = ((z1 + z2) * 0.5f) + zOffset;

        cell* pCenterZ = nullptr;
        cell* pRX = nullptr;
        cell* pRY = nullptr;
        cell* pRZ = nullptr;
        if (amx_GetAddr(amx, params[6], &pCenterZ) == AMX_ERR_NONE && pCenterZ &&
            amx_GetAddr(amx, params[7], &pRX) == AMX_ERR_NONE && pRX &&
            amx_GetAddr(amx, params[8], &pRY) == AMX_ERR_NONE && pRY &&
            amx_GetAddr(amx, params[9], &pRZ) == AMX_ERR_NONE && pRZ)
        {
            *pCenterZ = FloatToCell(centerZ);
            *pRX = FloatToCell(rx);
            *pRY = FloatToCell(ry);
            *pRZ = FloatToCell(rz);
            return 1;
        }

        return 0;
    }

    // native GetGroundLinePoints(Float:x1, Float:y1, Float:x2, Float:y2, Float:spacing, Float:z_offset, Float:out_x[], Float:out_y[], Float:out_z[], Float:out_rx[], Float:out_ry[], Float:out_rz[], max_points = sizeof(out_x));
    cell AMX_NATIVE_CALL GetGroundLinePoints(AMX* amx, cell* params)
    {
        if (params[0] < (13 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x1 = amx_ctof(params[1]);
        const float y1 = amx_ctof(params[2]);
        const float x2 = amx_ctof(params[3]);
        const float y2 = amx_ctof(params[4]);
        const float spacing = amx_ctof(params[5]);
        const float zOffset = amx_ctof(params[6]);
        const cell maxPoints = params[13];

        if (maxPoints <= 0 || maxPoints > 1000000 || spacing < 0.1f)
        {
            return 0;
        }

        if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2) ||
            !std::isfinite(spacing) || !std::isfinite(zOffset))
        {
            return 0;
        }

        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float totalDist = std::sqrt(dx * dx + dy * dy);
        if (!std::isfinite(totalDist))
        {
            return 0;
        }

        // T5, N7: Bound totalDist / spacing before int cast
        const float n = std::min(totalDist / spacing, static_cast<float>(maxPoints - 1));
        int numPoints = static_cast<int>(n) + 1;
        if (numPoints <= 0)
        {
            return 0;
        }

        cell* pOutX = nullptr; cell* pOutY = nullptr; cell* pOutZ = nullptr;
        cell* pOutRX = nullptr; cell* pOutRY = nullptr; cell* pOutRZ = nullptr;

        if (!AmxHelper::GetArray(amx, params[7], numPoints, pOutX) ||
            !AmxHelper::GetArray(amx, params[8], numPoints, pOutY) ||
            !AmxHelper::GetArray(amx, params[9], numPoints, pOutZ) ||
            !AmxHelper::GetArray(amx, params[10], numPoints, pOutRX) ||
            !AmxHelper::GetArray(amx, params[11], numPoints, pOutRY) ||
            !AmxHelper::GetArray(amx, params[12], numPoints, pOutRZ))
        {
            return 0;
        }

        float yaw = std::atan2(dy, dx) * RAD_TO_DEG;
        if (yaw < 0.0f) yaw += 360.0f;

        for (int i = 0; i < numPoints; ++i)
        {
            const float t = (numPoints > 1) ? (static_cast<float>(i) / static_cast<float>(numPoints - 1)) : 0.0f;
            const float curX = x1 + dx * t;
            const float curY = y1 + dy * t;
            float groundZ = 0.0f;
            HeightMap::Instance().GetZ(curX, curY, groundZ, true, true);

            float rx = 0.0f, ry = 0.0f;
            HeightMap::Instance().GetObjectRotation(curX, curY, yaw, rx, ry);

            pOutX[i] = FloatToCell(curX);
            pOutY[i] = FloatToCell(curY);
            pOutZ[i] = FloatToCell(groundZ + zOffset);
            pOutRX[i] = FloatToCell(rx);
            pOutRY[i] = FloatToCell(ry);
            pOutRZ[i] = FloatToCell(yaw);
        }

        return static_cast<cell>(numPoints);
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
        if (!std::isfinite(x) || !std::isfinite(y))
        {
            return 0;
        }

        float waterDepth = 0.0f;
        const bool underwater = HeightMap::Instance().IsUnderwater(x, y, waterDepth);

        if (params[0] >= 3 * sizeof(cell))
        {
            cell* pDepth = nullptr;
            if (amx_GetAddr(amx, params[3], &pDepth) == AMX_ERR_NONE && pDepth != nullptr)
            {
                *pDepth = FloatToCell(waterDepth);
            }
        }

        return underwater ? 1 : 0;
    }

    // native bool:GetGroundClearance(Float:x, Float:y, Float:z, &Float:clearance, &bool:is_underground);
    cell AMX_NATIVE_CALL GetGroundClearance(AMX* amx, cell* params)
    {
        if (params[0] < (5 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float z = amx_ctof(params[3]);
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        {
            return 0;
        }

        float clearance = 0.0f;
        bool isUnderground = false;
        const bool success = HeightMap::Instance().GetClearance(x, y, z, clearance, isUnderground);

        cell* pClearance = nullptr;
        cell* pUnderground = nullptr;
        if (amx_GetAddr(amx, params[4], &pClearance) == AMX_ERR_NONE && pClearance &&
            amx_GetAddr(amx, params[5], &pUnderground) == AMX_ERR_NONE && pUnderground)
        {
            *pClearance = FloatToCell(clearance);
            *pUnderground = isUnderground ? 1 : 0;
        }

        return success ? 1 : 0;
    }

    // native bool:IsPointUnderGround(Float:x, Float:y, Float:z, Float:tolerance = 0.2);
    cell AMX_NATIVE_CALL IsPointUnderGround(AMX* amx, cell* params)
    {
        if (params[0] < (3 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float z = amx_ctof(params[3]);
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        {
            return 0;
        }

        if (WaterSystem::IsPointInWater(x, y, z, AmxHelper::GetServerWeather(amx, -1)))
        {
            return 0; // Points submerged in water are in a water volume, not underground
        }

        const float tolerance = (params[0] >= 4 * sizeof(cell)) ? amx_ctof(params[4]) : 0.2f;

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(x, y, groundZ, true, true))
        {
            return 0;
        }

        return (z < (groundZ - tolerance)) ? 1 : 0;
    }

    static bool VehicleUnderGround(AMX* amx, int vehicleId, bool ignoreInteriors)
    {
        if (ignoreInteriors && AmxHelper::GetVehicleInterior(amx, vehicleId) != 0)
        {
            return false;
        }

        float x = 0.0f, y = 0.0f, z = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, x, y, z))
        {
            return false;
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        float sizeZ = 0.8f;
        if (modelId >= 400 && modelId <= 611)
        {
            sizeZ = GetVehicleModelSizeZ(modelId);
        }

        if (WaterSystem::IsVehicleInWater(x, y, z, sizeZ, AmxHelper::GetServerWeather(amx, -1)))
        {
            return false; // Water contact is not underground
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(x, y, groundZ, true, true))
        {
            return false;
        }

        return (z < (groundZ - 0.5f));
    }

    static bool VehicleOnGround(AMX* amx, int vehicleId, float maxClearance, bool checkUpsideDown, bool allowWater, bool ignoreInteriors)
    {
        if (ignoreInteriors && AmxHelper::GetVehicleInterior(amx, vehicleId) != 0)
        {
            return false;
        }

        float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, vx, vy, vz))
        {
            return false;
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        float sizeZ = 0.8f;
        float wheelOffset = 0.5f;

        if (modelId >= 400 && modelId <= 611)
        {
            sizeZ = GetVehicleModelSizeZ(modelId);
            wheelOffset = GetVehicleBaseGroundOffset(modelId);
        }

        if (!allowWater && WaterSystem::IsVehicleInWater(vx, vy, vz, sizeZ, AmxHelper::GetServerWeather(amx, -1)))
        {
            return false;
        }

        if (checkUpsideDown && AmxHelper::IsVehicleUpsideDown(amx, vehicleId))
        {
            return false;
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(vx, vy, groundZ, true, true))
        {
            return false;
        }

        const float tireContactZ = vz - wheelOffset;
        const float clearance = std::abs(tireContactZ - groundZ);

        return (clearance <= maxClearance);
    }

    static float VehicleGroundDistance(AMX* amx, int vehicleId, bool ignoreInteriors)
    {
        if (ignoreInteriors && AmxHelper::GetVehicleInterior(amx, vehicleId) != 0)
        {
            return 0.0f;
        }

        float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, vx, vy, vz))
        {
            return 0.0f;
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        float wheelOffset = 0.5f;

        if (modelId >= 400 && modelId <= 611)
        {
            wheelOffset = GetVehicleBaseGroundOffset(modelId);
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(vx, vy, groundZ, true, true))
        {
            return 0.0f;
        }

        return (vz - wheelOffset) - groundZ;
    }

    // native bool:IsPlayerUnderGround(playerid, bool:ignore_interiors = true);
    // Interior-aware
    cell AMX_NATIVE_CALL IsPlayerUnderGround(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int playerId = static_cast<int>(params[1]);
        const bool ignoreInteriors = (params[0] >= 2 * sizeof(cell)) ? (params[2] != 0) : true;

        if (ignoreInteriors && AmxHelper::GetPlayerInterior(amx, playerId) != 0)
        {
            return 0; // Interior volumes are not represented in 2.5D heightmap
        }

        const int vid = AmxHelper::GetPlayerVehicleId(amx, playerId);
        if (vid > 0)
        {
            return VehicleUnderGround(amx, vid, ignoreInteriors) ? 1 : 0;
        }

        float x = 0.0f, y = 0.0f, z = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, x, y, z))
        {
            return 0;
        }

        if (WaterSystem::IsPlayerInWater(x, y, z, AmxHelper::GetServerWeather(amx, playerId)))
        {
            return 0; // The ocean has no seabed in the heightmap; swimming/diving is not "underground"
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(x, y, groundZ, true, true))
        {
            return 0;
        }

        const float feetZ = z - kPedRootHeight;
        return (feetZ < (groundZ - 0.5f)) ? 1 : 0;
    }

    // native bool:IsVehicleUnderGround(vehicleid, bool:ignore_interiors = true);
    // Interior-aware
    cell AMX_NATIVE_CALL IsVehicleUnderGround(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int vehicleId = static_cast<int>(params[1]);
        const bool ignoreInteriors = (params[0] >= 2 * sizeof(cell)) ? (params[2] != 0) : true;

        return VehicleUnderGround(amx, vehicleId, ignoreInteriors) ? 1 : 0;
    }

    // native bool:IsPlayerOnGround(playerid, Float:max_clearance = 0.35, bool:check_slope = true, bool:allow_water = false, bool:ignore_interiors = true);
    // Interior-aware
    cell AMX_NATIVE_CALL IsPlayerOnGround(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int playerId = static_cast<int>(params[1]);
        const float maxClearance = (params[0] >= 2 * sizeof(cell)) ? amx_ctof(params[2]) : 0.35f;
        const bool checkSlope = (params[0] >= 3 * sizeof(cell)) ? (params[3] != 0) : true;
        const bool allowWater = (params[0] >= 4 * sizeof(cell)) ? (params[4] != 0) : false;
        const bool ignoreInteriors = (params[0] >= 5 * sizeof(cell)) ? (params[5] != 0) : true;

        if (ignoreInteriors && AmxHelper::GetPlayerInterior(amx, playerId) != 0)
        {
            return 0;
        }

        const int vid = AmxHelper::GetPlayerVehicleId(amx, playerId);
        if (vid > 0)
        {
            return VehicleOnGround(amx, vid, maxClearance, false, allowWater, ignoreInteriors) ? 1 : 0;
        }

        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, px, py, pz))
        {
            return 0;
        }

        if (!allowWater && WaterSystem::IsPlayerInWater(px, py, pz, AmxHelper::GetServerWeather(amx, playerId)))
        {
            return 0;
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(px, py, groundZ, true, true))
        {
            return 0;
        }

        const float feetZ = pz - kPedRootHeight;
        const float clearance = std::abs(feetZ - groundZ);
        if (clearance > maxClearance)
        {
            return 0;
        }

        if (checkSlope)
        {
            float nx = 0.0f, ny = 0.0f, nz = 0.0f, slope = 0.0f;
            if (HeightMap::Instance().GetNormal(px, py, nx, ny, nz, slope))
            {
                if (slope > 60.0f)
                {
                    return 0;
                }
            }
        }

        return 1;
    }

    // native bool:IsVehicleOnGround(vehicleid, Float:max_clearance = 0.35, bool:check_upsidedown = true, bool:allow_water = false, bool:ignore_interiors = true);
    // Interior-aware
    cell AMX_NATIVE_CALL IsVehicleOnGround(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int vehicleId = static_cast<int>(params[1]);
        const float maxClearance = (params[0] >= 2 * sizeof(cell)) ? amx_ctof(params[2]) : 0.35f;
        const bool checkUpsideDown = (params[0] >= 3 * sizeof(cell)) ? (params[3] != 0) : true;
        const bool allowWater = (params[0] >= 4 * sizeof(cell)) ? (params[4] != 0) : false;
        const bool ignoreInteriors = (params[0] >= 5 * sizeof(cell)) ? (params[5] != 0) : true;

        return VehicleOnGround(amx, vehicleId, maxClearance, checkUpsideDown, allowWater, ignoreInteriors) ? 1 : 0;
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
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        {
            return FloatToCell(0.0f);
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(x, y, groundZ, true, true))
        {
            return FloatToCell(0.0f);
        }

        return FloatToCell(z - groundZ);
    }

    // native Float:GetPlayerGroundDistance(playerid, bool:ignore_interiors = true);
    // Interior-aware
    cell AMX_NATIVE_CALL GetPlayerGroundDistance(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return FloatToCell(0.0f);
        }

        const int playerId = static_cast<int>(params[1]);
        const bool ignoreInteriors = (params[0] >= 2 * sizeof(cell)) ? (params[2] != 0) : true;

        if (ignoreInteriors && AmxHelper::GetPlayerInterior(amx, playerId) != 0)
        {
            return FloatToCell(0.0f);
        }

        const int vid = AmxHelper::GetPlayerVehicleId(amx, playerId);
        if (vid > 0)
        {
            return FloatToCell(VehicleGroundDistance(amx, vid, ignoreInteriors));
        }

        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, px, py, pz))
        {
            return FloatToCell(0.0f);
        }

        float groundZ = 0.0f;
        if (!HeightMap::Instance().GetZ(px, py, groundZ, true, true))
        {
            return FloatToCell(0.0f);
        }

        const float feetZ = pz - kPedRootHeight;
        return FloatToCell(feetZ - groundZ);
    }

    // native Float:GetVehicleGroundDistance(vehicleid, bool:ignore_interiors = true);
    // Interior-aware
    cell AMX_NATIVE_CALL GetVehicleGroundDistance(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return FloatToCell(0.0f);
        }

        const int vehicleId = static_cast<int>(params[1]);
        const bool ignoreInteriors = (params[0] >= 2 * sizeof(cell)) ? (params[2] != 0) : true;

        return FloatToCell(VehicleGroundDistance(amx, vehicleId, ignoreInteriors));
    }

    // native bool:IsPointInWater(Float:x, Float:y, Float:z);
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
        int weatherId = (params[0] >= 4 * sizeof(cell)) ? static_cast<int>(params[4]) : -1;
        if (weatherId < 0)
        {
            weatherId = AmxHelper::GetServerWeather(amx, -1);
        }

        return WaterSystem::IsPointInWater(x, y, z, weatherId) ? 1 : 0;
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
        int weatherId = (params[0] >= 4 * sizeof(cell)) ? static_cast<int>(params[4]) : -1;
        if (weatherId < 0)
        {
            weatherId = AmxHelper::GetServerWeather(amx, -1);
        }

        return WaterSystem::IsPointUnderWater(x, y, z, weatherId) ? 1 : 0;
    }

    // native bool:IsPlayerInWater(playerid, weatherid = -1);
    cell AMX_NATIVE_CALL IsPlayerInWater(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int playerId = static_cast<int>(params[1]);
        int weatherId = (params[0] >= 2 * sizeof(cell)) ? static_cast<int>(params[2]) : -1;
        if (weatherId < 0)
        {
            weatherId = AmxHelper::GetServerWeather(amx, playerId);
        }

        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, px, py, pz))
        {
            return 0;
        }

        return WaterSystem::IsPlayerInWater(px, py, pz, weatherId) ? 1 : 0;
    }

    // native bool:IsPlayerUnderWater(playerid, weatherid = -1);
    cell AMX_NATIVE_CALL IsPlayerUnderWater(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int playerId = static_cast<int>(params[1]);
        int weatherId = (params[0] >= 2 * sizeof(cell)) ? static_cast<int>(params[2]) : -1;
        if (weatherId < 0)
        {
            weatherId = AmxHelper::GetServerWeather(amx, playerId);
        }

        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (!AmxHelper::GetPlayerPos(amx, playerId, px, py, pz))
        {
            return 0;
        }

        return WaterSystem::IsPlayerUnderWater(px, py, pz, weatherId) ? 1 : 0;
    }

    // native bool:IsVehicleInWater(vehicleid, weatherid = -1);
    cell AMX_NATIVE_CALL IsVehicleInWater(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int vehicleId = static_cast<int>(params[1]);
        int weatherId = (params[0] >= 2 * sizeof(cell)) ? static_cast<int>(params[2]) : -1;
        if (weatherId < 0)
        {
            weatherId = AmxHelper::GetServerWeather(amx, -1);
        }

        float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, vx, vy, vz))
        {
            return 0;
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        float sizeZ = 0.8f;
        if (modelId >= 400 && modelId <= 611)
        {
            sizeZ = GetVehicleModelSizeZ(modelId);
        }

        return WaterSystem::IsVehicleInWater(vx, vy, vz, sizeZ, weatherId) ? 1 : 0;
    }

    // native bool:IsVehicleUnderWater(vehicleid, weatherid = -1);
    cell AMX_NATIVE_CALL IsVehicleUnderWater(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        const int vehicleId = static_cast<int>(params[1]);
        int weatherId = (params[0] >= 2 * sizeof(cell)) ? static_cast<int>(params[2]) : -1;
        if (weatherId < 0)
        {
            weatherId = AmxHelper::GetServerWeather(amx, -1);
        }

        float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        if (!AmxHelper::GetVehiclePos(amx, vehicleId, vx, vy, vz))
        {
            return 0;
        }

        const int modelId = AmxHelper::GetVehicleModel(amx, vehicleId);
        float sizeZ = 0.8f;
        if (modelId >= 400 && modelId <= 611)
        {
            sizeZ = GetVehicleModelSizeZ(modelId);
        }

        return WaterSystem::IsVehicleUnderWater(vx, vy, vz, sizeZ, weatherId) ? 1 : 0;
    }

    // native bool:GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);
    cell AMX_NATIVE_CALL GetWaterLevel(AMX* amx, cell* params)
    {
        if (params[0] < (3 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        int weatherId = (params[0] >= 4 * sizeof(cell)) ? static_cast<int>(params[4]) : -1;
        if (weatherId < 0)
        {
            weatherId = AmxHelper::GetServerWeather(amx, -1);
        }

        float surfaceZ = 0.0f, bottomZ = 0.0f;
        const bool found = WaterSystem::GetWaterLevelAt(x, y, surfaceZ, bottomZ, weatherId);

        cell* pZ = nullptr;
        if (amx_GetAddr(amx, params[3], &pZ) == AMX_ERR_NONE && pZ != nullptr)
        {
            *pZ = FloatToCell(surfaceZ);
        }

        return found ? 1 : 0;
    }

    // native bool:GroundAndreas_SetVehicleInterior(vehicleid, interiorid);
    cell AMX_NATIVE_CALL GroundAndreas_SetVehicleInterior(AMX* amx, cell* params)
    {
        if (params[0] < (2 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const int vehicleId = static_cast<int>(params[1]);
        const int interiorId = static_cast<int>(params[2]);
        if (vehicleId >= 1 && vehicleId <= 2000)
        {
            AmxHelper::SetVehicleInterior(vehicleId, interiorId);
            return 1;
        }
        return 0;
    }

    // native bool:GroundAndreasRemoveBuilding(modelid, Float:x, Float:y, Float:z, Float:radius);
    cell AMX_NATIVE_CALL GroundAndreasRemoveBuilding(AMX* amx, cell* params)
    {
        if (params[0] < (5 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const int modelId = static_cast<int>(params[1]);
        const float x = amx_ctof(params[2]);
        const float y = amx_ctof(params[3]);
        const float z = amx_ctof(params[4]);
        const float radius = amx_ctof(params[5]);

        return HeightMap::Instance().RemoveBuilding(modelId, x, y, z, radius) ? 1 : 0;
    }

    // native bool:ApplyObjectToGroundAndreas(modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, bool:force_replace = false, Float:custom_size_x = 0.0, Float:custom_size_y = 0.0, Float:custom_size_z = 0.0);
    cell AMX_NATIVE_CALL ApplyObjectToGroundAndreas(AMX* amx, cell* params)
    {
        if (params[0] < (7 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const int modelId = static_cast<int>(params[1]);
        const float x = amx_ctof(params[2]);
        const float y = amx_ctof(params[3]);
        const float z = amx_ctof(params[4]);
        const float rx = amx_ctof(params[5]);
        const float ry = amx_ctof(params[6]);
        const float rz = amx_ctof(params[7]);

        const bool forceReplace = (params[0] >= 8 * sizeof(cell)) ? (params[8] != 0) : false;
        const float customSizeX = (params[0] >= 9 * sizeof(cell)) ? amx_ctof(params[9]) : 0.0f;
        const float customSizeY = (params[0] >= 10 * sizeof(cell)) ? amx_ctof(params[10]) : 0.0f;
        const float customSizeZ = (params[0] >= 11 * sizeof(cell)) ? amx_ctof(params[11]) : 0.0f;

        return HeightMap::Instance().ApplyObject(modelId, x, y, z, rx, ry, rz, forceReplace, customSizeX, customSizeY, customSizeZ) ? 1 : 0;
    }

    // native bool:SaveGroundAndreasPatches(const filename[]);
    // T12 (M8): Path restricted to scriptfiles
    cell AMX_NATIVE_CALL SaveGroundAndreasPatches(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        std::string rawPath;
        if (!AmxHelper::GetString(amx, params[1], rawPath) || rawPath.empty())
        {
            return 0;
        }

        std::string safePath = AmxHelper::SanitizePatchPath(rawPath);
        if (safePath.empty())
        {
            return 0;
        }

        return HeightMap::Instance().SavePatches(safePath) ? 1 : 0;
    }

    // native bool:LoadGroundAndreasPatches(const filename[]);
    // T12 (M8): Path restricted to scriptfiles
    cell AMX_NATIVE_CALL LoadGroundAndreasPatches(AMX* amx, cell* params)
    {
        if (params[0] < static_cast<cell>(sizeof(cell)))
        {
            return 0;
        }

        std::string rawPath;
        if (!AmxHelper::GetString(amx, params[1], rawPath) || rawPath.empty())
        {
            return 0;
        }

        std::string safePath = AmxHelper::SanitizePatchPath(rawPath);
        if (safePath.empty())
        {
            return 0;
        }

        return HeightMap::Instance().LoadPatches(safePath) ? 1 : 0;
    }

    // native bool:IsGroundSafeForSpawn(Float:x, Float:y, Float:max_slope = 15.0);
    cell AMX_NATIVE_CALL IsGroundSafeForSpawn(AMX* amx, cell* params)
    {
        if (params[0] < (2 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float maxSlope = (params[0] >= 3 * sizeof(cell)) ? amx_ctof(params[3]) : 15.0f;

        return HeightMap::Instance().IsSafeForSpawn(x, y, maxSlope) ? 1 : 0;
    }

    // native bool:GetSafeGroundSpawn(Float:search_x, Float:search_y, &Float:safe_x, &Float:safe_y, &Float:safe_z, Float:max_slope = 15.0, Float:search_radius = 50.0);
    cell AMX_NATIVE_CALL GetSafeGroundSpawn(AMX* amx, cell* params)
    {
        if (params[0] < (5 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float searchX = amx_ctof(params[1]);
        const float searchY = amx_ctof(params[2]);
        const float maxSlope = (params[0] >= 6 * sizeof(cell)) ? amx_ctof(params[6]) : 15.0f;
        const float searchRadius = (params[0] >= 7 * sizeof(cell)) ? amx_ctof(params[7]) : 50.0f;

        float safeX = 0.0f, safeY = 0.0f, safeZ = 0.0f;
        const bool found = HeightMap::Instance().GetSafeSpawn(searchX, searchY, safeX, safeY, safeZ, maxSlope, searchRadius);

        cell* pX = nullptr;
        cell* pY = nullptr;
        cell* pZ = nullptr;

        if (amx_GetAddr(amx, params[3], &pX) == AMX_ERR_NONE && pX &&
            amx_GetAddr(amx, params[4], &pY) == AMX_ERR_NONE && pY &&
            amx_GetAddr(amx, params[5], &pZ) == AMX_ERR_NONE && pZ)
        {
            *pX = FloatToCell(safeX);
            *pY = FloatToCell(safeY);
            *pZ = FloatToCell(safeZ);
        }

        return found ? 1 : 0;
    }

    // native bool:GetGroundImpactTime(Float:x, Float:y, Float:z, Float:vz, &Float:seconds_to_impact, &Float:impact_z, Float:gravity = 9.8);
    cell AMX_NATIVE_CALL GetGroundImpactTime(AMX* amx, cell* params)
    {
        if (params[0] < (6 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float x = amx_ctof(params[1]);
        const float y = amx_ctof(params[2]);
        const float z = amx_ctof(params[3]);
        const float vz = amx_ctof(params[4]);
        const float gravity = (params[0] >= 7 * sizeof(cell)) ? amx_ctof(params[7]) : 9.8f;

        float secondsToImpact = 0.0f, impactZ = 0.0f;
        const bool success = HeightMap::Instance().GetImpactTime(x, y, z, vz, secondsToImpact, impactZ, gravity);

        cell* pSec = nullptr;
        cell* pImpZ = nullptr;

        if (amx_GetAddr(amx, params[5], &pSec) == AMX_ERR_NONE && pSec &&
            amx_GetAddr(amx, params[6], &pImpZ) == AMX_ERR_NONE && pImpZ)
        {
            *pSec = FloatToCell(secondsToImpact);
            *pImpZ = FloatToCell(impactZ);
        }

        return success ? 1 : 0;
    }

    // native bool:IsPointInRadarShadow(Float:radar_x, Float:radar_y, Float:radar_z, Float:target_x, Float:target_y, Float:target_z);
    cell AMX_NATIVE_CALL IsPointInRadarShadow(AMX* amx, cell* params)
    {
        if (params[0] < (6 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float rx = amx_ctof(params[1]);
        const float ry = amx_ctof(params[2]);
        const float rz = amx_ctof(params[3]);
        const float tx = amx_ctof(params[4]);
        const float ty = amx_ctof(params[5]);
        const float tz = amx_ctof(params[6]);

        return HeightMap::Instance().IsPointInRadarShadow(rx, ry, rz, tx, ty, tz) ? 1 : 0;
    }

    // native bool:GetAreaElevationStats(Float:minx, Float:miny, Float:maxx, Float:maxy, &Float:min_z, &Float:max_z, &Float:avg_z, &Float:max_slope_deg);
    cell AMX_NATIVE_CALL GetAreaElevationStats(AMX* amx, cell* params)
    {
        if (params[0] < (8 * static_cast<cell>(sizeof(cell))))
        {
            return 0;
        }

        const float minX = amx_ctof(params[1]);
        const float minY = amx_ctof(params[2]);
        const float maxX = amx_ctof(params[3]);
        const float maxY = amx_ctof(params[4]);

        float minZ = 0.0f, maxZ = 0.0f, avgZ = 0.0f, maxSlope = 0.0f;
        const bool result = HeightMap::Instance().GetAreaElevationStats(minX, minY, maxX, maxY, minZ, maxZ, avgZ, maxSlope);

        cell* addrMinZ = nullptr;
        cell* addrMaxZ = nullptr;
        cell* addrAvgZ = nullptr;
        cell* addrMaxSlope = nullptr;

        if (amx_GetAddr(amx, params[5], &addrMinZ) == AMX_ERR_NONE && addrMinZ &&
            amx_GetAddr(amx, params[6], &addrMaxZ) == AMX_ERR_NONE && addrMaxZ &&
            amx_GetAddr(amx, params[7], &addrAvgZ) == AMX_ERR_NONE && addrAvgZ &&
            amx_GetAddr(amx, params[8], &addrMaxSlope) == AMX_ERR_NONE && addrMaxSlope)
        {
            *addrMinZ = FloatToCell(minZ);
            *addrMaxZ = FloatToCell(maxZ);
            *addrAvgZ = FloatToCell(avgZ);
            *addrMaxSlope = FloatToCell(maxSlope);
        }

        return result ? 1 : 0;
    }
}

// T3 (C3): Exception barrier template wrapper for all AMX natives
template <cell (AMX_NATIVE_CALL *Func)(AMX*, cell*)>
static cell AMX_NATIVE_CALL SafeNative(AMX* amx, cell* params)
{
    try
    {
        return Func(amx, params);
    }
    catch (const std::exception& e)
    {
        if (logprintf != nullptr)
        {
            logprintf("[GroundAndreas] Exception in native call: %s", e.what());
        }
        return 0;
    }
    catch (...)
    {
        if (logprintf != nullptr)
        {
            logprintf("[GroundAndreas] Unknown exception in native call.");
        }
        return 0;
    }
}

AMX_NATIVE_INFO PluginNatives[] =
{
    // 1. Engine Lifecycle
    { "GroundAndreas_Init",               SafeNative<Natives::GroundAndreas_Init> },
    { "GroundAndreas_Unload",             SafeNative<Natives::GroundAndreas_Unload> },
    { "GroundAndreas_IsLoaded",           SafeNative<Natives::GroundAndreas_IsLoaded> },

    // 2. Core Elevation Queries
    { "GetGroundZPos",                    SafeNative<Natives::GetGroundZPos> },
    { "GetGroundZ",                       SafeNative<Natives::GetGroundZPos> },      // Binary compatibility alias (macro in GroundAndreas.inc)
    { "GetGroundZPosBatch",               SafeNative<Natives::GetGroundZPosBatch> },
    { "GetGroundZBatch",                  SafeNative<Natives::GetGroundZPosBatch> }, // Binary compatibility alias (macro in GroundAndreas.inc)
    { "GetGroundZPosArea",                SafeNative<Natives::GetGroundZPosArea> },
    { "GetGroundZPosRadius",              SafeNative<Natives::GetGroundZPosRadius> },

    // 3. Raycasting, Normals & Surface Alignment
    { "IsWorldBlockingLine",              SafeNative<Natives::IsWorldBlockingLine> },
    { "GetGroundNormal",                  SafeNative<Natives::GetGroundNormal> },
    { "GetGroundObjectRotation",          SafeNative<Natives::GetGroundObjectRotation> },
    { "GetGroundObjectPose",              SafeNative<Natives::GetGroundObjectPose> },
    { "GetGroundTwoPointRotation",        SafeNative<Natives::GetGroundTwoPointRotation> },
    { "GetGroundLinePoints",              SafeNative<Natives::GetGroundLinePoints> },
    { "IsGroundUnderwater",               SafeNative<Natives::IsGroundUnderwater> },
    { "GetGroundClearance",               SafeNative<Natives::GetGroundClearance> },

    // 4. Underground & Proximity Detection
    { "IsPointUnderGround",               SafeNative<Natives::IsPointUnderGround> },
    { "IsPlayerUnderGround",              SafeNative<Natives::IsPlayerUnderGround> },
    { "IsVehicleUnderGround",             SafeNative<Natives::IsVehicleUnderGround> },
    { "IsPlayerOnGround",                 SafeNative<Natives::IsPlayerOnGround> },
    { "IsVehicleOnGround",                SafeNative<Natives::IsVehicleOnGround> },
    { "GetPointGroundDistance",           SafeNative<Natives::GetPointGroundDistance> },
    { "GetPlayerGroundDistance",          SafeNative<Natives::GetPlayerGroundDistance> },
    { "GetVehicleGroundDistance",         SafeNative<Natives::GetVehicleGroundDistance> },
    { "GroundAndreas_SetVehicleInterior", SafeNative<Natives::GroundAndreas_SetVehicleInterior> },

    // 5. Water Detection Subsystem
    { "IsPointInWater",                   SafeNative<Natives::IsPointInWater> },
    { "IsPointUnderWater",                SafeNative<Natives::IsPointUnderWater> },
    { "IsPlayerInWater",                  SafeNative<Natives::IsPlayerInWater> },
    { "IsPlayerUnderWater",               SafeNative<Natives::IsPlayerUnderWater> },
    { "IsVehicleInWater",                 SafeNative<Natives::IsVehicleInWater> },
    { "IsVehicleUnderWater",              SafeNative<Natives::IsVehicleUnderWater> },
    { "GetWaterLevel",                    SafeNative<Natives::GetWaterLevel> },

    // 6. Dynamic Terrain & ColEditor
    { "GroundAndreasRemoveBuilding",      SafeNative<Natives::GroundAndreasRemoveBuilding> },
    { "ApplyObjectToGroundAndreas",       SafeNative<Natives::ApplyObjectToGroundAndreas> },
    { "SaveGroundAndreasPatches",         SafeNative<Natives::SaveGroundAndreasPatches> },
    { "LoadGroundAndreasPatches",         SafeNative<Natives::LoadGroundAndreasPatches> },

    // 7. Terrain Analytics, Kinematics & Spawn Safety
    { "IsGroundSafeForSpawn",             SafeNative<Natives::IsGroundSafeForSpawn> },
    { "GetSafeGroundSpawn",               SafeNative<Natives::GetSafeGroundSpawn> },
    { "GetGroundImpactTime",              SafeNative<Natives::GetGroundImpactTime> },
    { "IsPointInRadarShadow",             SafeNative<Natives::IsPointInRadarShadow> },
    { "GetAreaElevationStats",            SafeNative<Natives::GetAreaElevationStats> },

    // =========================================================================
    // Prefixed Aliases (GA_...) - T6 / N9
    // =========================================================================
    { "GA_Init",                          SafeNative<Natives::GroundAndreas_Init> },
    { "GA_Unload",                        SafeNative<Natives::GroundAndreas_Unload> },
    { "GA_IsLoaded",                      SafeNative<Natives::GroundAndreas_IsLoaded> },
    { "GA_GetGroundZPos",                 SafeNative<Natives::GetGroundZPos> },
    { "GA_GetGroundZ",                    SafeNative<Natives::GetGroundZPos> },      // Binary compatibility alias (macro in GroundAndreas.inc)
    { "GA_GetGroundZPosBatch",            SafeNative<Natives::GetGroundZPosBatch> },
    { "GA_GetGroundZBatch",               SafeNative<Natives::GetGroundZPosBatch> }, // Binary compatibility alias (macro in GroundAndreas.inc)
    { "GA_GetGroundZPosArea",             SafeNative<Natives::GetGroundZPosArea> },
    { "GA_GetGroundZPosRadius",           SafeNative<Natives::GetGroundZPosRadius> },
    { "GA_IsWorldBlockingLine",           SafeNative<Natives::IsWorldBlockingLine> },
    { "GA_GetGroundNormal",               SafeNative<Natives::GetGroundNormal> },
    { "GA_GetGroundObjectRotation",       SafeNative<Natives::GetGroundObjectRotation> },
    { "GA_GetGroundObjectPose",           SafeNative<Natives::GetGroundObjectPose> },
    { "GA_GetGroundTwoPointRotation",     SafeNative<Natives::GetGroundTwoPointRotation> },
    { "GA_GetGroundLinePoints",           SafeNative<Natives::GetGroundLinePoints> },
    { "GA_IsGroundUnderwater",            SafeNative<Natives::IsGroundUnderwater> },
    { "GA_GetGroundClearance",            SafeNative<Natives::GetGroundClearance> },
    { "GA_IsPointUnderGround",            SafeNative<Natives::IsPointUnderGround> },
    { "GA_IsPlayerUnderGround",           SafeNative<Natives::IsPlayerUnderGround> },
    { "GA_IsVehicleUnderGround",          SafeNative<Natives::IsVehicleUnderGround> },
    { "GA_IsPlayerOnGround",              SafeNative<Natives::IsPlayerOnGround> },
    { "GA_IsVehicleOnGround",             SafeNative<Natives::IsVehicleOnGround> },
    { "GA_GetPointGroundDistance",        SafeNative<Natives::GetPointGroundDistance> },
    { "GA_GetPlayerGroundDistance",       SafeNative<Natives::GetPlayerGroundDistance> },
    { "GA_GetVehicleGroundDistance",      SafeNative<Natives::GetVehicleGroundDistance> },
    { "GA_SetVehicleInterior",            SafeNative<Natives::GroundAndreas_SetVehicleInterior> },
    { "GA_SetVehInterior",                SafeNative<Natives::GroundAndreas_SetVehicleInterior> },
    { "GA_IsPointInWater",                SafeNative<Natives::IsPointInWater> },
    { "GA_IsPointUnderWater",             SafeNative<Natives::IsPointUnderWater> },
    { "GA_IsPlayerInWater",               SafeNative<Natives::IsPlayerInWater> },
    { "GA_IsPlayerUnderWater",            SafeNative<Natives::IsPlayerUnderWater> },
    { "GA_IsVehicleInWater",              SafeNative<Natives::IsVehicleInWater> },
    { "GA_IsVehicleUnderWater",           SafeNative<Natives::IsVehicleUnderWater> },
    { "GA_GetWaterLevel",                 SafeNative<Natives::GetWaterLevel> },
    { "GA_RemoveBuilding",                SafeNative<Natives::GroundAndreasRemoveBuilding> },
    { "GA_ApplyObject",                   SafeNative<Natives::ApplyObjectToGroundAndreas> },
    { "GA_SavePatches",                   SafeNative<Natives::SaveGroundAndreasPatches> },
    { "GA_LoadPatches",                   SafeNative<Natives::LoadGroundAndreasPatches> },
    { "GA_IsGroundSafeForSpawn",          SafeNative<Natives::IsGroundSafeForSpawn> },
    { "GA_GetSafeGroundSpawn",            SafeNative<Natives::GetSafeGroundSpawn> },
    { "GA_GetGroundImpactTime",           SafeNative<Natives::GetGroundImpactTime> },
    { "GA_IsPointInRadarShadow",          SafeNative<Natives::IsPointInRadarShadow> },
    { "GA_GetAreaElevationStats",         SafeNative<Natives::GetAreaElevationStats> },

    { nullptr, nullptr }
};

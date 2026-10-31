#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

constexpr float MAP_MIN_X = -3000.0f;
constexpr float MAP_MAX_X = 3000.0f;
constexpr float MAP_MIN_Y = -3000.0f;
constexpr float MAP_MAX_Y = 3000.0f;

constexpr int MAP_GRID_WIDTH = 6000;
constexpr int MAP_GRID_HEIGHT = 6000;
constexpr size_t MAP_TOTAL_CELLS = static_cast<size_t>(MAP_GRID_WIDTH) * static_cast<size_t>(MAP_GRID_HEIGHT);
constexpr size_t MAP_FILE_SIZE = MAP_TOTAL_CELLS * sizeof(uint16_t); // 72,000,000 bytes

constexpr float SEA_LEVEL_Z = 0.0f;
constexpr float PI_CONST = 3.14159265358979323846f;
constexpr float RAD_TO_DEG = 180.0f / PI_CONST;
constexpr float DEG_TO_RAD = PI_CONST / 180.0f;

class HeightMap
{
public:
    static HeightMap& Instance();

    bool Load(const std::string& filepath);
    void Unload();
    bool IsLoaded() const { return m_isLoaded; }

    // Core elevation query
    bool GetZ(float x, float y, float& z, bool interpolate = true, bool waterSurface = false) const;

    // Fast raw integer cell lookup
    float ReadHeightRaw(int gx, int gy) const;

    // Normal vector and terrain slope
    bool GetNormal(float x, float y, float& nx, float& ny, float& nz, float& slopeDegrees) const;

    // Pitch & Roll orientation for dynamic objects
    bool GetObjectRotation(float x, float y, float facingAngle, float& rx, float& ry) const;

    // 2.5D Line-of-sight raycasting
    bool RaycastLine(float x1, float y1, float z1, float x2, float y2, float z2, float& hitX, float& hitY, float& hitZ) const;

    // Underwater check and column depth
    bool IsUnderwater(float x, float y, float& waterDepth) const;

    // Clearance and underground check
    bool GetClearance(float x, float y, float z, float& clearance, bool& isUnderground) const;

    // Batch query
    int GetZBatch(const float* xArr, const float* yArr, float* zOut, int totalPoints, bool interpolate) const;

    // Dynamic terrain modification and patch reconstruction
    bool RemoveBuilding(int modelId, float x, float y, float z, float radius);
    bool ApplyObject(int modelId, float x, float y, float z, float rx, float ry, float rz,
                     bool forceReplace = false, float customSizeX = 0.0f, float customSizeY = 0.0f, float customSizeZ = 0.0f);

    // Safe spawn & anti-void rescue
    bool IsSafeForSpawn(float x, float y, float maxSlope = 15.0f) const;
    bool GetSafeSpawn(float searchX, float searchY, float& safeX, float& safeY, float& safeZ,
                      float maxSlope = 15.0f, float searchRadius = 50.0f) const;

    // Aircraft altimeter & impact prediction
    bool GetImpactTime(float x, float y, float z, float vz, float& secondsToImpact, float& impactZ, float gravity = 9.8f) const;

    // Radar shadow & terrain masking
    bool IsPointInRadarShadow(float radarX, float radarY, float radarZ, float targetX, float targetY, float targetZ) const;

    // Topographic area elevation stats
    bool GetAreaElevationStats(float minX, float minY, float maxX, float maxY,
                               float& minZ, float& maxZ, float& avgZ, float& maxSlopeDeg) const;

    // Patch persistence
    bool SavePatches(const std::string& filename) const;
    bool LoadPatches(const std::string& filename);

    const std::string& GetLoadedPath() const { return m_loadedPath; }

private:
    HeightMap();
    ~HeightMap();
    HeightMap(const HeightMap&) = delete;
    HeightMap& operator=(const HeightMap&) = delete;

    bool WorldToGrid(float x, float y, int& gx, int& gy) const;
    void GetBilinearGradients(float x, float y, float& dz_dx, float& dz_dy) const;

    bool m_isLoaded{ false };
    bool m_isMmap{ false };
    uint16_t* m_data{ nullptr };
    std::string m_loadedPath;

    std::unordered_set<uint64_t> m_removedBuildings;
    std::unordered_map<size_t, uint16_t> m_modifiedCells;

    // Windows file mapping handles
#if defined(_WIN32) || defined(WIN32)
    HANDLE m_hFile{ INVALID_HANDLE_VALUE };
    HANDLE m_hMapping{ NULL };
#else
    // Linux POSIX mmap
    int m_fd{ -1 };
#endif

    // Fallback heap buffer
    std::vector<uint16_t> m_fallbackBuffer;
};

#pragma once

#include "common.h"
#include <string>
#include <vector>

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
    const uint16_t* m_data{ nullptr };
    std::string m_loadedPath;

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

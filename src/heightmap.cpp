#include "heightmap.h"
#include <fstream>
#include <algorithm>
#include <cmath>

HeightMap& HeightMap::Instance()
{
    static HeightMap s_instance;
    return s_instance;
}

HeightMap::HeightMap() = default;

HeightMap::~HeightMap()
{
    Unload();
}

bool HeightMap::Load(const std::string& filepath)
{
    Unload();

#if defined(_WIN32) || defined(WIN32)
    m_hFile = CreateFileA(
        filepath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_READONLY | FILE_FLAG_RANDOM_ACCESS,
        NULL
    );

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER size;
        if (GetFileSizeEx(m_hFile, &size) && static_cast<size_t>(size.QuadPart) >= MAP_FILE_SIZE)
        {
            m_hMapping = CreateFileMappingA(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (m_hMapping != NULL)
            {
                m_data = static_cast<const uint16_t*>(MapViewOfFile(m_hMapping, FILE_MAP_READ, 0, 0, 0));
                if (m_data != nullptr)
                {
                    m_isMmap = true;
                    m_isLoaded = true;
                    m_loadedPath = filepath;
                    return true;
                }
                CloseHandle(m_hMapping);
                m_hMapping = NULL;
            }
        }
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
#else
    m_fd = open(filepath.c_str(), O_RDONLY);
    if (m_fd != -1)
    {
        struct stat sb;
        if (fstat(m_fd, &sb) == 0 && static_cast<size_t>(sb.st_size) >= MAP_FILE_SIZE)
        {
            void* mapped = mmap(NULL, MAP_FILE_SIZE, PROT_READ, MAP_SHARED, m_fd, 0);
            if (mapped != MAP_FAILED)
            {
                m_data = static_cast<const uint16_t*>(mapped);
                m_isMmap = true;
                m_isLoaded = true;
                m_loadedPath = filepath;
                return true;
            }
        }
        close(m_fd);
        m_fd = -1;
    }
#endif

    // Fallback: Read file directly into heap buffer if memory mapping is unavailable
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        std::streamsize fileSize = file.tellg();
        if (static_cast<size_t>(fileSize) >= MAP_FILE_SIZE)
        {
            file.seekg(0, std::ios::beg);
            m_fallbackBuffer.resize(MAP_TOTAL_CELLS);
            if (file.read(reinterpret_cast<char*>(m_fallbackBuffer.data()), MAP_FILE_SIZE))
            {
                m_data = m_fallbackBuffer.data();
                m_isMmap = false;
                m_isLoaded = true;
                m_loadedPath = filepath;
                return true;
            }
        }
    }

    return false;
}

void HeightMap::Unload()
{
    if (m_isLoaded)
    {
#if defined(_WIN32) || defined(WIN32)
        if (m_isMmap)
        {
            if (m_data != nullptr)
            {
                UnmapViewOfFile(m_data);
            }
            if (m_hMapping != NULL)
            {
                CloseHandle(m_hMapping);
                m_hMapping = NULL;
            }
            if (m_hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_hFile);
                m_hFile = INVALID_HANDLE_VALUE;
            }
        }
#else
        if (m_isMmap)
        {
            if (m_data != nullptr)
            {
                munmap(const_cast<uint16_t*>(m_data), MAP_FILE_SIZE);
            }
            if (m_fd != -1)
            {
                close(m_fd);
                m_fd = -1;
            }
        }
#endif
        m_fallbackBuffer.clear();
        m_fallbackBuffer.shrink_to_fit();
        m_data = nullptr;
        m_isLoaded = false;
        m_isMmap = false;
        m_loadedPath.clear();
    }
}

bool HeightMap::WorldToGrid(float x, float y, int& gx, int& gy) const
{
    if (x < MAP_MIN_X || x > MAP_MAX_X || y < MAP_MIN_Y || y > MAP_MAX_Y)
    {
        return false;
    }

    // Grid coordinates:
    // (x = -3000.0) -> gx = 0, (x = +3000.0) -> gx = 5999
    // (y = +3000.0) -> gy = 0, (y = -3000.0) -> gy = 5999
    gx = static_cast<int>(std::floor(x + 3000.0f));
    gy = static_cast<int>(std::floor(3000.0f - y));

    if (gx < 0) gx = 0;
    else if (gx >= MAP_GRID_WIDTH) gx = MAP_GRID_WIDTH - 1;

    if (gy < 0) gy = 0;
    else if (gy >= MAP_GRID_HEIGHT) gy = MAP_GRID_HEIGHT - 1;

    return true;
}

float HeightMap::ReadHeightRaw(int gx, int gy) const
{
    if (!m_isLoaded || m_data == nullptr)
    {
        return 0.0f;
    }

    const size_t index = static_cast<size_t>(gy) * MAP_GRID_WIDTH + static_cast<size_t>(gx);
    return static_cast<float>(m_data[index]) * 0.01f;
}

bool HeightMap::GetZ(float x, float y, float& z, bool interpolate, bool waterSurface) const
{
    if (!m_isLoaded || m_data == nullptr)
    {
        z = 0.0f;
        return false;
    }

    if (x < MAP_MIN_X || x > MAP_MAX_X || y < MAP_MIN_Y || y > MAP_MAX_Y)
    {
        z = 0.0f;
        return false;
    }

    if (!interpolate)
    {
        int gx = 0, gy = 0;
        WorldToGrid(x, y, gx, gy);
        z = ReadHeightRaw(gx, gy);
    }
    else
    {
        // Continuous normalized coordinates
        const float fx = x + 3000.0f;
        const float fy = 3000.0f - y;

        int x0 = static_cast<int>(std::floor(fx));
        int y0 = static_cast<int>(std::floor(fy));

        if (x0 < 0) x0 = 0;
        else if (x0 >= MAP_GRID_WIDTH - 1) x0 = MAP_GRID_WIDTH - 2;

        if (y0 < 0) y0 = 0;
        else if (y0 >= MAP_GRID_HEIGHT - 1) y0 = MAP_GRID_HEIGHT - 2;

        const int x1 = x0 + 1;
        const int y1 = y0 + 1;

        const float u = std::clamp(fx - static_cast<float>(x0), 0.0f, 1.0f);
        const float v = std::clamp(fy - static_cast<float>(y0), 0.0f, 1.0f);

        const float z00 = ReadHeightRaw(x0, y0);
        const float z10 = ReadHeightRaw(x1, y0);
        const float z01 = ReadHeightRaw(x0, y1);
        const float z11 = ReadHeightRaw(x1, y1);

        const float zTop = z00 * (1.0f - u) + z10 * u;
        const float zBot = z01 * (1.0f - u) + z11 * u;
        z = zTop * (1.0f - v) + zBot * v;
    }

    if (waterSurface && z < SEA_LEVEL_Z)
    {
        z = SEA_LEVEL_Z;
    }

    return true;
}

void HeightMap::GetBilinearGradients(float x, float y, float& dz_dx, float& dz_dy) const
{
    const float fx = x + 3000.0f;
    const float fy = 3000.0f - y;

    int x0 = static_cast<int>(std::floor(fx));
    int y0 = static_cast<int>(std::floor(fy));

    if (x0 < 0) x0 = 0;
    else if (x0 >= MAP_GRID_WIDTH - 1) x0 = MAP_GRID_WIDTH - 2;

    if (y0 < 0) y0 = 0;
    else if (y0 >= MAP_GRID_HEIGHT - 1) y0 = MAP_GRID_HEIGHT - 2;

    const int x1 = x0 + 1;
    const int y1 = y0 + 1;

    const float u = std::clamp(fx - static_cast<float>(x0), 0.0f, 1.0f);
    const float v = std::clamp(fy - static_cast<float>(y0), 0.0f, 1.0f);

    const float z00 = ReadHeightRaw(x0, y0);
    const float z10 = ReadHeightRaw(x1, y0);
    const float z01 = ReadHeightRaw(x0, y1);
    const float z11 = ReadHeightRaw(x1, y1);

    // Partial derivatives with respect to grid coordinates u and v:
    // dz/du corresponds to dz/dX (since fx = X + 3000)
    dz_dx = (1.0f - v) * (z10 - z00) + v * (z11 - z01);

    // dz/dv corresponds to -dz/dY (since fy = 3000 - Y)
    // dz/dY = -dz/dv
    const float dz_dv = (1.0f - u) * (z01 - z00) + u * (z11 - z10);
    dz_dy = -dz_dv;
}

bool HeightMap::GetNormal(float x, float y, float& nx, float& ny, float& nz, float& slopeDegrees) const
{
    if (!m_isLoaded || m_data == nullptr)
    {
        nx = 0.0f; ny = 0.0f; nz = 1.0f;
        slopeDegrees = 0.0f;
        return false;
    }

    if (x < MAP_MIN_X || x > MAP_MAX_X || y < MAP_MIN_Y || y > MAP_MAX_Y)
    {
        nx = 0.0f; ny = 0.0f; nz = 1.0f;
        slopeDegrees = 0.0f;
        return false;
    }

    float dz_dx = 0.0f;
    float dz_dy = 0.0f;
    GetBilinearGradients(x, y, dz_dx, dz_dy);

    // Surface normal pointing upward: (-dz/dx, -dz/dy, 1)
    const float rawNx = -dz_dx;
    const float rawNy = -dz_dy;
    const float rawNz = 1.0f;

    const float len = std::sqrt(rawNx * rawNx + rawNy * rawNy + rawNz * rawNz);
    if (len > 0.00001f)
    {
        nx = rawNx / len;
        ny = rawNy / len;
        nz = rawNz / len;
    }
    else
    {
        nx = 0.0f;
        ny = 0.0f;
        nz = 1.0f;
    }

    const float clampedNz = std::clamp(nz, 0.0f, 1.0f);
    slopeDegrees = std::acos(clampedNz) * RAD_TO_DEG;
    return true;
}

bool HeightMap::GetObjectRotation(float x, float y, float facingAngle, float& rx, float& ry) const
{
    if (!m_isLoaded || m_data == nullptr)
    {
        rx = 0.0f; ry = 0.0f;
        return false;
    }

    if (x < MAP_MIN_X || x > MAP_MAX_X || y < MAP_MIN_Y || y > MAP_MAX_Y)
    {
        rx = 0.0f; ry = 0.0f;
        return false;
    }

    float dz_dx = 0.0f;
    float dz_dy = 0.0f;
    GetBilinearGradients(x, y, dz_dx, dz_dy);

    // SA-MP Compass Heading: 0 deg = North (+Y), 90 deg = West (-X), 180 deg = South (-Y), 270 deg = East (+X)
    const float headingRad = facingAngle * DEG_TO_RAD;
    const float fwdX = -std::sin(headingRad);
    const float fwdY = std::cos(headingRad);

    // Right vector (perpendicular to forward):
    const float rightX = std::cos(headingRad);
    const float rightY = std::sin(headingRad);

    // Directional slopes along forward (Pitch) and right (Roll) axes:
    const float slopeFwd = fwdX * dz_dx + fwdY * dz_dy;
    const float slopeRight = rightX * dz_dx + rightY * dz_dy;

    rx = std::atan(slopeFwd) * RAD_TO_DEG;
    ry = -std::atan(slopeRight) * RAD_TO_DEG;
    return true;
}

bool HeightMap::RaycastLine(float x1, float y1, float z1, float x2, float y2, float z2, float& hitX, float& hitY, float& hitZ) const
{
    if (!m_isLoaded || m_data == nullptr)
    {
        return false;
    }

    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float dz = z2 - z1;

    const float dist2D = std::sqrt(dx * dx + dy * dy);
    const float dist3D = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (dist3D < 0.001f)
    {
        return false;
    }

    // Step size: check at every ~0.5m along the horizontal projection
    const int numSteps = std::clamp(static_cast<int>(std::ceil(dist2D / 0.5f)), 2, 8000);
    const float stepT = 1.0f / static_cast<float>(numSteps);

    float prevT = 0.0f;

    for (int i = 0; i <= numSteps; ++i)
    {
        const float t = static_cast<float>(i) * stepT;
        const float currX = x1 + t * dx;
        const float currY = y1 + t * dy;
        const float currZ = z1 + t * dz;

        float groundZ = 0.0f;
        if (!GetZ(currX, currY, groundZ, true, false))
        {
            // Point is outside the world map boundary; continue
            prevT = t;
            continue;
        }

        // For the starting point (i == 0), allow small clearance threshold to avoid
        // self-collision if player's feet or camera origin are right at ground level
        if (i == 0)
        {
            if (currZ <= groundZ - 0.2f)
            {
                // Starting already deeply underground
                hitX = currX;
                hitY = currY;
                hitZ = currZ;
                return true;
            }
            prevT = t;
            continue;
        }

        // Check if the ray penetrated beneath the terrain surface
        if (currZ <= groundZ)
        {
            // Binary search refinement between prevT and t for millimeter accuracy
            float lowT = prevT;
            float highT = t;

            for (int refine = 0; refine < 5; ++refine)
            {
                const float midT = (lowT + highT) * 0.5f;
                const float mx = x1 + midT * dx;
                const float my = y1 + midT * dy;
                const float mz = z1 + midT * dz;

                float mgz = 0.0f;
                GetZ(mx, my, mgz, true, false);

                if (mz <= mgz)
                {
                    highT = midT;
                }
                else
                {
                    lowT = midT;
                }
            }

            hitX = x1 + highT * dx;
            hitY = y1 + highT * dy;
            hitZ = z1 + highT * dz;
            return true;
        }

        prevT = t;
    }

    return false;
}

bool HeightMap::IsUnderwater(float x, float y, float& waterDepth) const
{
    float groundZ = 0.0f;
    if (!GetZ(x, y, groundZ, true, false))
    {
        waterDepth = 0.0f;
        return false;
    }

    if (groundZ < SEA_LEVEL_Z)
    {
        waterDepth = SEA_LEVEL_Z - groundZ;
        return true;
    }

    waterDepth = 0.0f;
    return false;
}

bool HeightMap::GetClearance(float x, float y, float z, float& clearance, bool& isUnderground) const
{
    float groundZ = 0.0f;
    if (!GetZ(x, y, groundZ, true, false))
    {
        clearance = 0.0f;
        isUnderground = false;
        return false;
    }

    clearance = z - groundZ;
    isUnderground = (clearance < -0.05f);
    return true;
}

int HeightMap::GetZBatch(const float* xArr, const float* yArr, float* zOut, int totalPoints, bool interpolate) const
{
    if (!m_isLoaded || m_data == nullptr || xArr == nullptr || yArr == nullptr || zOut == nullptr || totalPoints <= 0)
    {
        return 0;
    }

    int processed = 0;
    for (int i = 0; i < totalPoints; ++i)
    {
        float z = 0.0f;
        if (GetZ(xArr[i], yArr[i], z, interpolate, false))
        {
            zOut[i] = z;
            processed++;
        }
        else
        {
            zOut[i] = 0.0f;
        }
    }

    return processed;
}

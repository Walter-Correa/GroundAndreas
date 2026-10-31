#include "heightmap.h"
#include "object_data.h"
#include "water.h"
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
            m_hMapping = CreateFileMappingA(m_hFile, NULL, PAGE_WRITECOPY, 0, 0, NULL);
            if (m_hMapping != NULL)
            {
                m_data = static_cast<uint16_t*>(MapViewOfFile(m_hMapping, FILE_MAP_COPY, 0, 0, 0));
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
            void* mapped = mmap(NULL, MAP_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, m_fd, 0);
            if (mapped != MAP_FAILED)
            {
                m_data = static_cast<uint16_t*>(mapped);
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
        m_removedBuildings.clear();
        m_modifiedCells.clear();
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

bool HeightMap::RemoveBuilding(int modelId, float x, float y, float z, float radius)
{
    if (!m_isLoaded || m_data == nullptr)
    {
        return false;
    }

    // Safety guard: Global map-wide wipe pattern (e.g. RemoveBuildingForPlayer(playerid, model, 0.0, 0.0, 0.0, 6000.0))
    // Global wipes are search queries for props/trees/lamps across the state, not a single massive structure.
    if (std::abs(x) < 1.0f && std::abs(y) < 1.0f && radius > 500.0f)
    {
        return true;
    }

    // Deduplication check: hash of modelId and quantized coordinates (run once per unique building location)
    const int qx = static_cast<int>(std::round(x * 2.0f));
    const int qy = static_cast<int>(std::round(y * 2.0f));
    const int qz = static_cast<int>(std::round(z * 2.0f));

    const uint64_t buildingKey = (static_cast<uint64_t>(modelId & 0xFFFF) << 48) ^
                                 (static_cast<uint64_t>(static_cast<uint32_t>(qx)) << 32) ^
                                 (static_cast<uint64_t>(static_cast<uint32_t>(qy)) << 16) ^
                                 (static_cast<uint64_t>(static_cast<uint32_t>(qz)));

    if (!m_removedBuildings.insert(buildingKey).second)
    {
        return true; // Already processed in this server session
    }

    // Determine the actual physical footprint of the model (decoupled from SA-MP search radius)
    float physicalRadius = GetModelPhysicalRadius(modelId);
    float effectiveRadius = std::clamp(physicalRadius, 4.0f, 60.0f);

    // If the mapper provided a specific local radius between 1.0m and 45.0m, consider it as an upper bound
    if (radius >= 1.0f && radius < effectiveRadius)
    {
        effectiveRadius = radius;
    }

    // Perimeter sampling to fit ground slope (Z = C + A*(x - x0) + B*(y - y0))
    constexpr int NUM_SAMPLES = 16;
    const float sampleRadius = effectiveRadius + 2.0f;

    struct SamplePoint
    {
        float u;
        float v;
        float z;
    };
    std::vector<SamplePoint> samples;
    samples.reserve(NUM_SAMPLES);

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float angle = i * (2.0f * PI_CONST / static_cast<float>(NUM_SAMPLES));
        float u = sampleRadius * std::cos(angle);
        float v = sampleRadius * std::sin(angle);
        float sx = x + u;
        float sy = y + v;
        float sz = 0.0f;

        if (GetZ(sx, sy, sz, true, false))
        {
            samples.push_back({ u, v, sz });
        }
    }

    float planeA = 0.0f;
    float planeB = 0.0f;
    float planeC = z;

    if (!samples.empty())
    {
        // Outlier rejection: In dense industrial sites, adjacent structures might still be elevated.
        // We sort sample heights and retain the lower 60% of samples (the true natural ground).
        std::vector<SamplePoint> sortedSamples = samples;
        std::sort(sortedSamples.begin(), sortedSamples.end(), [](const SamplePoint& a, const SamplePoint& b) {
            return a.z < b.z;
        });

        size_t keepCount = std::max<size_t>(3, (sortedSamples.size() * 6) / 10);
        float sumU2 = 0.0f, sumV2 = 0.0f, sumZ = 0.0f, sumUZ = 0.0f, sumVZ = 0.0f;

        for (size_t i = 0; i < keepCount; ++i)
        {
            const auto& s = sortedSamples[i];
            sumU2 += s.u * s.u;
            sumV2 += s.v * s.v;
            sumZ += s.z;
            sumUZ += s.u * s.z;
            sumVZ += s.v * s.z;
        }

        const float n = static_cast<float>(keepCount);
        planeC = sumZ / n;

        if (sumU2 > 0.001f)
        {
            planeA = sumUZ / sumU2;
        }
        if (sumV2 > 0.001f)
        {
            planeB = sumVZ / sumV2;
        }
    }

    // Bounding grid range for rasterization
    int minGx = 0, maxGx = 0, minGy = 0, maxGy = 0;
    WorldToGrid(x - effectiveRadius, y - effectiveRadius, minGx, minGy);
    WorldToGrid(x + effectiveRadius, y + effectiveRadius, maxGx, maxGy);

    minGx = std::clamp(minGx, 0, MAP_GRID_WIDTH - 1);
    maxGx = std::clamp(maxGx, 0, MAP_GRID_WIDTH - 1);
    minGy = std::clamp(minGy, 0, MAP_GRID_HEIGHT - 1);
    maxGy = std::clamp(maxGy, 0, MAP_GRID_HEIGHT - 1);

    const float radiusSq = effectiveRadius * effectiveRadius;

    for (int gy = minGy; gy <= maxGy; ++gy)
    {
        for (int gx = minGx; gx <= maxGx; ++gx)
        {
            float wx = static_cast<float>(gx - 3000);
            float wy = static_cast<float>(gy - 3000);

            float du = wx - x;
            float dv = wy - y;

            if ((du * du + dv * dv) <= radiusSq)
            {
                float newZ = planeC + planeA * du + planeB * dv;
                size_t cellIndex = static_cast<size_t>(gy) * MAP_GRID_WIDTH + static_cast<size_t>(gx);
                float currentZ = static_cast<float>(m_data[cellIndex]) * 0.01f;

                // Golden Rule: Removal can only LOWER the elevation (carve/flatten), never raise to a roof
                if (newZ < currentZ)
                {
                    uint16_t encodedZ = static_cast<uint16_t>(std::clamp(newZ * 100.0f, 0.0f, 65535.0f));
                    m_data[cellIndex] = encodedZ;
                    m_modifiedCells[cellIndex] = encodedZ;
                }
            }
        }
    }

    return true;
}

bool HeightMap::ApplyObject(int modelId, float x, float y, float z, float rx, float ry, float rz,
                            bool forceReplace, float customSizeX, float customSizeY, float customSizeZ)
{
    if (!m_isLoaded || m_data == nullptr)
    {
        return false;
    }

    float sx = customSizeX, sy = customSizeY, sz = customSizeZ;
    if (sx <= 0.001f || sy <= 0.001f)
    {
        GetCommonObjectModelDimensions(modelId, sx, sy, sz);
    }

    if (sz <= 0.001f)
    {
        sz = 0.5f;
    }

    // Top face corners in local object coordinates
    const float hx = sx * 0.5f;
    const float hy = sy * 0.5f;
    const float hz = sz * 0.5f;

    struct Vec3 { float x, y, z; };
    Vec3 localCorners[4] = {
        { -hx, -hy, hz },
        {  hx, -hy, hz },
        {  hx,  hy, hz },
        { -hx,  hy, hz }
    };

    // Euler angles in radians: GTA SA convention RX (Pitch), RY (Roll), RZ (Yaw)
    const float radX = rx * DEG_TO_RAD;
    const float radY = ry * DEG_TO_RAD;
    const float radZ = rz * DEG_TO_RAD;

    const float cx = std::cos(radX), sx_ = std::sin(radX);
    const float cy = std::cos(radY), sy_ = std::sin(radY);
    const float cz = std::cos(radZ), sz_ = std::sin(radZ);

    // Rotation transform R = Rz * Ry * Rx
    auto rotatePoint = [&](const Vec3& v) -> Vec3 {
        // Rx
        float y1 = v.y * cx - v.z * sx_;
        float z1 = v.y * sx_ + v.z * cx;
        // Ry
        float x2 = v.x * cy + z1 * sy_;
        float z2 = -v.x * sy_ + z1 * cy;
        // Rz
        float x3 = x2 * cz - y1 * sz_;
        float y3 = x2 * sz_ + y1 * cz;
        return { x3 + x, y3 + y, z2 + z };
    };

    Vec3 worldCorners[4];
    float minX = 99999.0f, maxX = -99999.0f, minY = 99999.0f, maxY = -99999.0f;
    for (int i = 0; i < 4; ++i)
    {
        worldCorners[i] = rotatePoint(localCorners[i]);
        minX = (std::min)(minX, worldCorners[i].x);
        maxX = (std::max)(maxX, worldCorners[i].x);
        minY = (std::min)(minY, worldCorners[i].y);
        maxY = (std::max)(maxY, worldCorners[i].y);
    }

    // Calculate face normal: (P1 - P0) x (P3 - P0)
    Vec3 e1 = { worldCorners[1].x - worldCorners[0].x, worldCorners[1].y - worldCorners[0].y, worldCorners[1].z - worldCorners[0].z };
    Vec3 e2 = { worldCorners[3].x - worldCorners[0].x, worldCorners[3].y - worldCorners[0].y, worldCorners[3].z - worldCorners[0].z };

    Vec3 normal = {
        e1.y * e2.z - e1.z * e2.y,
        e1.z * e2.x - e1.x * e2.z,
        e1.x * e2.y - e1.y * e2.x
    };

    float normalLen = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (normalLen < 0.0001f)
    {
        return false;
    }
    normal.x /= normalLen;
    normal.y /= normalLen;
    normal.z /= normalLen;

    // Ensure normal points upward towards +Z
    if (normal.z < 0.0f)
    {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
    }

    if (std::abs(normal.z) < 0.01f)
    {
        return false; // Vertical surface (wall)
    }

    // Plane constant D = -(N . P0)
    float planeD = -(normal.x * worldCorners[0].x + normal.y * worldCorners[0].y + normal.z * worldCorners[0].z);

    // Rasterization grid bounds
    int minGx = 0, maxGx = 0, minGy = 0, maxGy = 0;
    WorldToGrid(minX, minY, minGx, minGy);
    WorldToGrid(maxX, maxY, maxGx, maxGy);

    minGx = std::clamp(minGx, 0, MAP_GRID_WIDTH - 1);
    maxGx = std::clamp(maxGx, 0, MAP_GRID_WIDTH - 1);
    minGy = std::clamp(minGy, 0, MAP_GRID_HEIGHT - 1);
    maxGy = std::clamp(maxGy, 0, MAP_GRID_HEIGHT - 1);

    // Determine polygon 2D winding order
    bool invertedWinding = false;
    {
        float dx = worldCorners[1].x - worldCorners[0].x;
        float dy = worldCorners[1].y - worldCorners[0].y;
        float testCross = dx * (worldCorners[2].y - worldCorners[0].y) - dy * (worldCorners[2].x - worldCorners[0].x);
        if (testCross < 0.0f)
        {
            invertedWinding = true;
        }
    }

    auto isInsideQuad = [&](float px, float py) -> bool {
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            float dx = worldCorners[next].x - worldCorners[i].x;
            float dy = worldCorners[next].y - worldCorners[i].y;
            float cross = dx * (py - worldCorners[i].y) - dy * (px - worldCorners[i].x);
            if (invertedWinding)
            {
                if (cross > 0.001f) return false;
            }
            else
            {
                if (cross < -0.001f) return false;
            }
        }
        return true;
    };

    for (int gy = minGy; gy <= maxGy; ++gy)
    {
        for (int gx = minGx; gx <= maxGx; ++gx)
        {
            float wx = static_cast<float>(gx - 3000);
            float wy = static_cast<float>(gy - 3000);

            if (isInsideQuad(wx, wy))
            {
                // Surface elevation from plane equation: Z = (-D - nx * wx - ny * wy) / nz
                float surfZ = (-planeD - normal.x * wx - normal.y * wy) / normal.z;

                size_t cellIndex = static_cast<size_t>(gy) * MAP_GRID_WIDTH + static_cast<size_t>(gx);
                float currZ = static_cast<float>(m_data[cellIndex]) * 0.01f;

                if (forceReplace || surfZ > currZ)
                {
                    uint16_t encodedZ = static_cast<uint16_t>(std::clamp(surfZ * 100.0f, 0.0f, 65535.0f));
                    m_data[cellIndex] = encodedZ;
                    m_modifiedCells[cellIndex] = encodedZ;
                }
            }
        }
    }

    return true;
}

bool HeightMap::IsSafeForSpawn(float x, float y, float maxSlope) const
{
    if (!m_isLoaded || m_data == nullptr) return false;
    if (x < MAP_MIN_X || x > MAP_MAX_X || y < MAP_MIN_Y || y > MAP_MAX_Y) return false;

    float z = 0.0f;
    if (!GetZ(x, y, z, true, false)) return false;

    // Reject water (ocean, lakes, rivers, reservoirs)
    if (WaterSystem::IsPointInWater(x, y, z + 0.1f)) return false;

    // Check terrain slope
    float nx = 0.0f, ny = 0.0f, nz = 0.0f, slopeDeg = 0.0f;
    if (!GetNormal(x, y, nx, ny, nz, slopeDeg)) return false;

    if (slopeDeg > maxSlope) return false;

    return true;
}

bool HeightMap::GetSafeSpawn(float searchX, float searchY, float& safeX, float& safeY, float& safeZ,
                             float maxSlope, float searchRadius) const
{
    if (!m_isLoaded || m_data == nullptr) return false;

    if (IsSafeForSpawn(searchX, searchY, maxSlope))
    {
        safeX = searchX;
        safeY = searchY;
        GetZ(safeX, safeY, safeZ, true, false);
        return true;
    }

    searchRadius = (std::max)(5.0f, (std::min)(searchRadius, 300.0f));

    // Concentric spiral search outward
    for (float r = 5.0f; r <= searchRadius; r += 5.0f)
    {
        const int steps = (std::max)(8, static_cast<int>(r * 1.5f));
        for (int i = 0; i < steps; ++i)
        {
            float angle = i * (2.0f * PI_CONST / static_cast<float>(steps));
            float tx = searchX + r * std::cos(angle);
            float ty = searchY + r * std::sin(angle);

            if (IsSafeForSpawn(tx, ty, maxSlope))
            {
                safeX = tx;
                safeY = ty;
                GetZ(safeX, safeY, safeZ, true, false);
                return true;
            }
        }
    }

    return false;
}

bool HeightMap::GetImpactTime(float x, float y, float z, float vz, float& secondsToImpact, float& impactZ, float gravity) const
{
    if (!m_isLoaded || m_data == nullptr) return false;

    float groundZ = 0.0f;
    if (!GetZ(x, y, groundZ, true, false)) return false;

    impactZ = groundZ;

    if (z <= groundZ)
    {
        secondsToImpact = 0.0f;
        return true;
    }

    const float heightAboveGround = z - groundZ;

    // Linear motion if gravity is negligible
    if (std::abs(gravity) < 0.001f)
    {
        if (vz < -0.01f)
        {
            secondsToImpact = -heightAboveGround / vz;
            return true;
        }
        secondsToImpact = 99999.0f;
        return false;
    }

    // Ballistic vertical kinematics under gravity: z(t) = z0 + vz*t - 0.5*g*t^2 = groundZ
    const float discriminant = vz * vz + 2.0f * gravity * heightAboveGround;
    if (discriminant < 0.0f)
    {
        secondsToImpact = 99999.0f;
        return false;
    }

    const float sqrtDisc = std::sqrt(discriminant);
    const float t = (vz + sqrtDisc) / gravity;

    if (t < 0.0f)
    {
        secondsToImpact = 0.0f;
        return true;
    }

    secondsToImpact = t;
    return true;
}

bool HeightMap::IsPointInRadarShadow(float radarX, float radarY, float radarZ, float targetX, float targetY, float targetZ) const
{
    if (!m_isLoaded || m_data == nullptr) return false;

    float hitX = 0.0f, hitY = 0.0f, hitZ = 0.0f;
    return RaycastLine(radarX, radarY, radarZ, targetX, targetY, targetZ, hitX, hitY, hitZ);
}

bool HeightMap::GetAreaElevationStats(float minX, float minY, float maxX, float maxY,
                                     float& minZ, float& maxZ, float& avgZ, float& maxSlopeDeg) const
{
    if (!m_isLoaded || m_data == nullptr) return false;

    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);

    minX = (std::max)(MAP_MIN_X, minX);
    maxX = (std::min)(MAP_MAX_X, maxX);
    minY = (std::max)(MAP_MIN_Y, minY);
    maxY = (std::min)(MAP_MAX_Y, maxY);

    int minGx = 0, maxGx = 0, minGy = 0, maxGy = 0;
    if (!WorldToGrid(minX, minY, minGx, minGy) || !WorldToGrid(maxX, maxY, maxGx, maxGy))
    {
        return false;
    }

    int spanX = maxGx - minGx + 1;
    int spanY = maxGy - minGy + 1;
    if (spanX <= 0 || spanY <= 0) return false;

    // Adaptive step to bound queries to at most ~2500 samples
    int stepX = (std::max)(1, spanX / 50);
    int stepY = (std::max)(1, spanY / 50);

    float curMinZ = 99999.0f;
    float curMaxZ = -99999.0f;
    float sumZ = 0.0f;
    float curMaxSlope = 0.0f;
    int sampleCount = 0;

    for (int gy = minGy; gy <= maxGy; gy += stepY)
    {
        for (int gx = minGx; gx <= maxGx; gx += stepX)
        {
            float z = ReadHeightRaw(gx, gy);
            curMinZ = (std::min)(curMinZ, z);
            curMaxZ = (std::max)(curMaxZ, z);
            sumZ += z;

            // Approximate slope at cell
            int leftGx = (std::max)(0, gx - 1);
            int rightGx = (std::min)(MAP_GRID_WIDTH - 1, gx + 1);
            int downGy = (std::max)(0, gy - 1);
            int upGy = (std::min)(MAP_GRID_HEIGHT - 1, gy + 1);

            float dz_dx = (ReadHeightRaw(rightGx, gy) - ReadHeightRaw(leftGx, gy)) * 0.5f;
            float dz_dy = (ReadHeightRaw(gx, upGy) - ReadHeightRaw(gx, downGy)) * 0.5f;
            float slope = std::atan(std::sqrt(dz_dx * dz_dx + dz_dy * dz_dy)) * RAD_TO_DEG;
            curMaxSlope = (std::max)(curMaxSlope, slope);

            sampleCount++;
        }
    }

    if (sampleCount == 0) return false;

    minZ = curMinZ;
    maxZ = curMaxZ;
    avgZ = sumZ / static_cast<float>(sampleCount);
    maxSlopeDeg = curMaxSlope;
    return true;
}

constexpr uint32_t GPATCH_MAGIC = 0x43545047; // 'G','P','T','C'
constexpr uint32_t GPATCH_VERSION = 1;

bool HeightMap::SavePatches(const std::string& filename) const
{
    if (!m_isLoaded || m_data == nullptr) return false;

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) return false;

    out.write(reinterpret_cast<const char*>(&GPATCH_MAGIC), sizeof(GPATCH_MAGIC));
    out.write(reinterpret_cast<const char*>(&GPATCH_VERSION), sizeof(GPATCH_VERSION));

    const uint32_t count = static_cast<uint32_t>(m_modifiedCells.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& kv : m_modifiedCells)
    {
        uint32_t cellIndex = static_cast<uint32_t>(kv.first);
        uint16_t encodedZ = kv.second;
        out.write(reinterpret_cast<const char*>(&cellIndex), sizeof(cellIndex));
        out.write(reinterpret_cast<const char*>(&encodedZ), sizeof(encodedZ));
    }

    return true;
}

bool HeightMap::LoadPatches(const std::string& filename)
{
    if (!m_isLoaded || m_data == nullptr) return false;

    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) return false;

    uint32_t magic = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != GPATCH_MAGIC) return false;

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != GPATCH_VERSION) return false;

    uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t cellIndex = 0;
        uint16_t encodedZ = 0;
        in.read(reinterpret_cast<char*>(&cellIndex), sizeof(cellIndex));
        in.read(reinterpret_cast<char*>(&encodedZ), sizeof(encodedZ));

        if (cellIndex < MAP_TOTAL_CELLS)
        {
            m_data[cellIndex] = encodedZ;
            m_modifiedCells[cellIndex] = encodedZ;
        }
    }

    return true;
}

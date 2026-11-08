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
    InitObjectDatabase();

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
                    OnLoaded(filepath);
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
                OnLoaded(filepath);
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
                OnLoaded(filepath);
                return true;
            }
        }
    }

    return false;
}

void HeightMap::OnLoaded(const std::string& path)
{
    m_isLoaded = true;
    m_loadedPath = path;
    m_dirtyCells.assign((MAP_TOTAL_CELLS + 63) / 64, 0ULL);
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
        m_carvedAreas.clear();
        m_appliedObjectKeys.clear();
        m_dirtyCells.clear();
        m_appliedObjects.clear();
    }
}

bool HeightMap::WorldToGrid(float x, float y, int& gx, int& gy) const
{
    if (!std::isfinite(x) || !std::isfinite(y))
    {
        return false;
    }
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

bool HeightMap::GetZ(float x, float y, float& z, bool interpolate, bool waterSurface) const
{
    if (!std::isfinite(x) || !std::isfinite(y))
    {
        z = 0.0f;
        return false;
    }
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
        z = ReadHeightFast(gx, gy);
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

        const float z00 = ReadHeightFast(x0, y0);
        const float z10 = ReadHeightFast(x1, y0);
        const float z01 = ReadHeightFast(x0, y1);
        const float z11 = ReadHeightFast(x1, y1);

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
    if (!std::isfinite(x) || !std::isfinite(y)) return false;
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
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(facingAngle)) return false;
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

    // Surface normal vector: N = (-dz_dx, -dz_dy, 1) normalized
    float nx = -dz_dx;
    float ny = -dz_dy;
    float nz = 1.0f;
    const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0.0001f)
    {
        nx /= len;
        ny /= len;
        nz /= len;
    }

    // In GTA SA, Euler rotation order for objects is R = Rz(facingAngle) * Ry(ry) * Rx(rx).
    // To align the object's local UP vector (0, 0, 1) with the terrain normal N:
    // Un-rotate the normal by Rz(facingAngle):
    const float headingRad = facingAngle * DEG_TO_RAD;
    const float crz = std::cos(headingRad);
    const float srz = std::sin(headingRad);

    const float unX = crz * nx + srz * ny;
    const float unY = -srz * nx + crz * ny;
    const float unZ = nz;

    // Solve for local Euler angles rx and ry:
    rx = std::asin(std::clamp(-unY, -1.0f, 1.0f)) * RAD_TO_DEG;
    ry = std::atan2(unX, unZ) * RAD_TO_DEG;
    return true;
}

bool HeightMap::RaycastLine(float x1, float y1, float z1, float x2, float y2, float z2, float& hitX, float& hitY, float& hitZ) const
{
    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(z1) || !std::isfinite(x2) || !std::isfinite(y2) || !std::isfinite(z2)) return false;
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
    if (!std::isfinite(x) || !std::isfinite(y)) return false;
    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (WaterSystem::GetWaterLevelAt(x, y, surfaceZ, bottomZ))
    {
        float groundZ = 0.0f;
        if (GetZ(x, y, groundZ, true, false))
        {
            if (groundZ < surfaceZ)
            {
                waterDepth = surfaceZ - groundZ;
                return true;
            }
            if (std::abs(surfaceZ - SEA_LEVEL_Z) < 0.01f && groundZ <= 0.01f)
            {
                waterDepth = 2.0f;
                return true;
            }
        }
    }

    waterDepth = 0.0f;
    return false;
}

bool HeightMap::GetClearance(float x, float y, float z, float& clearance, bool& isUnderground) const
{
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
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

inline int64_t HeightMap::CrossProduct2D(const GridPoint& o, const GridPoint& a, const GridPoint& b)
{
    return static_cast<int64_t>(a.x - o.x) * static_cast<int64_t>(b.y - o.y) -
           static_cast<int64_t>(a.y - o.y) * static_cast<int64_t>(b.x - o.x);
}

std::vector<HeightMap::GridPoint> HeightMap::ComputeConvexHull(std::vector<GridPoint>& pts)
{
    if (pts.size() <= 2)
    {
        return pts;
    }

    std::sort(pts.begin(), pts.end());
    pts.erase(std::unique(pts.begin(), pts.end()), pts.end());

    if (pts.size() <= 2)
    {
        return pts;
    }

    const size_t n = pts.size();
    std::vector<GridPoint> lower;
    lower.reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
        while (lower.size() >= 2 && CrossProduct2D(lower[lower.size() - 2], lower.back(), pts[i]) <= 0)
        {
            lower.pop_back();
        }
        lower.push_back(pts[i]);
    }

    std::vector<GridPoint> upper;
    upper.reserve(n);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i)
    {
        while (upper.size() >= 2 && CrossProduct2D(upper[upper.size() - 2], upper.back(), pts[static_cast<size_t>(i)]) <= 0)
        {
            upper.pop_back();
        }
        upper.push_back(pts[static_cast<size_t>(i)]);
    }

    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}

inline bool HeightMap::IsPointInConvexHull(const std::vector<GridPoint>& hull, int x, int y)
{
    const size_t n = hull.size();
    if (n < 3) return false;
    GridPoint p{ x, y };
    for (size_t i = 0; i < n; ++i)
    {
        size_t j = (i + 1 == n) ? 0 : i + 1;
        if (CrossProduct2D(hull[i], hull[j], p) < 0)
        {
            return false;
        }
    }
    return true;
}

inline float HeightMap::DistanceSqToSegment(float px, float py, float ax, float ay, float bx, float by)
{
    const float abx = bx - ax;
    const float aby = by - ay;
    const float apx = px - ax;
    const float apy = py - ay;
    const float abLenSq = abx * abx + aby * aby;
    if (abLenSq <= 1e-9f)
    {
        return apx * apx + apy * apy;
    }
    const float t = std::clamp((apx * abx + apy * aby) / abLenSq, 0.0f, 1.0f);
    const float projX = ax + t * abx;
    const float projY = ay + t * aby;
    const float dx = px - projX;
    const float dy = py - projY;
    return dx * dx + dy * dy;
}

float HeightMap::MinDistanceToHull(const std::vector<GridPoint>& hull, int gx, int gy)
{
    if (hull.empty()) return 0.0f;
    if (hull.size() == 1)
    {
        float dx = static_cast<float>(gx - hull[0].x);
        float dy = static_cast<float>(gy - hull[0].y);
        return std::sqrt(dx * dx + dy * dy);
    }
    if (hull.size() == 2)
    {
        return std::sqrt(DistanceSqToSegment(static_cast<float>(gx), static_cast<float>(gy),
                                             static_cast<float>(hull[0].x), static_cast<float>(hull[0].y),
                                             static_cast<float>(hull[1].x), static_cast<float>(hull[1].y)));
    }
    if (IsPointInConvexHull(hull, gx, gy))
    {
        return 0.0f;
    }
    float minSq = 1e12f;
    const size_t n = hull.size();
    const float px = static_cast<float>(gx);
    const float py = static_cast<float>(gy);
    for (size_t i = 0; i < n; ++i)
    {
        size_t j = (i + 1 == n) ? 0 : i + 1;
        float dSq = DistanceSqToSegment(px, py,
                                        static_cast<float>(hull[i].x), static_cast<float>(hull[i].y),
                                        static_cast<float>(hull[j].x), static_cast<float>(hull[j].y));
        if (dSq < minSq)
        {
            minSq = dSq;
        }
    }
    return std::sqrt(minSq);
}


bool HeightMap::IsPointInCarvedArea(int gx, int gy) const
{
    for (const auto& area : m_carvedAreas)
    {
        if (gx >= area.minGx && gx <= area.maxGx &&
            gy >= area.minGy && gy <= area.maxGy)
        {
            if (MinDistanceToHull(area.hull, gx, gy) <= 2.0f)
            {
                return true;
            }
        }
    }
    return false;
}


bool HeightMap::RemoveBuilding(int modelId, float x, float y, float z, float radius)
{
    if (!m_isLoaded || m_data == nullptr)
    {
        return false;
    }

    // Safety guard: Model ID -1 (SA-MP wildcard for all models) must be ignored
    if (modelId < 0)
    {
        return true;
    }

    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(radius))
    {
        return false;
    }

    if (std::abs(x) > 20000.0f || std::abs(y) > 20000.0f || std::abs(z) > 20000.0f || radius < 0.0f)
    {
        return false;
    }

    // Safety guard: Global map-wide wipe pattern (e.g. RemoveBuildingForPlayer(playerid, model, 0.0, 0.0, 0.0, 6000.0))
    if (std::abs(x) < 1.0f && std::abs(y) < 1.0f && radius > 500.0f)
    {
        return true;
    }

    // Deduplication check: modelId and quantized coordinates
    const int qx = static_cast<int>(std::round(x * 2.0f));
    const int qy = static_cast<int>(std::round(y * 2.0f));
    const int qz = static_cast<int>(std::round(z * 2.0f));

    RemovedBuildingKey bKey{ modelId, qx, qy, qz };
    if (!m_removedBuildings.insert(bKey).second)
    {
        return true; // Already processed in this server session
    }

    // Pre-conditions: discard unknown models or flat props
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    if (!GetModelCollisionDimensions(modelId, sx, sy, sz))
    {
        return true; // Unknown model: skip
    }

    if (sz < 2.5f || (std::max)(sx, sy) < 2.0f)
    {
        return true; // Flat props, small benches, thin poles: skip
    }

    int centerGx = 0, centerGy = 0;
    if (!WorldToGrid(x, y, centerGx, centerGy))
    {
        return false;
    }

    // Plateau detection (BFS)
    struct PlateauResult
    {
        bool success{ false };
        bool aborted{ false };
        bool isRoof{ false };
        std::vector<GridPoint> cells;
        std::vector<GridPoint> boundary;
        int perimeter{ 0 };
        int drops{ 0 };
    };

    const float maxDist = (std::min)(120.0f, std::hypot(sx, sy) + 30.0f);
    constexpr int maxCells = 20000;
    const float maxDistSq = maxDist * maxDist;

    auto findPlateau = [&](int seedGx, int seedGy) -> PlateauResult
    {
        PlateauResult res;
        if (seedGx < 0 || seedGx >= MAP_GRID_WIDTH || seedGy < 0 || seedGy >= MAP_GRID_HEIGHT)
        {
            return res;
        }

        const int R = static_cast<int>(std::ceil(maxDist)) + 2;
        const int winMinX = (std::max)(0, seedGx - R);
        const int winMaxX = (std::min)(MAP_GRID_WIDTH - 1, seedGx + R);
        const int winMinY = (std::max)(0, seedGy - R);
        const int winMaxY = (std::min)(MAP_GRID_HEIGHT - 1, seedGy + R);
        const int winW = winMaxX - winMinX + 1;
        const int winH = winMaxY - winMinY + 1;

        std::vector<uint8_t> visited(static_cast<size_t>(winW) * static_cast<size_t>(winH), 0);
        std::vector<uint32_t> queue;
        queue.reserve(4096);

        auto markVisited = [&](int gx, int gy) {
            visited[static_cast<size_t>(gy - winMinY) * winW + static_cast<size_t>(gx - winMinX)] = 1;
        };
        auto isVisited = [&](int gx, int gy) -> bool {
            return visited[static_cast<size_t>(gy - winMinY) * winW + static_cast<size_t>(gx - winMinX)] != 0;
        };

        queue.push_back((static_cast<uint32_t>(seedGx) << 16) | static_cast<uint32_t>(seedGy));
        markVisited(seedGx, seedGy);
        res.cells.push_back({ seedGx, seedGy });

        size_t head = 0;
        while (head < queue.size())
        {
            uint32_t packed = queue[head++];
            int curGx = static_cast<int>(packed >> 16);
            int curGy = static_cast<int>(packed & 0xFFFF);
            float curZ = static_cast<float>(m_data[static_cast<size_t>(curGy) * MAP_GRID_WIDTH + static_cast<size_t>(curGx)]) * 0.01f;

            const int dxArr[4] = { 1, -1, 0, 0 };
            const int dyArr[4] = { 0, 0, 1, -1 };

            for (int i = 0; i < 4; ++i)
            {
                int nGx = curGx + dxArr[i];
                int nGy = curGy + dyArr[i];

                if (nGx < 0 || nGx >= MAP_GRID_WIDTH || nGy < 0 || nGy >= MAP_GRID_HEIGHT)
                {
                    continue;
                }

                float dFromSeedX = static_cast<float>(nGx - seedGx);
                float dFromSeedY = static_cast<float>(nGy - seedGy);
                float distFromSeedSq = dFromSeedX * dFromSeedX + dFromSeedY * dFromSeedY;

                float nZ = static_cast<float>(m_data[static_cast<size_t>(nGy) * MAP_GRID_WIDTH + static_cast<size_t>(nGx)]) * 0.01f;

                if (std::abs(nZ - curZ) <= 1.5f)
                {
                    if (distFromSeedSq > maxDistSq || res.cells.size() >= static_cast<size_t>(maxCells))
                    {
                        res.aborted = true;
                        return res;
                    }

                    if (nGx >= winMinX && nGx <= winMaxX && nGy >= winMinY && nGy <= winMaxY)
                    {
                        if (!isVisited(nGx, nGy))
                        {
                            markVisited(nGx, nGy);
                            queue.push_back((static_cast<uint32_t>(nGx) << 16) | static_cast<uint32_t>(nGy));
                            res.cells.push_back({ nGx, nGy });
                        }
                    }
                }
            }
        }

        res.boundary.reserve(res.cells.size() / 2);
        for (const auto& pt : res.cells)
        {
            float curZ = static_cast<float>(m_data[static_cast<size_t>(pt.y) * MAP_GRID_WIDTH + static_cast<size_t>(pt.x)]) * 0.01f;
            bool isBoundaryCell = false;

            const int dxArr[4] = { 1, -1, 0, 0 };
            const int dyArr[4] = { 0, 0, 1, -1 };

            for (int i = 0; i < 4; ++i)
            {
                int nGx = pt.x + dxArr[i];
                int nGy = pt.y + dyArr[i];

                bool inPlateau = false;
                if (nGx >= winMinX && nGx <= winMaxX && nGy >= winMinY && nGy <= winMaxY)
                {
                    inPlateau = isVisited(nGx, nGy);
                }

                if (!inPlateau)
                {
                    isBoundaryCell = true;
                    res.perimeter++;

                    if (nGx >= 0 && nGx < MAP_GRID_WIDTH && nGy >= 0 && nGy < MAP_GRID_HEIGHT)
                    {
                        float nZ = static_cast<float>(m_data[static_cast<size_t>(nGy) * MAP_GRID_WIDTH + static_cast<size_t>(nGx)]) * 0.01f;
                        if (curZ - nZ > 2.5f)
                        {
                            res.drops++;
                        }
                    }
                }
            }

            if (isBoundaryCell)
            {
                res.boundary.push_back(pt);
            }
        }

        res.isRoof = (res.drops >= (std::max)(8, static_cast<int>(0.10f * static_cast<float>(res.perimeter))));
        res.success = true;
        return res;
    };

    std::vector<GridPoint> hull;
    PlateauResult plateau = findPlateau(centerGx, centerGy);
    bool roofDetected = (plateau.success && plateau.isRoof);

    if (roofDetected)
    {
        hull = ComputeConvexHull(plateau.boundary);
        if (hull.empty())
        {
            roofDetected = false;
        }
    }

    if (!roofDetected)
    {
        // If the removal point has no rooftop plateau and lies within an already
        // carved area (e.g. LOD model, secondary prop, glass, or patch already applied),
        // safely skip Pass 2 search to prevent excavating neighboring structures.
        if (IsPointInCarvedArea(centerGx, centerGy))
        {
            return true;
        }

        // Pass 2: Search for highest cell in radius rs = clamp(0.5 * hypot(sx, sy), 3, 20) m
        const float rs = std::clamp(0.5f * std::hypot(sx, sy), 3.0f, 20.0f);
        const float rsSq = rs * rs;
        const int searchR = static_cast<int>(std::ceil(rs));

        const float seedZ = static_cast<float>(m_data[static_cast<size_t>(centerGy) * MAP_GRID_WIDTH + static_cast<size_t>(centerGx)]) * 0.01f;
        float highestZ = seedZ + 2.5f;
        int bestGx = -1;
        int bestGy = -1;

        for (int dy = -searchR; dy <= searchR; ++dy)
        {
            const int sGy = centerGy + dy;
            if (sGy < 0 || sGy >= MAP_GRID_HEIGHT) continue;
            for (int dx = -searchR; dx <= searchR; ++dx)
            {
                const int sGx = centerGx + dx;
                if (sGx < 0 || sGx >= MAP_GRID_WIDTH) continue;
                if (static_cast<float>(dx * dx + dy * dy) <= rsSq)
                {
                    float cz = static_cast<float>(m_data[static_cast<size_t>(sGy) * MAP_GRID_WIDTH + static_cast<size_t>(sGx)]) * 0.01f;
                    if (cz > highestZ)
                    {
                        highestZ = cz;
                        bestGx = sGx;
                        bestGy = sGy;
                    }
                }
            }
        }

        if (bestGx != -1 && bestGy != -1)
        {
            PlateauResult pass2 = findPlateau(bestGx, bestGy);
            if (pass2.success && pass2.isRoof)
            {
                // Candidate roof is accepted ONLY if its footprint encloses the removal point
                std::vector<GridPoint> pass2Hull = ComputeConvexHull(pass2.boundary);
                if (!pass2Hull.empty() && MinDistanceToHull(pass2Hull, centerGx, centerGy) <= 2.0f)
                {
                    plateau = std::move(pass2);
                    hull = std::move(pass2Hull);
                    roofDetected = true;
                }
                else
                {
                    // Candidate roof belongs to an adjacent/neighboring building: abort without altering
                    return true;
                }
            }
            else if (pass2.aborted)
            {
                return true;
            }
        }
        else if (plateau.aborted)
        {
            return true;
        }

        if (!roofDetected)
        {
            return true; // Not a roof, do not alter heightmap
        }
    }

    // Building footprint mask via convex hull
    if (hull.empty())
    {
        return true;
    }

    int hullMinGx = hull[0].x, hullMaxGx = hull[0].x;
    int hullMinGy = hull[0].y, hullMaxGy = hull[0].y;
    for (const auto& hp : hull)
    {
        if (hp.x < hullMinGx) hullMinGx = hp.x;
        if (hp.x > hullMaxGx) hullMaxGx = hp.x;
        if (hp.y < hullMinGy) hullMinGy = hp.y;
        if (hp.y > hullMaxGy) hullMaxGy = hp.y;
    }

    const int maskMinGx = (std::max)(0, hullMinGx - 2);
    const int maskMaxGx = (std::min)(MAP_GRID_WIDTH - 1, hullMaxGx + 2);
    const int maskMinGy = (std::max)(0, hullMinGy - 2);
    const int maskMaxGy = (std::min)(MAP_GRID_HEIGHT - 1, hullMaxGy + 2);

    const int winMinGx = (std::max)(0, maskMinGx - 6);
    const int winMaxGx = (std::min)(MAP_GRID_WIDTH - 1, maskMaxGx + 6);
    const int winMinGy = (std::max)(0, maskMinGy - 6);
    const int winMaxGy = (std::min)(MAP_GRID_HEIGHT - 1, maskMaxGy + 6);

    struct Sample
    {
        float u;
        float v;
        float z;
    };
    std::vector<GridPoint> maskCells;
    maskCells.reserve(static_cast<size_t>(maskMaxGx - maskMinGx + 1) * static_cast<size_t>(maskMaxGy - maskMinGy + 1));
    std::vector<Sample> primarySamples;
    std::vector<Sample> bufferSamples;
    primarySamples.reserve(2048);
    bufferSamples.reserve(1024);

    for (int gy = winMinGy; gy <= winMaxGy; ++gy)
    {
        const float wy = 3000.0f - static_cast<float>(gy);
        const float v = wy - y;

        for (int gx = winMinGx; gx <= winMaxGx; ++gx)
        {
            const float dHull = MinDistanceToHull(hull, gx, gy);

            if (dHull <= 2.0f)
            {
                // Mask: inside convex hull or within 2m dilation
                if (gx >= maskMinGx && gx <= maskMaxGx && gy >= maskMinGy && gy <= maskMaxGy)
                {
                    maskCells.push_back({ gx, gy });
                }
            }
            else if (dHull <= 8.0f)
            {
                // Outside mask, up to 6m from mask
                const float wx = static_cast<float>(gx - 3000);
                const float u = wx - x;
                const float cz = static_cast<float>(m_data[static_cast<size_t>(gy) * MAP_GRID_WIDTH + static_cast<size_t>(gx)]) * 0.01f;

                if (dHull >= 4.0f)
                {
                    // Primary sample: 2m to 6m away from mask
                    primarySamples.push_back({ u, v, cz });
                }
                else
                {
                    // Buffer sample: 0m to 2m away from mask
                    bufferSamples.push_back({ u, v, cz });
                }
            }
        }
    }

    if (maskCells.empty())
    {
        return true;
    }

    if (primarySamples.size() < 8)
    {
        primarySamples.insert(primarySamples.end(), bufferSamples.begin(), bufferSamples.end());
    }

    // Surrounding street lower envelope regression
    // Discard samples with z <= 0.2m (sea) if there are at least 8 samples with z > 0.2m
    size_t aboveSeaCount = 0;
    for (const auto& s : primarySamples)
    {
        if (s.z > 0.2f) aboveSeaCount++;
    }
    if (aboveSeaCount >= 8)
    {
        std::vector<Sample> filtered;
        filtered.reserve(aboveSeaCount);
        for (const auto& s : primarySamples)
        {
            if (s.z > 0.2f) filtered.push_back(s);
        }
        primarySamples = std::move(filtered);
    }

    float planeA = 0.0f;
    float planeB = 0.0f;
    float planeC = 0.0f;

    if (primarySamples.size() >= 8)
    {
        std::vector<Sample> curSamples = primarySamples;
        for (int it = 0; it < 4; ++it)
        {
            if (curSamples.size() < 8) break;

            float sumU = 0.0f, sumV = 0.0f, sumZ = 0.0f;
            for (const auto& s : curSamples)
            {
                sumU += s.u;
                sumV += s.v;
                sumZ += s.z;
            }
            const float invN = 1.0f / static_cast<float>(curSamples.size());
            const float meanU = sumU * invN;
            const float meanV = sumV * invN;
            const float meanZ = sumZ * invN;

            float varU = 0.0f, varV = 0.0f, covUZ = 0.0f, covVZ = 0.0f;
            for (const auto& s : curSamples)
            {
                const float du = s.u - meanU;
                const float dv = s.v - meanV;
                const float dz = s.z - meanZ;
                varU += du * du;
                varV += dv * dv;
                covUZ += du * dz;
                covVZ += dv * dz;
            }

            float a = (varU > 1e-4f) ? (covUZ / varU) : 0.0f;
            float b = (varV > 1e-4f) ? (covVZ / varV) : 0.0f;
            a = std::clamp(a, -0.30f, 0.30f);
            b = std::clamp(b, -0.30f, 0.30f);
            float c = meanZ - a * meanU - b * meanV;

            planeA = a;
            planeB = b;
            planeC = c;

            std::vector<Sample> nextSamples;
            nextSamples.reserve(curSamples.size());
            for (const auto& s : curSamples)
            {
                float planeZ = planeC + planeA * s.u + planeB * s.v;
                if (s.z <= planeZ + 1.0f)
                {
                    nextSamples.push_back(s);
                }
            }

            if (nextSamples.size() == curSamples.size() || nextSamples.size() < 8)
            {
                break;
            }
            curSamples = std::move(nextSamples);
        }
    }
    else if (!primarySamples.empty())
    {
        // Fallback: 10th percentile of heights as horizontal plane
        std::vector<float> zVals;
        zVals.reserve(primarySamples.size());
        for (const auto& s : primarySamples)
        {
            zVals.push_back(s.z);
        }
        std::sort(zVals.begin(), zVals.end());
        size_t p10Index = static_cast<size_t>(0.10f * static_cast<float>(zVals.size()));
        if (p10Index >= zVals.size()) p10Index = 0;
        planeA = 0.0f;
        planeB = 0.0f;
        planeC = zVals[p10Index];
    }
    else
    {
        return true; // No samples available, do not alter heightmap
    }

    // Carve mask cells to street plane
    for (const auto& pt : maskCells)
    {
        const float wx = static_cast<float>(pt.x - 3000);
        const float wy = 3000.0f - static_cast<float>(pt.y);
        const float u = wx - x;
        const float v = wy - y;

        const float newZ = planeC + planeA * u + planeB * v;
        const size_t cellIndex = static_cast<size_t>(pt.y) * MAP_GRID_WIDTH + static_cast<size_t>(pt.x);
        const float currentZ = static_cast<float>(m_data[cellIndex]) * 0.01f;

        // Golden Rule: Only lower height, never raise
        if (newZ < currentZ)
        {
            uint16_t encodedZ = static_cast<uint16_t>(std::clamp(newZ * 100.0f, 0.0f, 65535.0f));
            m_data[cellIndex] = encodedZ;
            MarkCellDirty(cellIndex);
        }
    }

    // Register carved area with exact convex hull footprint and bounding box
    m_carvedAreas.push_back({ hull, maskMinGx, maskMinGy, maskMaxGx, maskMaxGy });

    // Reapply stamped objects overlapping mask
    for (const auto& obj : m_appliedObjects)
    {
        int ogx = 0, ogy = 0;
        WorldToGrid(obj.x, obj.y, ogx, ogy);
        const float objRadius = GetModelPhysicalRadius(obj.modelId);
        const int objMargin = static_cast<int>(std::ceil(objRadius));
        if (ogx + objMargin >= maskMinGx && ogx - objMargin <= maskMaxGx &&
            ogy + objMargin >= maskMinGy && ogy - objMargin <= maskMaxGy)
        {
            RasterizeObject(obj.modelId, obj.x, obj.y, obj.z, obj.rx, obj.ry, obj.rz,
                            obj.forceReplace, obj.customSizeX, obj.customSizeY, obj.customSizeZ);
        }
    }

    return true;
}


bool HeightMap::ApplyObject(int modelId, float x, float y, float z, float rx, float ry, float rz,
                            bool forceReplace, float customSizeX, float customSizeY, float customSizeZ)
{
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        !std::isfinite(rx) || !std::isfinite(ry) || !std::isfinite(rz))
    {
        return false;
    }
    if (std::abs(x) > 20000.0f || std::abs(y) > 20000.0f || std::abs(z) > 20000.0f)
    {
        return false;
    }
    if (!m_isLoaded || m_data == nullptr)
    {
        return false;
    }

    // Validate custom dimensions
    if (customSizeX > 0.0f || customSizeY > 0.0f || customSizeZ > 0.0f)
    {
        if (!std::isfinite(customSizeX) || !std::isfinite(customSizeY) || !std::isfinite(customSizeZ) ||
            customSizeX <= 0.0f || customSizeY <= 0.0f || customSizeZ <= 0.0f ||
            customSizeX > 300.0f || customSizeY > 300.0f || customSizeZ > 300.0f)
        {
            return false;
        }
    }
    else
    {
        float dummyX = 0.0f, dummyY = 0.0f, dummyZ = 0.0f;
        if (!GetModelCollisionDimensions(modelId, dummyX, dummyY, dummyZ))
        {
            return false; // Unknown model without dimensions
        }
    }

    // Deduplicate applied objects
    AppliedObjectKey aKey{
        modelId,
        static_cast<int>(std::round(x * 2.0f)),
        static_cast<int>(std::round(y * 2.0f)),
        static_cast<int>(std::round(z * 2.0f)),
        static_cast<int>(std::round(rx)),
        static_cast<int>(std::round(ry)),
        static_cast<int>(std::round(rz))
    };
    if (!m_appliedObjectKeys.insert(aKey).second)
    {
        return true; // Already applied
    }

    m_appliedObjects.push_back({
        modelId, x, y, z, rx, ry, rz, forceReplace, customSizeX, customSizeY, customSizeZ
    });

    return RasterizeObject(modelId, x, y, z, rx, ry, rz, forceReplace, customSizeX, customSizeY, customSizeZ);
}

bool HeightMap::RasterizeObject(int modelId, float x, float y, float z, float rx, float ry, float rz,
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

    // Rasterization grid bounds with strict map boundary clamping
    if (maxX < MAP_MIN_X || minX > MAP_MAX_X || maxY < MAP_MIN_Y || minY > MAP_MAX_Y)
    {
        return false; // Entirely outside the world map
    }

    float cMinX = std::clamp(minX, MAP_MIN_X, MAP_MAX_X);
    float cMaxX = std::clamp(maxX, MAP_MIN_X, MAP_MAX_X);
    float cMinY = std::clamp(minY, MAP_MIN_Y, MAP_MAX_Y);
    float cMaxY = std::clamp(maxY, MAP_MIN_Y, MAP_MAX_Y);

    int gx1 = 0, gy1 = 0, gx2 = 0, gy2 = 0;
    WorldToGrid(cMinX, cMinY, gx1, gy1);
    WorldToGrid(cMaxX, cMaxY, gx2, gy2);

    int minGx = std::clamp((std::min)(gx1, gx2), 0, MAP_GRID_WIDTH - 1);
    int maxGx = std::clamp((std::max)(gx1, gx2), 0, MAP_GRID_WIDTH - 1);
    int minGy = std::clamp((std::min)(gy1, gy2), 0, MAP_GRID_HEIGHT - 1);
    int maxGy = std::clamp((std::max)(gy1, gy2), 0, MAP_GRID_HEIGHT - 1);

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
        const float wy = 3000.0f - static_cast<float>(gy);

        for (int gx = minGx; gx <= maxGx; ++gx)
        {
            const float wx = static_cast<float>(gx - 3000);

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
                    MarkCellDirty(cellIndex);
                }
            }
        }
    }

    return true;
}

bool HeightMap::IsSafeForSpawn(float x, float y, float maxSlope) const
{
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(maxSlope)) return false;
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
    if (!std::isfinite(searchX) || !std::isfinite(searchY) || !std::isfinite(maxSlope) || !std::isfinite(searchRadius)) return false;
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
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(vz) || !std::isfinite(gravity)) return false;
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
    if (!std::isfinite(radarX) || !std::isfinite(radarY) || !std::isfinite(radarZ) || !std::isfinite(targetX) || !std::isfinite(targetY) || !std::isfinite(targetZ)) return false;
    if (!m_isLoaded || m_data == nullptr) return false;

    float hitX = 0.0f, hitY = 0.0f, hitZ = 0.0f;
    return RaycastLine(radarX, radarY, radarZ, targetX, targetY, targetZ, hitX, hitY, hitZ);
}

bool HeightMap::GetAreaElevationStats(float minX, float minY, float maxX, float maxY,
                                     float& minZ, float& maxZ, float& avgZ, float& maxSlopeDeg) const
{
    if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(maxX) || !std::isfinite(maxY)) return false;
    if (!m_isLoaded || m_data == nullptr) return false;

    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);

    minX = (std::max)(MAP_MIN_X, minX);
    maxX = (std::min)(MAP_MAX_X, maxX);
    minY = (std::max)(MAP_MIN_Y, minY);
    maxY = (std::min)(MAP_MAX_Y, maxY);

    int gx1 = 0, gy1 = 0, gx2 = 0, gy2 = 0;
    if (!WorldToGrid(minX, minY, gx1, gy1) || !WorldToGrid(maxX, maxY, gx2, gy2))
    {
        return false;
    }

    int minGx = (std::min)(gx1, gx2);
    int maxGx = (std::max)(gx1, gx2);
    int minGy = (std::min)(gy1, gy2);
    int maxGy = (std::max)(gy1, gy2);

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

bool HeightMap::GetGroundZPosArea(float minX, float minY, float maxX, float maxY,
                                  float& avgZ, float& maxZ, float& minZ, bool waterSurface) const
{
    if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(maxX) || !std::isfinite(maxY)) return false;
    if (!m_isLoaded || m_data == nullptr) return false;

    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);

    minX = (std::max)(MAP_MIN_X, minX);
    maxX = (std::min)(MAP_MAX_X, maxX);
    minY = (std::max)(MAP_MIN_Y, minY);
    maxY = (std::min)(MAP_MAX_Y, maxY);

    int gx1 = 0, gy1 = 0, gx2 = 0, gy2 = 0;
    if (!WorldToGrid(minX, minY, gx1, gy1) || !WorldToGrid(maxX, maxY, gx2, gy2))
    {
        return false;
    }

    int minGx = (std::min)(gx1, gx2);
    int maxGx = (std::max)(gx1, gx2);
    int minGy = (std::min)(gy1, gy2);
    int maxGy = (std::max)(gy1, gy2);

    int spanX = maxGx - minGx + 1;
    int spanY = maxGy - minGy + 1;
    if (spanX <= 0 || spanY <= 0) return false;

    int stepX = (std::max)(1, spanX / 50);
    int stepY = (std::max)(1, spanY / 50);

    float curMinZ = 99999.0f;
    float curMaxZ = -99999.0f;
    float sumZ = 0.0f;
    int sampleCount = 0;

    for (int gy = minGy; gy <= maxGy; gy += stepY)
    {
        for (int gx = minGx; gx <= maxGx; gx += stepX)
        {
            float z = ReadHeightRaw(gx, gy);
            if (waterSurface && z < SEA_LEVEL_Z)
            {
                z = SEA_LEVEL_Z;
            }
            curMinZ = (std::min)(curMinZ, z);
            curMaxZ = (std::max)(curMaxZ, z);
            sumZ += z;
            sampleCount++;
        }
    }

    if (sampleCount == 0) return false;

    avgZ = sumZ / static_cast<float>(sampleCount);
    maxZ = curMaxZ;
    minZ = curMinZ;
    return true;
}

bool HeightMap::GetGroundZPosRadius(float centerX, float centerY, float radius,
                                    float& avgZ, float& maxZ, float& minZ, bool waterSurface) const
{
    if (!std::isfinite(centerX) || !std::isfinite(centerY) || !std::isfinite(radius)) return false;
    if (!m_isLoaded || m_data == nullptr || radius <= 0.0f) return false;

    float minX = (std::max)(MAP_MIN_X, centerX - radius);
    float maxX = (std::min)(MAP_MAX_X, centerX + radius);
    float minY = (std::max)(MAP_MIN_Y, centerY - radius);
    float maxY = (std::min)(MAP_MAX_Y, centerY + radius);

    int gx1 = 0, gy1 = 0, gx2 = 0, gy2 = 0;
    if (!WorldToGrid(minX, minY, gx1, gy1) || !WorldToGrid(maxX, maxY, gx2, gy2))
    {
        return false;
    }

    int minGx = (std::min)(gx1, gx2);
    int maxGx = (std::max)(gx1, gx2);
    int minGy = (std::min)(gy1, gy2);
    int maxGy = (std::max)(gy1, gy2);

    int spanX = maxGx - minGx + 1;
    int spanY = maxGy - minGy + 1;
    if (spanX <= 0 || spanY <= 0) return false;

    int stepX = (std::max)(1, spanX / 50);
    int stepY = (std::max)(1, spanY / 50);

    const float radiusSq = radius * radius;
    float curMinZ = 99999.0f;
    float curMaxZ = -99999.0f;
    float sumZ = 0.0f;
    int sampleCount = 0;

    for (int gy = minGy; gy <= maxGy; gy += stepY)
    {
        const float worldY = 3000.0f - static_cast<float>(gy);
        const float dy = worldY - centerY;
        const float dySq = dy * dy;

        for (int gx = minGx; gx <= maxGx; gx += stepX)
        {
            const float worldX = static_cast<float>(gx) - 3000.0f;
            const float dx = worldX - centerX;

            if ((dx * dx + dySq) <= radiusSq)
            {
                float z = ReadHeightRaw(gx, gy);
                if (waterSurface && z < SEA_LEVEL_Z)
                {
                    z = SEA_LEVEL_Z;
                }
                curMinZ = (std::min)(curMinZ, z);
                curMaxZ = (std::max)(curMaxZ, z);
                sumZ += z;
                sampleCount++;
            }
        }
    }

    if (sampleCount == 0)
    {
        float centerZ = 0.0f;
        if (GetZ(centerX, centerY, centerZ, true, waterSurface))
        {
            avgZ = centerZ;
            maxZ = centerZ;
            minZ = centerZ;
            return true;
        }
        return false;
    }

    avgZ = sumZ / static_cast<float>(sampleCount);
    maxZ = curMaxZ;
    minZ = curMinZ;
    return true;
}

constexpr uint32_t GPATCH_MAGIC = 0x43545047; // 'G','P','T','C'
constexpr uint32_t GPATCH_VERSION = 1;

bool HeightMap::SavePatches(const std::string& filename) const
{
    if (!m_isLoaded || m_data == nullptr || m_dirtyCells.empty()) return false;

    // Count modified cells from bitset using popcount
    uint32_t count = 0;
    for (uint64_t word : m_dirtyCells)
    {
        if (word != 0)
        {
            count += static_cast<uint32_t>(std::bitset<64>(word).count());
        }
    }

    const std::string tmpFilename = filename + ".tmp";
    std::ofstream out(tmpFilename, std::ios::binary);
    if (!out.is_open()) return false;

    out.write(reinterpret_cast<const char*>(&GPATCH_MAGIC), sizeof(GPATCH_MAGIC));
    out.write(reinterpret_cast<const char*>(&GPATCH_VERSION), sizeof(GPATCH_VERSION));
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (size_t wordIdx = 0; wordIdx < m_dirtyCells.size(); ++wordIdx)
    {
        uint64_t word = m_dirtyCells[wordIdx];
        if (word != 0)
        {
            for (size_t bit = 0; bit < 64; ++bit)
            {
                if (word & (1ULL << bit))
                {
                    uint32_t cellIndex = static_cast<uint32_t>((wordIdx << 6) + bit);
                    if (cellIndex < MAP_TOTAL_CELLS)
                    {
                        uint16_t encodedZ = m_data[cellIndex];
                        out.write(reinterpret_cast<const char*>(&cellIndex), sizeof(cellIndex));
                        out.write(reinterpret_cast<const char*>(&encodedZ), sizeof(encodedZ));
                    }
                }
            }
        }
    }

    out.flush();
    if (!out.good())
    {
        out.close();
        std::remove(tmpFilename.c_str());
        return false;
    }
    out.close();

    // Atomic replace
#if defined(_WIN32) || defined(WIN32)
    if (!MoveFileExA(tmpFilename.c_str(), filename.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        std::remove(tmpFilename.c_str());
        return false;
    }
#else
    if (rename(tmpFilename.c_str(), filename.c_str()) != 0)
    {
        std::remove(tmpFilename.c_str());
        return false;
    }
#endif

    return true;
}

bool HeightMap::LoadPatches(const std::string& filename)
{
    if (!m_isLoaded || m_data == nullptr || m_dirtyCells.empty()) return false;

    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) return false;

    in.seekg(0, std::ios::end);
    std::streampos fileSize = in.tellg();
    in.seekg(0, std::ios::beg);

    if (fileSize < 12) return false;

    uint32_t magic = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (!in.good() || magic != GPATCH_MAGIC) return false;

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!in.good() || version != GPATCH_VERSION) return false;

    uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in.good() || count > MAP_TOTAL_CELLS) return false;

    const uint64_t expectedSize = 12ULL + static_cast<uint64_t>(count) * 6ULL;
    if (static_cast<uint64_t>(fileSize) != expectedSize) return false;

    struct PatchEntry
    {
        uint32_t cellIndex;
        uint16_t encodedZ;
    };

    std::vector<PatchEntry> entries(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        in.read(reinterpret_cast<char*>(&entries[i].cellIndex), sizeof(uint32_t));
        in.read(reinterpret_cast<char*>(&entries[i].encodedZ), sizeof(uint16_t));
        if (!in.good()) return false;
        if (entries[i].cellIndex >= MAP_TOTAL_CELLS) return false;
    }

    // Apply only after complete validation
    for (const auto& e : entries)
    {
        m_data[e.cellIndex] = e.encodedZ;
        MarkCellDirty(e.cellIndex);
    }

    return true;
}

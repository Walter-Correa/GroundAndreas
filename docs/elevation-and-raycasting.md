# Elevation & World Raycasting

This guide details the mathematical and algorithmic foundation of **GroundAndreas**: continuous bilinear interpolation, 2.5D world line-of-sight raycasting, surface normal vectors, slope calculation, and dynamic object slope alignment.

---

## 1. Continuous Bilinear Interpolation

The San Andreas heightmap grid consists of $6000 \times 6000$ discrete elevation nodes sampled at 1-meter intervals across the world coordinates $[-3000.0, +3000.0]$.

### The Discrete Staircase Problem
Querying raw heightmap values directly produces discrete integer staircases: every step across a 1-meter boundary results in an abrupt elevation jump. This causes jitter when calculating vehicle suspension dynamics, smooth ped walking, or camera placement on slopes.

### The Bilinear Interpolation Solution
GroundAndreas uses continuous bilinear surface interpolation across the four nearest elevation nodes:

```
  (x0, y1) Z01 +--------------------+ Z11 (x1, y1)
               |        ^ v         |
               |        |           |
               |        +--- (u, v) |
               |        |           |
               |        v           |
  (x0, y0) Z00 +--------------------+ Z10 (x1, y0)
               <------ u ------>
```

Given normalized fractional coordinates $u = x - x_0$ and $v = y - y_0 \in [0.0, 1.0)$:

$$Z(u, v) = (1 - u)(1 - v) Z_{00} + u(1 - v) Z_{10} + (1 - u) v Z_{01} + u v Z_{11}$$

This produces a continuous, differentiable surface via **continuous bilinear interpolation over a 1 m grid with 1 cm vertical resolution**.

---

## 2. Core Elevation Queries

### `GetGroundZPos`

```pawn
native GetGroundZPos(Float:x, Float:y, &Float:z, bool:interpolate = true, bool:water_surface = true);
```

* **`x`, `y`:** World coordinates between $-3000.0$ and $+3000.0$.
* **`&z`:** Output variable populated with the ground elevation.
* **`interpolate`:**
  * `true` (Default): Smooth bilinear interpolation across the 4 neighboring grid vertices.
  * `false`: Discrete 1m integer grid value (maximum raw read speed).
* **`water_surface`:** Kept for API compatibility. Because the heightmap stores unsigned uint16 elevations (range $0.0\text{m}$ to $655.35\text{m}$), ocean cells always evaluate to $0.0\text{m}$.
* **Returns:** `1` on success, `0` if coordinates are outside world boundaries.

#### Practical Usage Example
```pawn
new Float:zFloor;

// Query ground elevation (returns terrain Z on land, 0.0 in the ocean)
GetGroundZPos(360.0, -2090.0, zFloor, true, true);

printf("Water depth at coordinates: %.2f meters", zFloor - zBed);
```

---

### `GetGroundZPosBatch`

```pawn
native GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points, bool:interpolate = true, bool:water_surface = true);
```

When querying hundreds or thousands of coordinates simultaneously (e.g., bot navigation meshes, terrain radar scans, or vehicle convoy pathing), calling a native in a Pawn loop incurs AMX context-switch overhead.

`GetGroundZPosBatch` executes the queries entirely in native C++ memory:

```pawn
#define SCAN_POINTS 1000

new Float:scanX[SCAN_POINTS];
new Float:scanY[SCAN_POINTS];
new Float:scanZ[SCAN_POINTS];

// Generate coordinates...
for (new i = 0; i < SCAN_POINTS; i++)
{
    scanX[i] = -1500.0 + float(i);
    scanY[i] = 500.0;
}

// Process all 1,000 points in < 1.5ms
new processed = GetGroundZPosBatch(scanX, scanY, scanZ, SCAN_POINTS, true);
```

---

### `GetGroundZPosArea`

```pawn
native bool:GetGroundZPosArea(
    Float:minx, Float:miny, Float:maxx, Float:maxy,
    &Float:avg_z,
    &Float:max_z = 0.0,
    &Float:min_z = 0.0,
    bool:water_surface = true
);
```

Queries regional ground elevation metrics across a rectangular zone $[X_{\min}, Y_{\min}] 	imes [X_{\max}, Y_{\max}]$ (e.g., gang territories, dispute zones, procedural air-drop squares).

* **`minx`, `miny`, `maxx`, `maxy`:** Bounding box world coordinates in meters. The engine automatically normalizes inverted bounds ($X_{\min} > X_{\max}$).
* **`&avg_z`:** Mean ground elevation across all sampled grid cells in the zone.
* **`&max_z`:** Highest terrain elevation inside the zone (mountain crest or building roof).
* **`&min_z`:** Lowest terrain elevation inside the zone.
* **`water_surface`:** Kept for API compatibility; ocean cells evaluate to $0.0\text{m}$.
* **Returns:** `true` on success; `false` if the area lies completely outside world bounds $[-3000, +3000]$ or the heightmap is not loaded.

#### Gangzone Territory Pickup Positioning Example
```pawn
stock CreateTerritoryDisputePickup(Float:minx, Float:miny, Float:maxx, Float:maxy)
{
    new Float:centerX = (minx + maxx) / 2.0;
    new Float:centerY = (miny + maxy) / 2.0;
    new Float:avgZ, Float:maxZ, Float:minZ;

    // Discover the elevation profile of the entire gangzone
    if (GetGroundZPosArea(minx, miny, maxx, maxy, avgZ, maxZ, minZ))
    {
        // Place central dispute pickup 1.0m above the highest terrain feature
        // to guarantee full visibility from anywhere in the territory
        return CreateDynamicPickup(1240, 1, centerX, centerY, maxZ + 1.0);
    }
    return -1;
}
```

---

### `GetGroundZPosRadius`

```pawn
native bool:GetGroundZPosRadius(
    Float:x, Float:y, Float:radius,
    &Float:avg_z,
    &Float:max_z = 0.0,
    &Float:min_z = 0.0,
    bool:water_surface = true
);
```

Queries circular radial ground elevation metrics across a circle of radius $R$ centered at $(x, y)$.

* **`x`, `y`:** Center point world coordinates.
* **`radius`:** Radius in meters ($R > 0.0$).
* **`&avg_z`:** Mean ground elevation across all grid nodes within the circular radius ($dx^2 + dy^2 \le R^2$).
* **`&max_z`:** Highest terrain elevation inside the circle.
* **`&min_z`:** Lowest terrain elevation inside the circle.
* **`water_surface`:** Kept for API compatibility. Because the heightmap stores unsigned uint16 elevations (range $0.0\text{m}$ to $655.35\text{m}$), ocean cells always evaluate to $0.0\text{m}$.
* **Returns:** `true` on success; `false` if coordinates are out of bounds or `radius <= 0.0`.

#### Air-Drop Parachute Crate Safety Check
```pawn
stock bool:IsDropZoneFlat(Float:dropX, Float:dropY, Float:searchRadius)
{
    new Float:avgZ, Float:maxZ, Float:minZ;

    // Check a 30-meter radius around the target drop location
    if (GetGroundZPosRadius(dropX, dropY, searchRadius, avgZ, maxZ, minZ))
    {
        // If elevation delta across the circle exceeds 6 meters, zone is too rugged
        if ((maxZ - minZ) > 6.0)
        {
            return false;
        }
        return true;
    }
    return false;
}
```

---

## 3. 2.5D World Line-of-Sight Raycasting

In multiplayer servers, line-of-sight checks are critical for anti-cheat verification (detecting wallhacks, silent aim through hills), sniper targeting, and AI visibility.

Full 3D physics libraries (like ColAndreas/Bullet) execute complex triangle mesh traversal, consuming substantial CPU time. GroundAndreas provides ultra-fast **2.5D raycasting** (`IsWorldBlockingLine`) running in **under $1.0\,\mu\text{s}$**.

### How It Works
1. Traces a 3D ray from point $A (x_1, y_1, z_1)$ to point $B (x_2, y_2, z_2)$.
2. Steps through the heightmap along the 2D projection with adaptive sub-meter step intervals.
3. If the ray altitude drops below the terrain/building height at any sample, it triggers a **high-precision binary search** along the segment to locate the exact surface intersection point $(hit_x, hit_y, hit_z)$ with millimeter accuracy.

```
       (x1, y1, z1)
            \
             \    Ray Path
              \
               \   (hit_x, hit_y, hit_z)
================*=================== (Terrain / Hill)
                 \
                  \  (Occluded)
                   v
              (x2, y2, z2)
```

### Signature
```pawn
native bool:IsWorldBlockingLine(
    Float:x1, Float:y1, Float:z1,
    Float:x2, Float:y2, Float:z2,
    &Float:hit_x = 0.0, &Float:hit_y = 0.0, &Float:hit_z = 0.0
);
```

* **Returns:** `true` if terrain or buildings obstruct the line; `false` if line of sight is clear.
* **`hit_x`, `hit_y`, `hit_z`:** Filled with the collision impact coordinates on obstruction.

#### Anti-Cheat Line-of-Sight Check Example
```pawn
public OnPlayerWeaponShot(playerid, weaponid, hittype, hitid, Float:fX, Float:fY, Float:fZ)
{
    if (hittype == BULLET_HIT_TYPE_PLAYER)
    {
        new Float:pX, Float:pY, Float:pZ;
        new Float:tX, Float:tY, Float:tZ;
        GetPlayerCameraPos(playerid, pX, pY, pZ);
        GetPlayerPos(hitid, tX, tY, tZ);

        new Float:hitX, Float:hitY, Float:hitZ;
        // Verify if a mountain or building stands between the shooter and target
        if (IsWorldBlockingLine(pX, pY, pZ, tX, tY, tZ + 0.5, hitX, hitY, hitZ))
        {
            printf("[AC] Suspicious shot by player %d: obstructed by terrain at (%.1f, %.1f, %.1f)", playerid, hitX, hitY, hitZ);
            return 0; // Block invalid shot
        }
    }
    return 1;
}
```

---

## 4. Surface Normal Vector & Slope Calculation

Knowing the ground slope and orientation is vital for vehicle traction loss on cliffs, fall damage calculations, and anti-hillclimb scripts.

### Gradient & Unit Normal Math
GroundAndreas calculates the terrain normal using centered finite differences:

$$\frac{\partial Z}{\partial x} \approx \frac{Z(x + \delta, y) - Z(x - \delta, y)}{2\delta}, \quad \frac{\partial Z}{\partial y} \approx \frac{Z(x, y + \delta) - Z(x, y - \delta)}{2\delta}$$

The perpendicular 3D unit normal vector $\vec{N} = (n_x, n_y, n_z)$ is:

$$\vec{N} = \frac{(-\frac{\partial Z}{\partial x}, -\frac{\partial Z}{\partial y}, 1)}{\sqrt{\left(\frac{\partial Z}{\partial x}\right)^2 + \left(\frac{\partial Z}{\partial y}\right)^2 + 1}}$$

The slope inclination angle $\theta \in [0^\circ, 90^\circ]$ from horizontal ground is:

$$\theta = \arccos(n_z) \cdot \frac{180^\circ}{\pi}$$

### Signature
```pawn
native GetGroundNormal(Float:x, Float:y, &Float:nx, &Float:ny, &Float:nz, &Float:slope_degrees);
```

#### Anti-Hill-Climb / Slippery Slopes Example
```pawn
public OnPlayerUpdate(playerid)
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);

    new Float:nx, Float:ny, Float:nz, Float:slope;
    GetGroundNormal(x, y, nx, ny, nz, slope);

    // If slope is steeper than 50 degrees, apply sliding velocity
    if (slope > 50.0)
    {
        SetPlayerVelocity(playerid, nx * 0.25, ny * 0.25, -0.2);
    }
    return 1;
}
```

---

## 5. Dynamic Object Slope Alignment

When placing objects (e.g., police barricades, road cones, camping tents, medical stretchers, or dynamic ramps) on slopes, setting rotation to `0.0, 0.0` causes them to either hover unnaturally in the air or clip into the ground.

`GetGroundObjectRotation` computes the exact **Pitch ($RX$)** and **Roll ($RY$)** angles required so that the object sits flush against the ground, accounting for any custom compass heading (`facing_angle` / Yaw $RZ$).

### Signature
```pawn
native GetGroundObjectRotation(Float:x, Float:y, Float:facing_angle, &Float:rx, &Float:ry);
```

* **`facing_angle`:** Object heading direction in degrees ($0.0^\circ$ to $360.0^\circ$).
* **`&rx`, `&ry`:** Output Pitch ($RX$) and Roll ($RY$) rotation angles.

#### Practical Object Placement Example
```pawn
stock PlaceRoadConeOnSlope(Float:x, Float:y, Float:heading)
{
    new Float:z, Float:rx, Float:ry;
    GetGroundZPos(x, y, z);
    GetGroundObjectRotation(x, y, heading, rx, ry);

    // Create object flush with the sloped road
    return CreateObject(1238, x, y, z, rx, ry, heading);
}
```

### Complete Object Pose Alignment (`GetGroundObjectPose`)

Combines ground elevation querying and slope orientation in a single atomic call, optionally adding a vertical offset for the object pivot:

```pawn
native GetGroundObjectPose(Float:x, Float:y, Float:facing_angle, Float:z_offset, &Float:out_z, &Float:out_rx, &Float:out_ry, bool:interpolate = true);
```

* **`facing_angle`:** Object heading direction in degrees ($0.0^\circ$ to $360.0^\circ$).
* **`z_offset`:** Vertical elevation offset to add above the ground surface (e.g. object model center-of-mass or pivot height).
* **`&out_z`:** Output variable receiving final calculated elevation ($Z_{\text{ground}} + z_{\text{offset}}$).
* **`&out_rx`, `&out_ry`:** Output variables receiving Pitch ($RX$) and Roll ($RY$) rotation angles in degrees.
* **`interpolate`:** When `true` (default), applies continuous bilinear interpolation.
* **Returns:** `1` on success, `0` out of bounds or if heightmap is not loaded.

### 3D Alignment Between Two Ground Coordinates (`GetGroundTwoPointRotation`)
While `GetGroundObjectRotation` aligns an object at a single point based on the local tangent plane, `GetGroundTwoPointRotation` bridges an object directly between **two distinct coordinates** $(x_1, y_1)$ and $(x_2, y_2)$ (e.g., spanning a modular bridge, wooden barricade, or construction ramp up a steep hill):

```pawn
native GetGroundTwoPointRotation(
    Float:x1, Float:y1, Float:x2, Float:y2, Float:z_offset,
    &Float:center_z, &Float:rx, &Float:ry, &Float:rz,
    bool:interpolate = true
);
```

* Computes the exact 3D midpoint elevation $\frac{z_1 + z_2}{2} + z_{\text{offset}}$.
* Calculates the exact RenderWare Euler angles: Yaw ($RZ = \text{atan2}(dy, dx)$) and Pitch ($RY = -\text{pitch}$ along heading), leaving Roll ($RX = 0.0^\circ$).

```pawn
stock CreateRampBetweenPoints(Float:x1, Float:y1, Float:x2, Float:y2)
{
    new Float:cz, Float:rx, Float:ry, Float:rz;
    if (GetGroundTwoPointRotation(x1, y1, x2, y2, 0.5, cz, rx, ry, rz))
    {
        return CreateObject(1459, (x1 + x2) * 0.5, (y1 + y2) * 0.5, cz, rx, ry, rz);
    }
    return INVALID_OBJECT_ID;
}
```

### Contoured Line Segment Nodes (`GetGroundLinePoints`)
Samples a sequence of contoured nodes along a line segment at fixed intervals, extracting both elevation and surface inclination angles:

```pawn
native GetGroundLinePoints(
    Float:start_x, Float:start_y, Float:end_x, Float:end_y, Float:spacing, Float:z_offset,
    Float:out_x[], Float:out_y[], Float:out_z[],
    Float:out_rx[], Float:out_ry[], Float:out_rz[],
    max_points = sizeof(out_x)
);
```
Ideal for spawning roadside cone barriers, chain-link fences, guardrails, or lighting poles that naturally trace the undulations of the terrain.

---

## 6. Terrain Clearance & Submersion Queries

### `GetGroundClearance`
Computes clearance between an arbitrary point and the ground:
```pawn
native GetGroundClearance(Float:x, Float:y, Float:z, &Float:clearance, &bool:is_underground = false);
```
* **`clearance`:** Vertical distance ($z - z_{\text{ground}}$). Positive indicates above ground; negative indicates underground.
* **`is_underground`:** Populated with `true` if $z < z_{\text{ground}}$.

### `IsGroundUnderwater`
Checks if water exists at $(x, y)$ and terrain elevation is below the water surface level:
```pawn
native bool:IsGroundUnderwater(Float:x, Float:y, &Float:water_depth = 0.0);
```
* Returns `true` if water is present above the terrain, `false` on dry land.
* Populates `water_depth` with the water column depth ($Z_{\text{surface}} - Z_{\text{ground}}$). For inland lakes and pools, this reflects physical water depth against the basin floor; in the ocean (where the heightmap floor is $0.0\text{m}$), this reflects the active wave swell height ($0.15\text{m}$, $0.50\text{m}$, or $0.80\text{m}$).

# Dynamic Terrain & Objects

This guide details how **GroundAndreas** dynamically modifies terrain at runtime. It covers **automatic terrain slope reconstruction when buildings are removed**, **stamping mapped 3D objects onto the heightmap**, and **baking modifications to disk for instantaneous reloads**.

---

## 1. Dynamic Terrain Reconstruction on Building Removal

When a GTA building is removed via `RemoveBuildingForPlayer`, the original `SAfull.hmap` still contains the building's walls and roof elevation. Any script checking ground height inside the demolished area would falsely return the elevation of the old rooftop.

`GroundAndreasRemoveBuilding` repairs the heightmap in memory by reconstructing the natural ground slope underneath the removed structure.

```
       [ Before: Roof in Heightmap ]                [ After: Reconstructed Slope ]

              +-----------+ (Roof Z)
              |  Removed  |
              |  Building |
    ----------+           +----------           -----------------------------------
    (Ground)                 (Ground)           (Least-Squares Perimeter Slope Fit)
```

### The Plateau Detection Algorithm
1. **Model Pre-Conditions:** Checks physical collision dimensions ($sz \ge 2.5\text{m}$ and $\max(sx, sy) \ge 2.0\text{m}$) to safely filter out unknown models and extremely flat ground slabs. Objects that do not form an elevated rooftop on the heightmap (street light poles, road signs, benches, hydrants) do not alter the terrain because no elevated rooftop plateau is found at their position, leaving the ground untouched. Wildcard `modelid = -1` and global map wipes are also safely ignored.
2. **Carved Area Caching:** Records the exact convex hull footprint and bounding box of all excavated structures. If a subsequent removal request (such as an object's LOD, glass windows, interior stairs, or re-executions on servers using pre-loaded patches) lands inside an already excavated area and has no rooftop plateau at its coordinates, it returns immediately without running a radius search or touching adjacent buildings.
3. **Rooftop Seed Finding & Validation:** Starts at the grid cell of $(X, Y)$. If this seed sits at street/foundation level, the algorithm searches an adaptive radius $rs = \text{clamp}(0.5 \times \text{hypot}(sx, sy), 3, 20)\text{m}$ for the highest cell ($z > seedZ + 2.5\text{m}$). The searched roof candidate is accepted only if its convex hull footprint actually encompasses the removal point ($\text{MinDistanceToHull} \le 2.0\text{m}$), preventing accidental excavation of neighboring buildings. A candidate plateau is verified as a true building roof by counting vertical perimeter drops ($> 2.5\text{m}$ falloff to exterior neighbor); it is accepted if $\text{drops} \ge \max(8, 10\%\text{ of perimeter})$.
4. **Plateau Flood Fill (BFS):** Uses 4-neighbor breadth-first search to trace the contiguous roof surface ($|z_{\text{neighbor}} - z_{\text{current}}| \le 1.5\text{m}$). Safety guards abort processing if the region exceeds distance limits ($\le \min(120\text{m}, \text{hypot}(sx, sy) + 30\text{m})$) or 20,000 cells (preventing accidental excavation of open terrain, plazas, or freeways).
5. **Building Footprint Mask:** Computes the 2D convex hull of the plateau boundary cells using Andrew's Monotone Chain algorithm. The excavation mask encompasses all cells inside the convex hull plus a 2-meter dilation buffer.
6. **Surrounding Street Envelope:** Collects sample cells located 2m to 6m outside the building mask. Sea-level samples ($z \le 0.2\text{m}$) are discarded when terrestrial street samples exist. Fits a 3D plane equation ($z = C + A \cdot u + B \cdot v$) with slope gradients clamped to $\pm 0.30$. Performs up to 4 iterative refinement passes, stripping elevated terraces ($z > \text{plane} + 1.0\text{m}$) until pure street grade converges.
7. **Excavation & Cell Invalidation:** Sets each mask cell to the fitted street plane elevation strictly when `newZ < currentZ` (never raising existing ground). Marks modified cells dirty for patch serialization.

### Known Limitations
1. **Adjoining Buildings with Identical Roof Height:** Connected neighboring structures sharing the same rooftop elevation ($\le 1.5\text{m}$ step) are grouped into the same plateau and carved together.
2. **Independent LOD Removal:** Calling removal solely for an object's LOD model without the parent building will also excavate the structure if called in isolation.
3. **Terrace-Locked Buildings:** Structures surrounded on all sides exclusively by raised pedestrian terraces will level to the terrace height rather than distant street grade.
4. **Steep Exterior Roof Steps:** Complex multi-tiered roof steps with vertical drops $> 1.5\text{m}$ situated outside the convex hull footprint are not lowered.

### Signature
```pawn
native GroundAndreasRemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
```
* **`modelid`:** Model ID of the building to remove (wildcard `-1` is unsupported and ignored).
* **`fX, fY, fZ`:** World position coordinates of the removed building.
* **`fRadius`:** Search radius parameter from `RemoveBuildingForPlayer`. In GroundAndreas, `fRadius` does not define the excavation area; the excavation boundary is strictly derived from the detected rooftop contour and surrounding street envelope. Negative radius values return `0`.

---

## 2. Automatic `RemoveBuildingForPlayer` Hook

`GroundAndreas.inc` features a built-in ALS hook that automatically intercepts SA-MP's native `RemoveBuildingForPlayer`:

```pawn
// This native call in your gamemode removes a warehouse / building:
RemoveBuildingForPlayer(playerid, 4019, 1481.0, -1785.0, 15.0, 50.0);

// Automatically triggers GroundAndreasRemoveBuilding on the server,
// updating the heightmap strictly within the building's physical footprint (radius ~21.6m)!
```

### Disabling the Automatic Hook
If your gamemode requires manual control over building removal in GroundAndreas, define `GROUNDANDREAS_NO_REMOVE_BUILD` or `GA_NO_REMOVE_BUILDING` prior to including the library:

```pawn
#define GROUNDANDREAS_NO_REMOVE_BUILD
#include <GroundAndreas>
```

```pawn
#define GA_NO_REMOVE_BUILDING
#include <GroundAndreas>
```

---

## 3. Mapped Object Surface Stamping

Custom maps frequently introduce elevated roads, bridges, helipads, airport runways, and vehicle ramps. GroundAndreas allows you to **stamp dynamic 3D objects directly onto the heightfield** in memory with full rotation support.

### 3D Euler Incline Math
`ApplyObjectToGroundAndreas` calculates 3D coordinate transformations using Euler angles ($RX = \text{Pitch}, RY = \text{Roll}, RZ = \text{Yaw}$). For inclined ramps or tilted bridges, it derives the exact 3D plane equation:

$$Ax + By + Cz + D = 0 \implies z = -\frac{Ax + By + D}{C}$$

Every $(X, Y)$ coordinate falling within the oriented bounding box receives the exact surface elevation of the inclined object.

### Signature
```pawn
native ApplyObjectToGroundAndreas(
    modelid,
    Float:x, Float:y, Float:z,
    Float:rx, Float:ry, Float:rz,
    bool:force_replace = false,
    Float:custom_sizex = 0.0,
    Float:custom_sizey = 0.0,
    Float:custom_sizez = 0.0
);
```

* **`modelid`:** GTA object model ID.
* **`x, y, z`:** Center coordinates of the object.
* **`rx, ry, rz`:** Object Euler rotations in degrees.
* **`force_replace`:**
  * `false` (Default): Only stamps the surface if the object is *above* existing terrain (prevents burying natural hills).
  * `true`: Forcefully replaces ground elevation regardless of whether the object sits higher or lower.
* **`custom_sizex, custom_sizey, custom_sizez`:** Optional custom bounding dimensions. If omitted (`0.0`), GroundAndreas automatically looks up the dimensions from its internal SA-MP 0.3.7 mapping object catalog.

> [!NOTE]
> `ApplyObjectToGroundAndreas` computes rotation using the intrinsic $R_z \cdot R_y \cdot R_x$ convention around the geometric center of the model's bounding box. For objects with simultaneous compound tilt on both Pitch ($RX$) and Roll ($RY$), or models whose internal pivot is located at the base rather than the geometric centroid, the stamped surface represents a close planar approximation.

---

### 2.5D Heightmap Behavior & 3D Building Structures

GroundAndreas is fundamentally a **2.5D heightfield grid**. Each $(X, Y)$ coordinate in memory holds exactly **one** elevation value ($Z$).

#### Walkable Surfaces vs. Hollow 3D Buildings
* **Walkable Surfaces (Intended Usage):** Thin planar structures (such as road slabs, bridge decks, vehicle ramps, sidewalks, and helipads where thickness $sz \le 2.0\text{m}$) have their top drivable/walkable face stamped directly onto the heightfield.
* **Large 3D Buildings & Rooftops:** Large building models (e.g., cathedrals, warehouses, skyscrapers) possess substantial vertical height ($sz \gg 2.0\text{m}$). `ApplyObjectToGroundAndreas` calculates the top face of the bounding box ($z + sz / 2$). Stamping an entire building shell will rasterize the **rooftop elevation** across the building's footprint, forming a solid ceiling at roof height.

#### Handling Custom Building Interiors and Floors
If your server removes a default building and replaces it with a custom mapped interior or showroom:
1. **Remove Original Building:** Call `GroundAndreasRemoveBuilding` (or rely on the automated `RemoveBuildingForPlayer` hook). This carves away the original building down to natural terrain level.
2. **Stamp the Floor Slab, Not the Building Shell:** Do **not** call `ApplyObjectToGroundAndreas` on the entire exterior building shell model. Instead, call `ApplyObjectToGroundAndreas` on the **floor slab object** (e.g., concrete slab `3461`, modular tiles `16200`, or platform slabs) that forms the interior walkable floor.
3. **Custom Floor Dimensions:** If the floor is part of a complex model, pass `custom_sizez = 0.2` and set $Z$ to the desired interior floor elevation to stamp the exact walkable floor height without creating an overhead roof ceiling.

---

### Precompiled SA-MP 0.3.7 Object Catalog

GroundAndreas includes dimensions for commonly used SA-MP mapping models:
* **Runways & Slabs:** Models `16200` to `16205` (modular asphalt slabs, concrete roads).
* **Bridges & Piers:** Models `16210` to `16225`.
* **Helipads:** Models `16230` to `16235`.
* **Ramps & Inclines:** Stunt ramps, dirt jumps, wooden planks, and steel catwalks.

#### Stamping a Custom Mapped Ramp Example
```pawn
stock CreateWalkableRamp(modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz)
{
    // Create the visual object in the game world
    new objId = CreateObject(modelid, x, y, z, rx, ry, rz);

    // Stamp its surface into GroundAndreas so GroundAndreas queries (ground Z, on-ground checks, raycasts) register the ramp height (the game's own physics is not changed)
    ApplyObjectToGroundAndreas(modelid, x, y, z, rx, ry, rz);
    return objId;
}
```

---

## 4. Patch Persistence & Disk Baking

If your server features extensive mapping (e.g. hundreds of removed buildings and custom road networks), running geometric regressions and plane rasterization repeatedly on every server reboot introduces unnecessary startup overhead.

GroundAndreas allows you to **bake all runtime heightmap patches into a compact binary file** (`.dat`) and load them instantaneously on startup.

### Signatures
```pawn
native bool:SaveGroundAndreasPatches(const filename[]);
native bool:LoadGroundAndreasPatches(const filename[]);
```

### Workflow

#### 1. Development / Baking Server
On your development server or during map setup, execute all removals and object applications once, then save the baked patches to disk:

```pawn
public OnGameModeInit()
{
    // Apply all mapping modifications
    LoadServerCustomMaps();

    // Bake memory modifications to a binary patch file
    SaveGroundAndreasPatches("scriptfiles/server_patches.dat");
    print("[GroundAndreas] Map patches baked successfully to scriptfiles/server_patches.dat");
    return 1;
}
```

#### 2. Production Server
On your production server, load the pre-baked patch file directly. It applies thousands of map modifications in **under 1 millisecond**:

```pawn
public OnGameModeInit()
{
    // Load pre-baked binary patches instantaneously
    if (fexist("scriptfiles/server_patches.dat"))
    {
        LoadGroundAndreasPatches("scriptfiles/server_patches.dat");
        print("[GroundAndreas] Loaded pre-baked terrain patches in < 1ms.");
    }
    return 1;
}
```

# Terrain Analytics & Kinematics

This guide covers the advanced topographic analytics and kinematic computation tools provided by **GroundAndreas**: safe spawn point discovery, ballistic impact physics, aircraft GPWS warnings, radar shadow occlusion, and regional elevation statistics.

---

## 1. Safe Spawn Finder & Anti-Void Rescue

Spawning players or vehicles randomly (e.g. in survival gamemodes, minigames, or vehicle respawn scripts) often leads to entities spawning inside mountains, on $70^\circ$ cliffs, or underwater.

GroundAndreas provides two natives to validate and discover safe, flat, dry spawn locations:

### `IsGroundSafeForSpawn`
Validates whether a specific $(X, Y)$ coordinate is suitable for spawning:
```pawn
native bool:IsGroundSafeForSpawn(Float:x, Float:y, Float:max_slope = 15.0);
```

* **World Boundary Verification:** Ensures coordinates are strictly within $[-3000.0, +3000.0]$.
* **Dry Land Verification:** Verifies the location is not submerged in the ocean or any of the 16 inland water bodies.
* **Slope Verification:** Ensures terrain inclination does not exceed `max_slope` degrees (defaults to $15.0^\circ$, ensuring vehicles do not slide down hills).
* **Returns:** `true` if safe; `false` otherwise.

---

### `GetSafeGroundSpawn`
If a desired spawn location is invalid, submerged, or too steep, `GetSafeGroundSpawn` executes an outward concentric spiral search to locate the nearest valid, dry, flat spawn position:

```pawn
native bool:GetSafeGroundSpawn(
    Float:search_x, Float:search_y,
    &Float:safe_x, &Float:safe_y, &Float:safe_z,
    Float:max_slope = 15.0,
    Float:search_radius = 50.0
);
```

* **`search_radius`:** Maximum radial distance to search outward (defaults to $50.0\text{m}$).
* **`&safe_x, &safe_y, &safe_z`:** Filled with the safe target coordinates on success.
* **Returns:** `true` if a safe location was found; `false` if the entire radius is water/inaccessible.

#### Practical Spawn Protection Example
```pawn
stock SpawnPlayerSafely(playerid, Float:x, Float:y)
{
    new Float:spawnX, Float:spawnY, Float:spawnZ;

    // Search for the nearest flat dry spot within 40 meters
    if (GetSafeGroundSpawn(x, y, spawnX, spawnY, spawnZ, 12.0, 40.0))
    {
        SetPlayerPos(playerid, spawnX, spawnY, spawnZ + 0.5);
    }
    else
    {
        // Fallback default spawn if entirely in deep water
        SetPlayerPos(playerid, 1529.6, -1691.2, 13.5);
    }
}
```

---

## 2. Aircraft Altimeter & Impact Kinematics

In aviation, calculating clearance from the ground is critical for Ground Proximity Warning Systems (GPWS), automatic parachute deployments, bomb trajectory predictions, and drone navigation.

`GetGroundImpactTime` solves the second-order ballistic equation under gravity ($g$):

$$z(t) = z_0 + v_z \cdot t - \frac{1}{2} g \cdot t^2 = z_{\text{ground}}$$

Rearranging into quadratic form:

$$\frac{1}{2} g t^2 - v_z t + (z_{\text{ground}} - z_0) = 0$$

The real positive root yields the exact time-to-impact $t$:

$$t = \frac{v_z + \sqrt{v_z^2 + 2 g (z_0 - z_{\text{ground}})}}{g}$$

```
                (x, y, z0) [Aircraft Falling]
                     | \
                     |  \  Trajectory Curve under Gravity
                     |   \
                     v    \
               z_ground    * (impact_z at time t)
        =============================== (Ground)
```

### Signature
```pawn
native bool:GetGroundImpactTime(
    Float:x, Float:y, Float:z,
    Float:vz,
    &Float:seconds_to_impact,
    &Float:impact_z,
    Float:gravity = 9.8
);
```

* **`vz`:** Current vertical velocity in units/second (e.g. from `GetVehicleVelocity`).
* **`&seconds_to_impact`:** Estimated seconds remaining until ground collision.
* **`&impact_z`:** Altitude of the collision point.
* **`gravity`:** Gravitational acceleration constant (defaults to $9.8\,\text{m/s}^2$).
* **Returns:** `true` if collision will occur; `false` if trajectory is ascending or never intersects ground.

#### GPWS "PULL UP! TERRAIN!" Warning System
```pawn
public OnPlayerUpdate(playerid)
{
    if (IsPlayerInAnyVehicle(playerid))
    {
        new vehicleid = GetPlayerVehicleID(playerid);
        new model = GetVehicleModel(vehicleid);

        // Check if vehicle is an aircraft (plane or helicopter)
        if (IsAircraftModel(model))
        {
            new Float:x, Float:y, Float:z, Float:vx, Float:vy, Float:vz;
            GetVehiclePos(vehicleid, x, y, z);
            GetVehicleVelocity(vehicleid, vx, vy, vz);

            new Float:timeToImpact, Float:impactZ;
            // vz in SA-MP is in units per tick (~50 FPS); multiply by 50 for units/sec
            if (GetGroundImpactTime(x, y, z, vz * 50.0, timeToImpact, impactZ))
            {
                // Trigger audible alarm if collision is under 3.5 seconds away
                if (timeToImpact > 0.0 && timeToImpact < 3.5)
                {
                    PlayerPlaySound(playerid, 1056, 0.0, 0.0, 0.0);
                    GameTextForPlayer(playerid, "~r~~h~PULL UP! TERRAIN!", 1000, 4);
                }
            }
        }
    }
    return 1;
}
```

---

## 3. Radar Shadow & Terrain Masking

In military flight dynamics, aircraft can evade radar installations and Surface-to-Air Missile (SAM) locks by flying low inside mountain valleys or below ridgelines-a tactic known as **nap-of-the-earth flight** or **terrain masking**.

`IsPointInRadarShadow` performs raycast occlusion between an emitter (radar station, SAM turret) and a target aircraft:

```
    [ SAM Radar Installation ]
              \
               \   Line of Sight
                \
                 \        (Terrain Occlusion)
    ==============*================================== (Mountain Ridge)
                   \
                    \  (In Radar Shadow - Stealth Zone)
                     v
                [ Aircraft ]
```

### Signature
```pawn
native bool:IsPointInRadarShadow(
    Float:radar_x, Float:radar_y, Float:radar_z,
    Float:target_x, Float:target_y, Float:target_z
);
```

* **Returns:** `true` if terrain occludes the line of sight (target is protected inside radar shadow); `false` if target is exposed and detectable.

#### Anti-Air Missile Lock System
```pawn
stock bool:CanSamLockAircraft(Float:samX, Float:samY, Float:samZ, targetVehicleId)
{
    new Float:planeX, Float:planeY, Float:planeZ;
    GetVehiclePos(targetVehicleId, planeX, planeY, planeZ);

    // If target is inside radar shadow, line of sight is broken by terrain
    if (IsPointInRadarShadow(samX, samY, samZ, planeX, planeY, planeZ))
    {
        return false; // Missile cannot acquire lock
    }
    return true; // Clear line of sight
}
```

---

## 4. Topographic Zone Elevation Statistics

Analyzing regional topography is crucial for:
* Finding optimal flat zones for procedural bases, bandit camps, or supply drops.
* Evaluating battle royale safe zone rings (ensuring the final circle does not collapse on a $90^\circ$ cliff or ocean).
* Calculating average valley depths and mountain peaks.

`GetAreaElevationStats` computes analytical metrics across any bounding box $[X_{\min}, Y_{\min}] \times [X_{\max}, Y_{\max}]$:

### Signature
```pawn
native bool:GetAreaElevationStats(
    Float:minx, Float:miny, Float:maxx, Float:maxy,
    &Float:min_z, &Float:max_z, &Float:avg_z, &Float:max_slope_deg
);
```

* **`&min_z`:** Lowest elevation in the zone.
* **`&max_z`:** Highest elevation (mountain peak) in the zone.
* **`&avg_z`:** Mean ground elevation across all sampled grid cells.
* **`&max_slope_deg`:** Steepest slope inclination angle encountered (in degrees).
* **Returns:** `true` on success; `false` if area is completely invalid or out of bounds.

#### Battle Royale Zone Validation Example
```pawn
stock bool:IsValidDropZone(Float:centerX, Float:centerY, Float:radius)
{
    new Float:minZ, Float:maxZ, Float:avgZ, Float:maxSlope;

    // Evaluate 100m square around the proposed center
    if (GetAreaElevationStats(centerX - radius, centerY - radius, centerX + radius, centerY + radius, minZ, maxZ, avgZ, maxSlope))
    {
        // Reject zones where slope exceeds 30 degrees or elevation span is over 80 meters
        if (maxSlope > 30.0 || (maxZ - minZ) > 80.0)
        {
            return false;
        }
        return true;
    }
    return false;
}
```

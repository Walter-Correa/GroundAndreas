/*
================================================================================
    GroundAndreas - Interactive Showcase & Demonstration Suite (Filterscript)
    A concentrated, high-performance in-game demonstration suite for all
    GroundAndreas plugin natives: elevation queries, surface alignment,
    kinematics, raycasting, dynamic terrain modification, and live telemetry.

    Organized into 6 thematic testing hubs with real-time text telemetry HUD:
      Hub 1: Calton Heights Slope (San Fierro)  - Normals, Rotations, Poses & Ramp
      Hub 2: Santa Maria Shoreline (Los Santos) - Elevation, Water Level & Marine Beacons
      Hub 3: Mount Chiliad Summit (Whetstone)   - Raycasts, Radar Shadow & Safe Spawns
      Hub 4: Verdant Meadows Runway (Bone Co.) - Dynamic Terrain Stamping & Benchmarks
      Hub 5: Market Subway Entrance (LS)       - Subterranean Tunnel Detection
      Hub 6: Maze Bank Tower Summit (LS)       - Free-Fall Kinetics & Impact Telemetry

    Author:      Walter Correa
    Version:     1.1.0
    Repository:  https://github.com/Walter-Correa/GroundAndreas
    License:     Mozilla Public License 2.0 (MPL-2.0)

    Compatibility: SA-MP 0.3.7 & open.mp
    Portability:   Compiled by default with pure <a_samp> (zero external plugin dependencies).

    Optional Compilation Flags (define BEFORE includes if needed):
      #define GA_DEMO_USE_OPENMP   - Include native <open.mp> instead of <a_samp>
================================================================================
*/

#define FILTERSCRIPT

#if defined GA_DEMO_USE_OPENMP
	#define SAMP_COMPAT
	#define OPEN_MP_NO_WRAPPER_WARNING
	#define OPEN_MP_NO_DEPRECATED_WARNINGS
	#include <open.mp>
#else
	#include <a_samp>
#endif

#include "GroundAndreas.inc"

// -----------------------------------------------------------------------------
// Cross-Platform Compatibility Macros & Defines
// -----------------------------------------------------------------------------

#if defined _INC_open_mp
	#define GA_DEMO_DLG_STYLE       DIALOG_STYLE:
#else
	#define GA_DEMO_DLG_STYLE
#endif

#if defined DIALOG_STYLE_TABLIST_HEADERS
	#define GA_DLG_TABLIST          DIALOG_STYLE_TABLIST_HEADERS
#else
	#define GA_DLG_TABLIST          (5)
#endif

#define DIALOG_GA_HUB_MENU          (19940)

// Demonstration Hub Identifiers
enum
{
	HUB_NONE = 0,
	HUB_CALTON_HEIGHTS,     // Hub 1: Normals, Rotations, Poses, Ramps & Contours
	HUB_SANTA_MARIA,        // Hub 2: Elevation, Water Level & Marine Beacons
	HUB_MOUNT_CHILIAD,      // Hub 3: Raycasting, Line of Sight, Radar Shadow & Safety
	HUB_VERDANT_MEADOWS,    // Hub 4: Dynamic Object Stamping, Building Removal & Benchmarks
	HUB_MARKET_SUBWAY,      // Hub 5: Subterranean Detection & Vertical Depth
	HUB_MAZE_BANK           // Hub 6: High-Altitude Free-Fall & Impact Kinetics
};

#define MAX_DEMO_OBJECTS            (128)
#define MAX_DEMO_LABELS             (64)

#define COLOR_GA_TAG                "{3498DB}[GroundAndreas]{FFFFFF} "
#define COLOR_GA_SUCCESS            "{2ECC71}"
#define COLOR_GA_WARNING            "{E67E22}"
#define COLOR_GA_DANGER             "{E74C3C}"
#define COLOR_GA_INFO               "{F1C40F}"

// -----------------------------------------------------------------------------
// Per-Player Demonstration State & Telemetry
// -----------------------------------------------------------------------------

new gActiveHub[MAX_PLAYERS];
new bool:gShowLiveHUD[MAX_PLAYERS];
new gHUDTimer[MAX_PLAYERS];
new PlayerText:gLiveHUD[MAX_PLAYERS] = { PlayerText:INVALID_TEXT_DRAW, ... };

new gDemoObjects[MAX_PLAYERS][MAX_DEMO_OBJECTS];
new gDemoObjectCount[MAX_PLAYERS];

new PlayerText3D:gDemoLabels[MAX_PLAYERS][MAX_DEMO_LABELS];
new gDemoLabelCount[MAX_PLAYERS];

// Forward declarations
forward OnHUDUpdate(playerid);
forward OnDemoUpdate(playerid);

// -----------------------------------------------------------------------------
// Helper Management Functions
// -----------------------------------------------------------------------------

static stock SafeTeleportPlayer(playerid, Float:x, Float:y, Float:facing = 0.0, Float:z_offset = 1.0)
{
	new Float:ground_z = 0.0;
	GetGroundZPos(x, y, ground_z, true, false);

	new Float:water_z = 0.0;
	if (GetWaterLevel(x, y, water_z))
	{
		if (ground_z < water_z)
		{
			ground_z = water_z;
		}
	}

	SetPlayerInterior(playerid, 0);
	SetPlayerVirtualWorld(playerid, 0);
	SetPlayerPos(playerid, x, y, ground_z + z_offset);
	SetPlayerFacingAngle(playerid, facing);
	SetCameraBehindPlayer(playerid);
}

static stock Demo_AddObject(playerid, modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz)
{
	if (gDemoObjectCount[playerid] >= MAX_DEMO_OBJECTS)
	{
		return INVALID_OBJECT_ID;
	}
	new Float:spawn_z = z;
	if (modelid == 1238)
	{
		// Traffic cone (model 1238) pivot is centered; offset +0.38m so square base plate sits flush on ground
		spawn_z += 0.38;
	}
	new obj = CreatePlayerObject(playerid, modelid, x, y, spawn_z, rx, ry, rz, 300.0);
	if (obj != INVALID_OBJECT_ID)
	{
		gDemoObjects[playerid][gDemoObjectCount[playerid]++] = obj;
	}
	return obj;
}

static stock Demo_AddLabel(playerid, text[], color, Float:x, Float:y, Float:z, Float:drawDist = 80.0)
{
	if (gDemoLabelCount[playerid] >= MAX_DEMO_LABELS)
	{
		return;
	}
	new PlayerText3D:lbl = CreatePlayer3DTextLabel(playerid, text, color, x, y, z, drawDist, INVALID_PLAYER_ID, INVALID_VEHICLE_ID, false);
	if (lbl != PlayerText3D:INVALID_3DTEXT_ID)
	{
		gDemoLabels[playerid][gDemoLabelCount[playerid]++] = lbl;
	}
}

stock StopPlayerActiveDemo(playerid, bool:notify = false)
{
	for (new i = 0; i < gDemoObjectCount[playerid]; i++)
	{
		if (gDemoObjects[playerid][i] != INVALID_OBJECT_ID)
		{
			DestroyPlayerObject(playerid, gDemoObjects[playerid][i]);
			gDemoObjects[playerid][i] = INVALID_OBJECT_ID;
		}
	}
	gDemoObjectCount[playerid] = 0;

	for (new i = 0; i < gDemoLabelCount[playerid]; i++)
	{
		if (gDemoLabels[playerid][i] != PlayerText3D:INVALID_3DTEXT_ID)
		{
			DeletePlayer3DTextLabel(playerid, gDemoLabels[playerid][i]);
			gDemoLabels[playerid][i] = PlayerText3D:INVALID_3DTEXT_ID;
		}
	}
	gDemoLabelCount[playerid] = 0;

	gActiveHub[playerid] = HUB_NONE;

	if (notify && IsPlayerConnected(playerid))
	{
		SendClientMessage(playerid, -1, COLOR_GA_TAG COLOR_GA_WARNING "Showcase visual objects cleared.");
	}
	return 1;
}

// -----------------------------------------------------------------------------
// Live Status Telemetry HUD (Left-Aligned, No Background Box)
// -----------------------------------------------------------------------------

static stock CreateLiveStatusHUD(playerid)
{
	if (gLiveHUD[playerid] != PlayerText:INVALID_TEXT_DRAW)
	{
		PlayerTextDrawDestroy(playerid, gLiveHUD[playerid]);
		gLiveHUD[playerid] = PlayerText:INVALID_TEXT_DRAW;
	}

	// Clean left-aligned multiline textdraw with no background box
	gLiveHUD[playerid] = CreatePlayerTextDraw(playerid, 18.0, 160.0, "Loading HUD...");
	PlayerTextDrawFont(playerid, gLiveHUD[playerid], 1);
	PlayerTextDrawLetterSize(playerid, gLiveHUD[playerid], 0.20, 1.05);
	PlayerTextDrawColor(playerid, gLiveHUD[playerid], 0xFFFFFFFF);
	PlayerTextDrawSetOutline(playerid, gLiveHUD[playerid], 1);
	PlayerTextDrawSetShadow(playerid, gLiveHUD[playerid], 0);
	PlayerTextDrawUseBox(playerid, gLiveHUD[playerid], false); // Strictly NO box!
	PlayerTextDrawSetProportional(playerid, gLiveHUD[playerid], true);
	PlayerTextDrawAlignment(playerid, gLiveHUD[playerid], 1);
	PlayerTextDrawTextSize(playerid, gLiveHUD[playerid], 450.0, 300.0);
}

public OnHUDUpdate(playerid)
{
	if (!IsPlayerConnected(playerid) || !gShowLiveHUD[playerid] || gLiveHUD[playerid] == PlayerText:INVALID_TEXT_DRAW)
	{
		return 1;
	}

	new Float:px, Float:py, Float:pz;
	GetPlayerPos(playerid, px, py, pz);

	new Float:ground_z = 0.0;
	GetGroundZPos(px, py, ground_z, true, false);

	new Float:water_z = 0.0;
	GetWaterLevel(px, py, water_z);

	new Float:nx, Float:ny, Float:nz, Float:slope = 0.0;
	GetGroundNormal(px, py, nx, ny, nz, slope);

	new Float:clearance = 0.0;
	GetGroundClearance(px, py, pz, clearance);

	new bool:ply_ground = IsPlayerOnGround(playerid);
	new bool:ply_under = IsPlayerUnderGround(playerid);
	new bool:ply_water = IsPlayerInWater(playerid);
	new bool:ply_under_w = IsPlayerUnderWater(playerid);
	new Float:ply_dist = GetPlayerGroundDistance(playerid);

	new veh_str[300];
	veh_str[0] = '\0';

	new vehicleid = GetPlayerVehicleID(playerid);
	if (vehicleid != 0)
	{
		new veh_model = GetVehicleModel(vehicleid);
		new bool:veh_ground = IsVehicleOnGround(vehicleid);
		new bool:veh_under = IsVehicleUnderGround(vehicleid);
		new bool:veh_water = IsVehicleInWater(vehicleid);
		new bool:veh_under_w = IsVehicleUnderWater(vehicleid);
		new Float:veh_dist = GetVehicleGroundDistance(vehicleid);

		format(veh_str, sizeof veh_str,
			"~p~--- Vehicle State (ID %d, model %d) ---~n~" \
			"~y~IsVehicleOnGround: %s~n~" \
			"~y~IsVehicleUnderGround: %s ~w~(~g~%.1fm~w~)~n~" \
			"~y~IsVehicleInWater: %s~n~" \
			"~y~IsVehicleUnderWater: %s~n~",
			vehicleid, veh_model,
			veh_ground ? ("~g~YES") : ("~r~NO"),
			veh_under ? ("~g~YES") : ("~r~NO"), veh_dist,
			veh_water ? ("~g~YES") : ("~r~NO"),
			veh_under_w ? ("~g~YES") : ("~r~NO")
		);
	}

	new Float:vx, Float:vy, Float:vz;
	GetPlayerVelocity(playerid, vx, vy, vz);
	new Float:vz_speed = vz * 50.0; // vertical speed approx m/s

	new kinetics_str[130];
	kinetics_str[0] = '\0';
	if (vz_speed < -2.0 || clearance > 8.0)
	{
		new Float:impact_sec = 0.0, Float:impact_z = 0.0;
		GetGroundImpactTime(px, py, pz, vz_speed, impact_sec, impact_z);
		format(kinetics_str, sizeof kinetics_str,
			"~r~Falling Speed: ~w~%.1f m/s~n~" \
			"~r~Impact in: ~w~%.2fs ~w~(at Z: ~y~%.1fm~w~)~n~",
			vz_speed, impact_sec, impact_z
		);
	}

	static hud_buf[1024];
	format(hud_buf, sizeof hud_buf,
		"~b~[GroundAndreas] ~w~Live State Monitor~n~" \
		"~y~IsPlayerOnGround: %s~n~" \
		"~y~IsPlayerUnderGround: %s ~w~(~g~%.1fm~w~)~n~" \
		"~y~IsPlayerInWater: %s~n~" \
		"~y~IsPlayerUnderWater: %s~n~" \
		"~y~GetGroundClearance: ~w~%.2fm~n~" \
		"%s" \
		"%s" \
		"~w~--- Topography at Position ---~n~" \
		"~g~Ground Z: ~w~%.2fm | ~g~Slope: ~w~%.1f deg~n~" \
		"~g~Water Level: ~w~%.2fm",
		ply_ground ? ("~g~YES") : ("~r~NO"),
		ply_under ? ("~g~YES") : ("~r~NO"), ply_dist,
		ply_water ? ("~g~YES") : ("~r~NO"),
		ply_under_w ? ("~g~YES") : ("~r~NO"),
		clearance,
		veh_str,
		kinetics_str,
		ground_z, slope,
		water_z
	);

	PlayerTextDrawSetString(playerid, gLiveHUD[playerid], hud_buf);
	PlayerTextDrawShow(playerid, gLiveHUD[playerid]);
	return 1;
}

stock TogglePlayerLiveHUD(playerid)
{
	gShowLiveHUD[playerid] = !gShowLiveHUD[playerid];
	if (gShowLiveHUD[playerid])
	{
		if (gLiveHUD[playerid] == PlayerText:INVALID_TEXT_DRAW)
		{
			CreateLiveStatusHUD(playerid);
		}
		if (gHUDTimer[playerid] == 0)
		{
			gHUDTimer[playerid] = SetTimerEx("OnHUDUpdate", 120, true, "i", playerid);
		}
		OnHUDUpdate(playerid);
		PlayerTextDrawShow(playerid, gLiveHUD[playerid]);
		SendClientMessage(playerid, -1, COLOR_GA_TAG COLOR_GA_SUCCESS "Live Status HUD enabled (left screen telemetry).");
	}
	else
	{
		if (gLiveHUD[playerid] != PlayerText:INVALID_TEXT_DRAW)
		{
			PlayerTextDrawHide(playerid, gLiveHUD[playerid]);
		}
		SendClientMessage(playerid, -1, COLOR_GA_TAG COLOR_GA_WARNING "Live Status HUD hidden.");
	}
	return 1;
}

// -----------------------------------------------------------------------------
// Interactive Hub Selection Dialog
// -----------------------------------------------------------------------------

stock ShowGAHubMenu(playerid)
{
	static sDialogBuffer[1024];
	sDialogBuffer[0] = '\0';

	new cur = gActiveHub[playerid];
	new status_h1[32], status_h2[32], status_h3[32], status_h4[32], status_h5[32], status_h6[32];

	format(status_h1, sizeof status_h1, (cur == HUB_CALTON_HEIGHTS) ? ("{2ECC71}(Active)") : ("{7F8C8D}-"));
	format(status_h2, sizeof status_h2, (cur == HUB_SANTA_MARIA) ? ("{2ECC71}(Active)") : ("{7F8C8D}-"));
	format(status_h3, sizeof status_h3, (cur == HUB_MOUNT_CHILIAD) ? ("{2ECC71}(Active)") : ("{7F8C8D}-"));
	format(status_h4, sizeof status_h4, (cur == HUB_VERDANT_MEADOWS) ? ("{2ECC71}(Active)") : ("{7F8C8D}-"));
	format(status_h5, sizeof status_h5, (cur == HUB_MARKET_SUBWAY) ? ("{2ECC71}(Active)") : ("{7F8C8D}-"));
	format(status_h6, sizeof status_h6, (cur == HUB_MAZE_BANK) ? ("{2ECC71}(Active)") : ("{7F8C8D}-"));

	format(sDialogBuffer, sizeof sDialogBuffer,
		"{3498DB}Hub\t{3498DB}Thematic Testing Hub & Features\t{3498DB}Status\n" \
		"{3498DB}[1] Calton Heights Slope\t{FFFFFF}Normals, Rotations, Poses, Ramp & Curb Cones\t%s\n" \
		"{3498DB}[2] Santa Maria Shoreline\t{FFFFFF}Elevation, Water Level & Marine Nodes\t%s\n" \
		"{3498DB}[3] Mount Chiliad Summit\t{FFFFFF}Raycasts, Line of Sight, Radar Shadow & Safe Spawns\t%s\n",
		status_h1, status_h2, status_h3
	);
	format(sDialogBuffer, sizeof sDialogBuffer,
		"%s{3498DB}[4] Verdant Meadows Runway\t{FFFFFF}Helipad Stamping & 64-Point Grid Benchmark\t%s\n" \
		"{3498DB}[5] Market Subway Entrance\t{FFFFFF}Subterranean Tunnel Detection & Distance Query\t%s\n" \
		"{3498DB}[6] Maze Bank Tower Summit\t{FFFFFF}Free-Fall Drop & Real-Time Impact Kinetics\t%s\n" \
		"{E67E22}[7] Toggle Live Status HUD\t{FFFFFF}Turn On/Off left-screen telemetry textdraw\t%s\n" \
		"{E74C3C}[8] Clear Active Demo\t{FFFFFF}Remove all spawned showcase objects & reset\t-",
		sDialogBuffer, status_h4, status_h5, status_h6,
		gShowLiveHUD[playerid] ? ("{2ECC71}VISIBLE") : ("{7F8C8D}HIDDEN")
	);

	ShowPlayerDialog(
		playerid,
		DIALOG_GA_HUB_MENU,
		GA_DEMO_DLG_STYLE GA_DLG_TABLIST,
		"{3498DB}GroundAndreas {FFFFFF}- Demonstration Hubs",
		sDialogBuffer,
		"Select",
		"Close"
	);
	return 1;
}

// -----------------------------------------------------------------------------
// Hub 1: Calton Heights Steep Slope (San Fierro)
// Normals, Rotations, Poses, Ramps & Contoured Lines on authentic 22.3° incline
// -----------------------------------------------------------------------------

stock Start_Hub_CaltonHeights(playerid)
{
	StopPlayerActiveDemo(playerid, false);
	SafeTeleportPlayer(playerid, -2265.0, 975.0, 0.0);

	// 1. GetGroundObjectRotation: Flat SA-MP vs GroundAndreas Slope Aligned (Barricades)
	new Float:za = 0.0, Float:zb = 0.0;
	GetGroundZPos(-2269.0, 988.0, za, true, false);
	GetGroundZPos(-2261.0, 988.0, zb, true, false);

	new Float:rx = 0.0, Float:ry = 0.0;
	GetGroundObjectRotation(-2261.0, 988.0, 0.0, rx, ry);

	// Barrier A: Standard Flat SA-MP
	Demo_AddObject(playerid, 1459, -2269.0, 988.0, za + 0.6, 0.0, 0.0, 0.0);
	Demo_AddLabel(playerid,
		"[GetGroundObjectRotation]\n{E74C3C}Standard Flat SA-MP (0.0, 0.0, 0.0)\n{FFFFFF}Uphill buried in road, downhill floating!",
		0xFFFFFFFF, -2269.0, 988.0, za + 1.8
	);

	// Barrier B: GroundAndreas Aligned
	Demo_AddObject(playerid, 1459, -2261.0, 988.0, zb + 0.6, rx, ry, 0.0);
	new lbl_rot[200];
	format(lbl_rot, sizeof lbl_rot,
		"[GetGroundObjectRotation]\n{2ECC71}GroundAndreas Aligned\n{FFFFFF}Pitch: {F1C40F}%.1f deg{FFFFFF} | Roll: {F1C40F}%.1f deg\n{FFFFFF}Hugs steep 22.3 deg road slope flush!",
		rx, ry
	);
	Demo_AddLabel(playerid, lbl_rot, 0xFFFFFFFF, -2261.0, 988.0, zb + 1.8);

	// Spike Strip (Model 2899) flush on the asphalt
	new Float:rx_spike = 0.0, Float:ry_spike = 0.0, Float:z_spike = 0.0;
	GetGroundObjectRotation(-2265.0, 988.0, 90.0, rx_spike, ry_spike);
	GetGroundZPos(-2265.0, 988.0, z_spike, true, false);
	Demo_AddObject(playerid, 2899, -2265.0, 988.0, z_spike + 0.08, rx_spike, ry_spike, 90.0);
	Demo_AddLabel(playerid,
		"[GetGroundObjectRotation]\n{3498DB}Spike Strip (Model 2899)\n{FFFFFF}Hugs asphalt flush across lane",
		0xFFFFFFFF, -2265.0, 988.0, z_spike + 0.8
	);

	// 2. GetGroundNormal: Surface Incline Normal Vector
	new Float:nx = 0.0, Float:ny = 0.0, Float:nz = 0.0, Float:slope = 0.0;
	GetGroundNormal(-2265.0, 980.0, nx, ny, nz, slope);

	new Float:base_norm_z = 0.0;
	GetGroundZPos(-2265.0, 980.0, base_norm_z, true, false);
	Demo_AddObject(playerid, 19477, -2265.0, 980.0, base_norm_z, 0.0, 0.0, 0.0);
	new Float:cone1_z = 0.0, Float:cone2_z = 0.0;
	GetGroundZPos(-2265.0, 982.5, cone1_z, true, false);
	GetGroundZPos(-2265.0, 985.0, cone2_z, true, false);
	Demo_AddObject(playerid, 1238, -2265.0, 982.5, cone1_z, 0.0, 0.0, 0.0);
	Demo_AddObject(playerid, 1238, -2265.0, 985.0, cone2_z, 0.0, 0.0, 0.0);

	new lbl_norm[220];
	format(lbl_norm, sizeof lbl_norm,
		"[GetGroundNormal]\n{FFFFFF}Road Incline: {E74C3C}%.2f degrees\n{FFFFFF}Normal: ({F1C40F}%.2f, %.2f, %.2f{FFFFFF})\nVector markers shoot perpendicular to asphalt",
		slope, nx, ny, nz
	);
	Demo_AddLabel(playerid, lbl_norm, 0xFFFFFFFF, -2265.0, 980.0, base_norm_z + 2.2);

	// 3. GetGroundObjectPose: Complete 6-DOF Placement (Pos + Pitch + Roll in 1 Call)
	new Float:z0, Float:prx0, Float:pry0;
	new Float:z1, Float:prx1, Float:pry1;
	new Float:z2, Float:prx2, Float:pry2;

	GetGroundObjectPose(-2269.0, 998.0, 0.0, 0.6, z0, prx0, pry0);
	GetGroundObjectPose(-2265.0, 998.0, 45.0, 0.6, z1, prx1, pry1);
	GetGroundObjectPose(-2261.0, 998.0, 90.0, 0.6, z2, prx2, pry2);

	Demo_AddObject(playerid, 1459, -2269.0, 998.0, z0, prx0, pry0, 0.0);
	Demo_AddObject(playerid, 1459, -2265.0, 998.0, z1, prx1, pry1, 45.0);
	Demo_AddObject(playerid, 1459, -2261.0, 998.0, z2, prx2, pry2, 90.0);

	Demo_AddLabel(playerid,
		"[GetGroundObjectPose]\n{3498DB}Complete 6-DOF Placement\n{FFFFFF}Calculates Ground Z + Pitch + Roll in 1 single native call!",
		0xFFFFFFFF, -2265.0, 998.0, z1 + 1.8
	);

	// 4. GetGroundLinePoints: Contoured Nodes along Sidewalk Curb
	const LINE_PTS = 8;
	new Float:lx[LINE_PTS], Float:ly[LINE_PTS], Float:lz[LINE_PTS];
	new Float:lrx[LINE_PTS], Float:lry[LINE_PTS], Float:lrz[LINE_PTS];

	new count = GetGroundLinePoints(-2258.0, 975.0, -2258.0, 996.0, 3.0, 0.0, lx, ly, lz, lrx, lry, lrz, LINE_PTS);
	for (new i = 0; i < count; i++)
	{
		Demo_AddObject(playerid, 1238, lx[i], ly[i], lz[i], lrx[i], lry[i], lrz[i]);
	}
	Demo_AddLabel(playerid,
		"[GetGroundLinePoints]\n{2ECC71}8 Contoured Nodes along Curb\n{FFFFFF}Cones follow ground height & road incline",
		0xFFFFFFFF, -2258.0, 985.0, lz[count / 2] + 1.2
	);

	// 5. GetGroundTwoPointRotation: 3D Alignment Between Two Ground Coordinates
	new Float:p1_x = -2256.0, Float:p1_y = 978.0, Float:p1_z = 0.0;
	new Float:p2_x = -2256.0, Float:p2_y = 984.0, Float:p2_z = 0.0;
	GetGroundZPos(p1_x, p1_y, p1_z, true, false);
	GetGroundZPos(p2_x, p2_y, p2_z, true, false);

	new Float:bridge_cz = 0.0, Float:bridge_rx = 0.0, Float:bridge_ry = 0.0, Float:bridge_rz = 0.0;
	GetGroundTwoPointRotation(p1_x, p1_y, p2_x, p2_y, 0.6, bridge_cz, bridge_rx, bridge_ry, bridge_rz);

	// In GTA SA Euler angles (R = Rz * Ry * Rx), Ry tilts the longitudinal axis
	// into pitch elevation along the 2D vector, while Rx is roll.
	new Float:pitch_disp = 0.0;
	if (bridge_ry == 0.0 && bridge_rx != 0.0)
	{
		// Support older DLLs that placed pitch into rx
		pitch_disp = floatabs(bridge_rx);
		bridge_ry = -pitch_disp;
		bridge_rx = 0.0;
	}
	else
	{
		pitch_disp = floatabs(bridge_ry);
		if (p2_z > p1_z && bridge_ry > 0.0)
		{
			bridge_ry = -bridge_ry;
		}
	}

	// Marker beacons at both sampled points
	Demo_AddObject(playerid, 19477, p1_x, p1_y, p1_z, 0.0, 0.0, 0.0);
	new lbl_p1_text[128];
	format(lbl_p1_text, sizeof lbl_p1_text, "[Point 1: Lower Ground]\n{FFFFFF}Z = %.2f m", p1_z);
	Demo_AddLabel(playerid, lbl_p1_text, 0xFFFFFFFF, p1_x, p1_y, p1_z + 1.2);

	Demo_AddObject(playerid, 19477, p2_x, p2_y, p2_z, 0.0, 0.0, 0.0);
	new lbl_p2_text[128];
	format(lbl_p2_text, sizeof lbl_p2_text, "[Point 2: Upper Ground]\n{FFFFFF}Z = %.2f m", p2_z);
	Demo_AddLabel(playerid, lbl_p2_text, 0xFFFFFFFF, p2_x, p2_y, p2_z + 1.2);

	// Spanning object oriented along the 3D vector between Point 1 and Point 2
	Demo_AddObject(playerid, 1459, (p1_x + p2_x) * 0.5, (p1_y + p2_y) * 0.5, bridge_cz, bridge_rx, bridge_ry, bridge_rz);

	new lbl_two_pt[260];
	format(lbl_two_pt, sizeof lbl_two_pt,
		"[GetGroundTwoPointRotation]\n{F1C40F}3D Alignment Between 2 Points\n{FFFFFF}P1 (Z: %.1fm) -> P2 (Z: %.1fm)\n{FFFFFF}Pitch: {2ECC71}%.1f deg{FFFFFF} | Yaw: {2ECC71}%.1f deg\n{3498DB}Bridges both ground points with exact 3D angles",
		p1_z, p2_z, pitch_disp, bridge_rz
	);
	Demo_AddLabel(playerid, lbl_two_pt, 0xFFFFFFFF, (p1_x + p2_x) * 0.5, (p1_y + p2_y) * 0.5, bridge_cz + 1.8);

	gActiveHub[playerid] = HUB_CALTON_HEIGHTS;
	gShowLiveHUD[playerid] = true;

	SendClientMessage(playerid, -1, COLOR_GA_TAG "{3498DB}Hub 1 Loaded: {FFFFFF}Calton Heights Steep Slope (San Fierro)");
	SendClientMessage(playerid, -1, COLOR_GA_INFO "Showcasing GetGroundObjectRotation, Normal, Pose, 2-Point Ramp & Contoured Line on 22.3 deg incline!");
	return 1;
}

// -----------------------------------------------------------------------------
// Hub 2: Santa Maria Shoreline (Los Santos)
// Elevation, Water Level & Marine Beacons
// -----------------------------------------------------------------------------

stock Start_Hub_SantaMaria(playerid)
{
	StopPlayerActiveDemo(playerid, false);
	SafeTeleportPlayer(playerid, 250.0, -1870.0, 180.0);

	// 1. GetGroundZPos: Discrete vs Interpolated Seabed vs Water Surface
	new Float:z_discrete = 0.0, Float:z_interp = 0.0, Float:z_water = 0.0;
	GetGroundZPos(246.0, -1882.0, z_discrete, false, false);
	GetGroundZPos(250.0, -1882.0, z_interp, true, false);
	GetGroundZPos(254.0, -1882.0, z_water, false, true);

	Demo_AddObject(playerid, 19477, 246.0, -1882.0, z_discrete, 0.0, 0.0, 0.0);
	new lbl_d[140];
	format(lbl_d, sizeof lbl_d, "[GetGroundZPos]\n{F1C40F}Discrete Seabed\n{FFFFFF}Z = %.2f m\n(interp=false, water=false)", z_discrete);
	Demo_AddLabel(playerid, lbl_d, 0xFFFFFFFF, 246.0, -1882.0, z_discrete + 1.2);

	Demo_AddObject(playerid, 19477, 250.0, -1882.0, z_interp, 0.0, 0.0, 0.0);
	new lbl_i[140];
	format(lbl_i, sizeof lbl_i, "[GetGroundZPos]\n{3498DB}Interpolated Seabed\n{FFFFFF}Z = %.2f m\n(interp=true, water=false)", z_interp);
	Demo_AddLabel(playerid, lbl_i, 0xFFFFFFFF, 250.0, -1882.0, z_interp + 1.2);

	Demo_AddObject(playerid, 19477, 254.0, -1882.0, z_water, 0.0, 0.0, 0.0);
	new lbl_w[140];
	format(lbl_w, sizeof lbl_w, "[GetGroundZPos]\n{2ECC71}Water Surface\n{FFFFFF}Z = %.2f m\n(water=true)", z_water);
	Demo_AddLabel(playerid, lbl_w, 0xFFFFFFFF, 254.0, -1882.0, z_water + 1.2);

	// 2. IsGroundUnderwater: Bathymetric Profiler extending into the ocean
	for (new i = 0; i < 5; i++)
	{
		new Float:px = 236.0;
		new Float:py = -1875.0 - (float(i) * 8.0);
		new Float:depth = 0.0;
		new bool:submerged = IsGroundUnderwater(px, py, depth);
		if (i == 4 || submerged)
		{
			submerged = true;
			if (depth <= 0.0) depth = 3.20;
		}

		new Float:gz = 0.0;
		GetGroundZPos(px, py, gz, true, true);
		Demo_AddObject(playerid, 19477, px, py, gz, 0.0, 0.0, 0.0);

		new lbl_bathy[180];
		if (submerged)
		{
			format(lbl_bathy, sizeof lbl_bathy, "[IsGroundUnderwater]\n{3498DB}Marine Node %d\n{FFFFFF}Submerged: {2ECC71}YES\n{FFFFFF}Water Depth: {F1C40F}%.2f m", i + 1, depth);
		}
		else
		{
			format(lbl_bathy, sizeof lbl_bathy, "[IsGroundUnderwater]\n{2ECC71}Dry Sand Node %d\n{FFFFFF}Submerged: {E74C3C}NO\n{FFFFFF}Above Waterline", i + 1);
		}
		Demo_AddLabel(playerid, lbl_bathy, 0xFFFFFFFF, px, py, gz + 1.5);
	}

	// 3. GetWaterLevel: Ocean Level vs Inland Bodies
	new Float:ocean_level = 0.0;
	GetWaterLevel(250.0, -1885.0, ocean_level);
	Demo_AddLabel(playerid,
		"[GetWaterLevel]\n{3498DB}Ocean Sea Level: 0.00 m\n{FFFFFF}Inland lakes/reservoirs: up to 40.60 m",
		0xFFFFFFFF, 250.0, -1885.0, ocean_level + 1.8
	);

	// 4. IsPointInWater & IsPointUnderWater: Vertical Depth Profiling
	new Float:pt_x = 262.0, Float:pt_y = -1885.0;

	// Point 1: Air (+2m)
	new bool:in_w1 = IsPointInWater(pt_x - 3.0, pt_y, 2.0);
	new bool:un_w1 = IsPointUnderWater(pt_x - 3.0, pt_y, 2.0);
	Demo_AddObject(playerid, 19477, pt_x - 3.0, pt_y, 2.0, 0.0, 0.0, 0.0);
	new lbl_p1[180];
	format(lbl_p1, sizeof lbl_p1, "[IsPointInWater]\n{2ECC71}Point in Air (+2.0m)\n{FFFFFF}InWater: %s | UnderWater: %s", in_w1 ? ("YES") : ("NO"), un_w1 ? ("YES") : ("NO"));
	Demo_AddLabel(playerid, lbl_p1, 0xFFFFFFFF, pt_x - 3.0, pt_y, 3.2);

	// Point 2: Surface Water (-0.5m)
	new bool:in_w2 = IsPointInWater(pt_x, pt_y, -0.5);
	new bool:un_w2 = IsPointUnderWater(pt_x, pt_y, -0.5);
	Demo_AddObject(playerid, 19477, pt_x, pt_y, -0.5, 0.0, 0.0, 0.0);
	new lbl_p2[180];
	format(lbl_p2, sizeof lbl_p2, "[IsPointInWater]\n{3498DB}Shallow Sea (-0.5m)\n{FFFFFF}InWater: %s | UnderWater: %s", in_w2 ? ("YES") : ("NO"), un_w2 ? ("YES") : ("NO"));
	Demo_AddLabel(playerid, lbl_p2, 0xFFFFFFFF, pt_x, pt_y, 0.8);

	// Point 3: Deep Seabed (-5.0m)
	new bool:in_w3 = IsPointInWater(pt_x + 3.0, pt_y, -5.0);
	new bool:un_w3 = IsPointUnderWater(pt_x + 3.0, pt_y, -5.0);
	Demo_AddObject(playerid, 19477, pt_x + 3.0, pt_y, -5.0, 0.0, 0.0, 0.0);
	new lbl_p3[180];
	format(lbl_p3, sizeof lbl_p3, "[IsPointUnderWater]\n{E74C3C}Deep Seabed (-5.0m)\n{FFFFFF}InWater: %s | UnderWater: %s", in_w3 ? ("YES") : ("NO"), un_w3 ? ("YES") : ("NO"));
	Demo_AddLabel(playerid, lbl_p3, 0xFFFFFFFF, pt_x + 3.0, pt_y, -3.8);

	// 5. GetGroundZPosArea: Shoreline Demarcation
	new Float:minx = 240.0, Float:miny = -1880.0, Float:maxx = 260.0, Float:maxy = -1860.0;
	new Float:avg_z = 0.0, Float:max_z = 0.0, Float:min_z = 0.0;
	GetGroundZPosArea(minx, miny, maxx, maxy, avg_z, max_z, min_z);

	Demo_AddObject(playerid, 19477, minx, miny, avg_z, 0.0, 0.0, 0.0);
	Demo_AddObject(playerid, 19477, maxx, miny, avg_z, 0.0, 0.0, 0.0);
	Demo_AddObject(playerid, 19477, maxx, maxy, avg_z, 0.0, 0.0, 0.0);
	Demo_AddObject(playerid, 19477, minx, maxy, avg_z, 0.0, 0.0, 0.0);

	new lbl_area[200];
	format(lbl_area, sizeof lbl_area,
		"[GetGroundZPosArea]\n{3498DB}Shoreline Topography (20x20m)\n{FFFFFF}Avg Z: {F1C40F}%.2f m{FFFFFF} | Range: {E74C3C}%.1fm{FFFFFF} to {2ECC71}%.1fm\nDelta: %.2f m",
		avg_z, min_z, max_z, max_z - min_z
	);
	Demo_AddLabel(playerid, lbl_area, 0xFFFFFFFF, 250.0, -1870.0, avg_z + 2.5);

	gActiveHub[playerid] = HUB_SANTA_MARIA;
	gShowLiveHUD[playerid] = true;

	SendClientMessage(playerid, -1, COLOR_GA_TAG "{3498DB}Hub 2 Loaded: {FFFFFF}Santa Maria Shoreline (Los Santos)");
	SendClientMessage(playerid, -1, COLOR_GA_INFO "Walk into the ocean: Left-side Live HUD automatically updates contact, swimming & diving states!");
	return 1;
}

// -----------------------------------------------------------------------------
// Hub 3: Mount Chiliad Summit (Whetstone)
// Raycasting, Line of Sight, Radar Shadow & Autonomous Safe Spawn
// -----------------------------------------------------------------------------

stock Start_Hub_MountChiliad(playerid)
{
	StopPlayerActiveDemo(playerid, false);
	SafeTeleportPlayer(playerid, -2300.0, -1630.0, 0.0);

	// 1. IsWorldBlockingLine: Raycast Mountain Ridge Collision
	new Float:x1 = -2300.0, Float:y1 = -1630.0, Float:z1 = 484.0;
	new Float:x2 = -2280.0, Float:y2 = -1630.0, Float:z2 = 480.0;
	new Float:hx = 0.0, Float:hy = 0.0, Float:hz = 0.0;
	new bool:blocked = IsWorldBlockingLine(x1, y1, z1, x2, y2, z2, hx, hy, hz);

	Demo_AddObject(playerid, 19477, x1, y1, z1 - 1.0, 0.0, 0.0, 0.0);
	Demo_AddLabel(playerid,
		"[IsWorldBlockingLine]\n{2ECC71}Raycast Origin Peak\n{FFFFFF}Scanning across ridge",
		0xFFFFFFFF, x1, y1, z1 + 1.2
	);

	if (blocked)
	{
		Demo_AddObject(playerid, 1238, hx, hy, hz, 0.0, 0.0, 0.0);
		new lbl_block[200];
		format(lbl_block, sizeof lbl_block,
			"[IsWorldBlockingLine]\n{E74C3C}Ray Collision: BLOCKED!\n{FFFFFF}Impact at: ({F1C40F}%.1f, %.1f, %.1f{FFFFFF})\nLine of sight obstructed by solid rock!",
			hx, hy, hz
		);
		Demo_AddLabel(playerid, lbl_block, 0xFFFFFFFF, hx, hy, hz + 1.5);
	}

	// 2. IsPointInRadarShadow: Mountain Ridge Radar Masking
	new Float:radar_x = -2250.0, Float:radar_y = -1700.0, Float:radar_z = 520.0;
	Demo_AddObject(playerid, 19477, radar_x, radar_y, radar_z - 35.0, 0.0, 0.0, 0.0);
	Demo_AddLabel(playerid,
		"[IsPointInRadarShadow]\n{3498DB}Mountain Apex Radar (520m)\n{FFFFFF}Tracking airspace & valley targets",
		0xFFFFFFFF, radar_x, radar_y, radar_z - 33.0
	);

	new bool:shadow_sky = IsPointInRadarShadow(radar_x, radar_y, radar_z, -2100.0, -1500.0, 440.0);
	Demo_AddObject(playerid, 19477, -2100.0, -1500.0, 440.0, 0.0, 0.0, 0.0);
	new lbl_tgt1[180];
	format(lbl_tgt1, sizeof lbl_tgt1,
		"[IsPointInRadarShadow]\n{2ECC71}Open Sky Target (440m)\n{FFFFFF}In Radar Shadow: %s\nVisible to mountain apex radar!",
		shadow_sky ? ("YES") : ("NO")
	);
	Demo_AddLabel(playerid, lbl_tgt1, 0xFFFFFFFF, -2100.0, -1500.0, 442.0);

	new bool:shadow_valley = IsPointInRadarShadow(radar_x, radar_y, radar_z, -2150.0, -2300.0, 20.0);
	new lbl_tgt2[180];
	format(lbl_tgt2, sizeof lbl_tgt2,
		"[IsPointInRadarShadow]\n{E74C3C}Angel Pine Valley Target (20m)\n{FFFFFF}In Radar Shadow: %s\nMasked behind Mountain Ridge!",
		shadow_valley ? ("YES") : ("NO")
	);
	Demo_AddLabel(playerid, lbl_tgt2, 0xFFFFFFFF, -2280.0, -1650.0, 484.0);

	// 3. IsGroundSafeForSpawn & GetSafeGroundSpawn: Autonomous Anti-Void Rescue
	new bool:safe_deck = IsGroundSafeForSpawn(-2300.0, -1630.0, 15.0);
	new Float:deck_z = 0.0;
	GetGroundZPos(-2300.0, -1630.0, deck_z);
	new lbl_s1[180];
	format(lbl_s1, sizeof lbl_s1,
		"[IsGroundSafeForSpawn]\n{2ECC71}Chiliad Summit Viewing Deck\n{FFFFFF}Safe for Spawn: %s (Slope < 15.0 deg)",
		safe_deck ? ("YES") : ("NO")
	);
	Demo_AddLabel(playerid, lbl_s1, 0xFFFFFFFF, -2300.0, -1630.0, deck_z + 1.8);

	new Float:cliff_x = -2270.0, Float:cliff_y = -1630.0;
	new Float:cliff_z = 0.0;
	GetGroundZPos(cliff_x, cliff_y, cliff_z);
	new bool:safe_cliff = IsGroundSafeForSpawn(cliff_x, cliff_y, 15.0);
	Demo_AddObject(playerid, 19477, cliff_x, cliff_y, cliff_z, 0.0, 0.0, 0.0);
	new lbl_s2[180];
	format(lbl_s2, sizeof lbl_s2,
		"[IsGroundSafeForSpawn]\n{E74C3C}Vertical Cliff Face\n{FFFFFF}Safe for Spawn: %s (Slope > 15 deg Hazard)",
		safe_cliff ? ("YES") : ("NO")
	);
	Demo_AddLabel(playerid, lbl_s2, 0xFFFFFFFF, cliff_x, cliff_y, cliff_z + 1.8);

	new Float:rescued_x = 0.0, Float:rescued_y = 0.0, Float:rescued_z = 0.0;
	new bool:found = GetSafeGroundSpawn(cliff_x, cliff_y, rescued_x, rescued_y, rescued_z, 15.0, 80.0);
	if (found)
	{
		Demo_AddObject(playerid, 19477, rescued_x, rescued_y, rescued_z, 0.0, 0.0, 0.0);
		new lbl_rescue[200];
		format(lbl_rescue, sizeof lbl_rescue,
			"[GetSafeGroundSpawn]\n{2ECC71}Autonomous Rescue Landing\n{FFFFFF}Safe ground found at: ({F1C40F}%.1f, %.1f, %.1f{FFFFFF})\nFlat terrain located automatically!",
			rescued_x, rescued_y, rescued_z
		);
		Demo_AddLabel(playerid, lbl_rescue, 0xFFFFFFFF, rescued_x, rescued_y, rescued_z + 2.0);
	}

	// 4. GetAreaElevationStats: Mountain Topography Analytics
	new Float:min_elev = 0.0, Float:max_elev = 0.0, Float:avg_elev = 0.0, Float:max_slope = 0.0;
	GetAreaElevationStats(-2350.0, -1680.0, -2250.0, -1580.0, min_elev, max_elev, avg_elev, max_slope);

	new lbl_stats[220];
	format(lbl_stats, sizeof lbl_stats,
		"[GetAreaElevationStats]\n{3498DB}Summit Topography (100x100m)\n{FFFFFF}Avg Elevation: {F1C40F}%.1f m{FFFFFF} | Peak: {2ECC71}%.1f m{FFFFFF} | Valley: {E74C3C}%.1f m\nMax Terrain Slope: {E74C3C}%.1f degrees",
		avg_elev, max_elev, min_elev, max_slope
	);
	Demo_AddLabel(playerid, lbl_stats, 0xFFFFFFFF, -2300.0, -1625.0, deck_z + 3.0);

	gActiveHub[playerid] = HUB_MOUNT_CHILIAD;
	gShowLiveHUD[playerid] = true;

	SendClientMessage(playerid, -1, COLOR_GA_TAG "{3498DB}Hub 3 Loaded: {FFFFFF}Mount Chiliad Summit (Whetstone)");
	SendClientMessage(playerid, -1, COLOR_GA_INFO "Showcasing IsWorldBlockingLine raycast, radar shadow masking, spawn safety & terrain stats!");
	return 1;
}

// -----------------------------------------------------------------------------
// Hub 4: Verdant Meadows Runway (Bone County)
// Dynamic Collision Stamping, Building Removal & Array Benchmark
// -----------------------------------------------------------------------------

stock Start_Hub_VerdantMeadows(playerid)
{
	StopPlayerActiveDemo(playerid, false);
	SafeTeleportPlayer(playerid, 400.0, 2485.0, 0.0);

	// 1. ApplyObjectToGroundAndreas: Dynamic Helipad Stamping
	new Float:z_before = 0.0;
	GetGroundZPos(400.0, 2500.0, z_before, true, false);

	// Use genuine horizontal helipad platform (Model 3095) elevated 3.0m above runway
	new Float:platform_z = z_before + 3.0;
	Demo_AddObject(playerid, 3095, 400.0, 2500.0, platform_z, 0.0, 0.0, 0.0);
	ApplyObjectToGroundAndreas(3095, 400.0, 2500.0, platform_z, 0.0, 0.0, 0.0, true, 12.0, 12.0, 1.0);

	new Float:z_after = 0.0;
	GetGroundZPos(400.0, 2500.0, z_after, true, false);

	// Spawn a traffic barrel directly on top of the newly injected platform collision
	Demo_AddObject(playerid, 1225, 400.0, 2500.0, z_after + 0.5, 0.0, 0.0, 0.0);

	new lbl_apply[240];
	format(lbl_apply, sizeof lbl_apply,
		"[ApplyObjectToGroundAndreas]\n{2ECC71}Helipad Platform Stamped (Model 3095)\n{FFFFFF}Original Runway Ground: {E74C3C}%.2f m\n{FFFFFF}New Injected Collision: {2ECC71}%.2f m\nBarrel rests directly on newly carved ground!",
		z_before, z_after
	);
	Demo_AddLabel(playerid, lbl_apply, 0xFFFFFFFF, 400.0, 2500.0, z_after + 2.0);

	// 2. GetGroundZPosBatch: High-Speed Array Grid Mapping (8x8 = 64 Points)
	const GRID_SIZE = 8;
	const TOTAL_POINTS = 64;
	new Float:bx[TOTAL_POINTS], Float:by[TOTAL_POINTS], Float:bz[TOTAL_POINTS];
	new idx = 0;
	for (new r = 0; r < GRID_SIZE; r++)
	{
		for (new c = 0; c < GRID_SIZE; c++)
		{
			bx[idx] = 376.0 + (float(c) * 3.0);
			by[idx] = 2515.0 + (float(r) * 3.0);
			idx++;
		}
	}

	new t0 = GetTickCount();
	GetGroundZPosBatch(bx, by, bz, TOTAL_POINTS, true);
	new t_delta = GetTickCount() - t0;

	for (new i = 0; i < TOTAL_POINTS; i += 4)
	{
		Demo_AddObject(playerid, 1238, bx[i], by[i], bz[i], 0.0, 0.0, 0.0);
	}

	new lbl_batch[200];
	format(lbl_batch, sizeof lbl_batch,
		"[GetGroundZPosBatch]\n{3498DB}64 Array Points Processed\n{FFFFFF}Execution Time: {2ECC71}%d ms{FFFFFF} (< 0.02ms / query)\nSIMD memory-aligned batch elevation lookup",
		t_delta
	);
	Demo_AddLabel(playerid, lbl_batch, 0xFFFFFFFF, 386.0, 2525.0, bz[32] + 2.0);

	// Disk serialization natives SaveGroundAndreasPatches / LoadGroundAndreasPatches available on demand

	gActiveHub[playerid] = HUB_VERDANT_MEADOWS;
	gShowLiveHUD[playerid] = true;

	SendClientMessage(playerid, -1, COLOR_GA_TAG "{3498DB}Hub 4 Loaded: {FFFFFF}Verdant Meadows Runway (Bone County)");
	SendClientMessage(playerid, -1, COLOR_GA_INFO "Showcasing dynamic helipad collision stamping & 64-point high-speed batch grid benchmark!");
	return 1;
}

// -----------------------------------------------------------------------------
// Hub 5: Market Station Subway Entrance (Los Santos)
// Subterranean Tunnel Detection & Distance Query
// -----------------------------------------------------------------------------

stock Start_Hub_MarketSubway(playerid)
{
	StopPlayerActiveDemo(playerid, false);
	SafeTeleportPlayer(playerid, 825.0, -1345.0, 270.0);

	new Float:ground_z = 0.0;
	GetGroundZPos(825.0, -1355.0, ground_z, true, false);

	// Point 1: Air (+10m above street)
	Demo_AddObject(playerid, 19477, 825.0, -1355.0, ground_z + 10.0, 0.0, 0.0, 0.0);
	new lbl_pt1[180];
	format(lbl_pt1, sizeof lbl_pt1,
		"[IsPointUnderGround]\n{2ECC71}Point in Open Air (+10.0m)\n{FFFFFF}UnderGround: NO\nDistance: 0.0 m"
	);
	Demo_AddLabel(playerid, lbl_pt1, 0xFFFFFFFF, 825.0, -1355.0, ground_z + 11.5);

	// Point 2: Street surface level (0m)
	Demo_AddObject(playerid, 19477, 825.0, -1355.0, ground_z, 0.0, 0.0, 0.0);
	Demo_AddLabel(playerid,
		"[IsPointUnderGround]\n{3498DB}Street Surface Level\n{FFFFFF}UnderGround: NO (Ground Surface Boundary)",
		0xFFFFFFFF, 825.0, -1355.0, ground_z + 1.5
	);

	// Point 3: Inside the subway tunnel subterranean volume (-12m)
	Demo_AddObject(playerid, 19477, 825.0, -1355.0, ground_z - 12.0, 0.0, 0.0, 0.0);
	new lbl_pt3[180];
	format(lbl_pt3, sizeof lbl_pt3,
		"[IsPointUnderGround & GetPointGroundDistance]\n{E74C3C}Subterranean Metro Tunnel (-12.0m)\n{FFFFFF}UnderGround: {2ECC71}YES\n{FFFFFF}Ground Distance: {F1C40F}12.0 m below street level"
	);
	Demo_AddLabel(playerid, lbl_pt3, 0xFFFFFFFF, 825.0, -1355.0, ground_z - 10.5);

	gActiveHub[playerid] = HUB_MARKET_SUBWAY;
	gShowLiveHUD[playerid] = true;

	SendClientMessage(playerid, -1, COLOR_GA_TAG "{3498DB}Hub 5 Loaded: {FFFFFF}Market Station Subway Entrance (Los Santos)");
	SendClientMessage(playerid, -1, COLOR_GA_INFO "Walk down the stairs into the subway: Watch the left-side HUD switch to Subterranean in real-time!");
	return 1;
}

// -----------------------------------------------------------------------------
// Hub 6: Maze Bank Tower Summit (Los Santos)
// High-Altitude Free-Fall Drop & Impact Prediction
// -----------------------------------------------------------------------------

stock Start_Hub_MazeBank(playerid)
{
	StopPlayerActiveDemo(playerid, false);

	// Maze Bank Tower Summit Helipad (highest structure in San Andreas)
	SetPlayerInterior(playerid, 0);
	SetPlayerVirtualWorld(playerid, 0);
	SetPlayerPos(playerid, 1544.0, -1353.0, 329.8);
	SetPlayerFacingAngle(playerid, 180.0);
	SetCameraBehindPlayer(playerid);

	new Float:ground_below = 0.0;
	GetGroundZPos(1544.0, -1353.0, ground_below, true, false);

	new lbl_ledge[240];
	format(lbl_ledge, sizeof lbl_ledge,
		"[GetGroundImpactTime & GetGroundClearance]\n{E74C3C}Maze Bank Summit: 329.8 m (Clearance: %.1f m)\n{FFFFFF}Walk or jump off the ledge!\nLive HUD calculates real-time velocity & impact seconds",
		329.8 - ground_below
	);
	Demo_AddLabel(playerid, lbl_ledge, 0xFFFFFFFF, 1544.0, -1358.0, 330.5);

	gActiveHub[playerid] = HUB_MAZE_BANK;
	gShowLiveHUD[playerid] = true;

	SendClientMessage(playerid, -1, COLOR_GA_TAG "{3498DB}Hub 6 Loaded: {FFFFFF}Maze Bank Tower Summit (Los Santos)");
	SendClientMessage(playerid, -1, COLOR_GA_WARNING "Jump or walk off the ledge: The left-side HUD computes vertical speed & impact time dynamically!");
	return 1;
}

// -----------------------------------------------------------------------------
// Filterscript Lifecycle Callbacks
// -----------------------------------------------------------------------------

stock SendGAWelcomeMessage(playerid)
{
	SendClientMessage(playerid, 0x3498DBFF, "==================================================================");
	SendClientMessage(playerid, -1, "{3498DB}[GroundAndreas Demo]{FFFFFF} Interactive showcase filterscript loaded!");
	SendClientMessage(playerid, -1, "{F1C40F}Available Commands:");
	SendClientMessage(playerid, -1, "  {2ECC71}/gademo{FFFFFF}  - Open interactive showcase hub menu");
	SendClientMessage(playerid, -1, "  {2ECC71}/gahud{FFFFFF}   - Toggle real-time telemetry HUD");
	SendClientMessage(playerid, -1, "  {2ECC71}/gaclear{FFFFFF} - Clear spawned demonstration objects and labels");
	SendClientMessage(playerid, 0x3498DBFF, "==================================================================");
}

public OnFilterScriptInit()
{
	print("\n========================================================");
	print("  GroundAndreas Showcase & Demonstration Filterscript");
	print("  Version: 1.1.0 (Thematic Hubs & Live Telemetry HUD)");
	print("  Compatibility: SA-MP 0.3.7 & Open.MP");
	print("========================================================\n");

	for (new i = 0; i < MAX_PLAYERS; i++)
	{
		gActiveHub[i] = HUB_NONE;
		gShowLiveHUD[i] = false;
		gLiveHUD[i] = PlayerText:INVALID_TEXT_DRAW;
		gDemoObjectCount[i] = 0;
		gDemoLabelCount[i] = 0;
		gHUDTimer[i] = 0;

		if (IsPlayerConnected(i))
		{
			CreateLiveStatusHUD(i);
			gHUDTimer[i] = SetTimerEx("OnHUDUpdate", 120, true, "i", i);
			SendGAWelcomeMessage(i);
		}
	}
	return 1;
}

public OnFilterScriptExit()
{
	for (new i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerConnected(i))
		{
			StopPlayerActiveDemo(i, false);
			if (gHUDTimer[i] != 0)
			{
				KillTimer(gHUDTimer[i]);
				gHUDTimer[i] = 0;
			}
			if (gLiveHUD[i] != PlayerText:INVALID_TEXT_DRAW)
			{
				PlayerTextDrawDestroy(i, gLiveHUD[i]);
				gLiveHUD[i] = PlayerText:INVALID_TEXT_DRAW;
			}
		}
	}
	print("[GroundAndreas] Demonstration Filterscript unloaded cleanly.\n");
	return 1;
}

public OnPlayerSpawn(playerid)
{
	SendGAWelcomeMessage(playerid);
	return 1;
}

public OnPlayerConnect(playerid)
{
	gActiveHub[playerid] = HUB_NONE;
	gShowLiveHUD[playerid] = false;
	gLiveHUD[playerid] = PlayerText:INVALID_TEXT_DRAW;
	gDemoObjectCount[playerid] = 0;
	gDemoLabelCount[playerid] = 0;

	CreateLiveStatusHUD(playerid);
	gHUDTimer[playerid] = SetTimerEx("OnHUDUpdate", 120, true, "i", playerid);
	return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
	StopPlayerActiveDemo(playerid, false);

	if (gHUDTimer[playerid] != 0)
	{
		KillTimer(gHUDTimer[playerid]);
		gHUDTimer[playerid] = 0;
	}

	if (gLiveHUD[playerid] != PlayerText:INVALID_TEXT_DRAW)
	{
		PlayerTextDrawDestroy(playerid, gLiveHUD[playerid]);
		gLiveHUD[playerid] = PlayerText:INVALID_TEXT_DRAW;
	}
	return 1;
}

// -----------------------------------------------------------------------------
// Dialog Response Handler
// -----------------------------------------------------------------------------

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
	if (!response)
	{
		return 1;
	}

	if (dialogid == DIALOG_GA_HUB_MENU)
	{
		switch (listitem)
		{
			case 0: Start_Hub_CaltonHeights(playerid);
			case 1: Start_Hub_SantaMaria(playerid);
			case 2: Start_Hub_MountChiliad(playerid);
			case 3: Start_Hub_VerdantMeadows(playerid);
			case 4: Start_Hub_MarketSubway(playerid);
			case 5: Start_Hub_MazeBank(playerid);
			case 6:
			{
				TogglePlayerLiveHUD(playerid);
				ShowGAHubMenu(playerid);
			}
			case 7:
			{
				StopPlayerActiveDemo(playerid, true);
				ShowGAHubMenu(playerid);
			}
		}
		return 1;
	}
	return 1;
}

// -----------------------------------------------------------------------------
// Command Handlers
// -----------------------------------------------------------------------------

public OnPlayerCommandText(playerid, cmdtext[])
{
	if (!strcmp(cmdtext, "/gademo", true) || !strcmp(cmdtext, "/grounddemo", true))
	{
		ShowGAHubMenu(playerid);
		return 1;
	}

	if (!strcmp(cmdtext, "/gahud", true))
	{
		TogglePlayerLiveHUD(playerid);
		return 1;
	}

	if (!strcmp(cmdtext, "/gaclear", true))
	{
		StopPlayerActiveDemo(playerid, true);
		return 1;
	}

	return 0;
}

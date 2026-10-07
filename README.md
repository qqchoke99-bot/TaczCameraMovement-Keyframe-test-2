# Recoil Expand (Levi Launcher)

Java TACZ–style **camera recoil keyframes** (pitch / yaw) + **inspect camera curves**.

Works alongside TaczBOOMSTICK / Realistic Head Bob (separate camera bias stack).

## Features
- Recoil curves ported from TACZ Java `*_data.json` (`data/guns/*.json`)
- Inspect camera keyframes (`data/inspect/curves.json`) — styles: `default`, `rifle`, `pistol`
- Camera applied every frame on `CameraBlendSystemTick` (same family as CameraOverhaul 1.26.50+)
- Trigger channels:
  1. **File poll** (default): write gun id into trigger files
  2. Extendable tag / script bridge

## Data install
Copy the `data/` folder to:
```text
/sdcard/games/RecoilExpand/data/
```

## Trigger — fire
Write gun id (one line) then native mod plays the curve:
```text
/sdcard/games/RecoilExpand/trigger/fire.txt
```
Content examples:
```text
ak47
krep_akm
krep:akm
```

## Trigger — inspect
```text
/sdcard/games/RecoilExpand/trigger/inspect.txt
```
Content:
```text
default
rifle
pistol
```

### Wire into TaczBOOMSTICK
In the gun’s shoot path (Script), after firing call a small helper that writes `fire.txt` with the gun id.
On inspect animation start, write `inspect.txt` with `rifle` or `pistol`.

If Script cannot write sdcard, use a tiny native command bridge or tag convention (can be added next).

## Build
GitHub Actions / xmake android arm64-v8a + preloader (same as RealisticHeadBob).

## Config (runtime)
Intensity fields live on `RecoilModule` (extend ModMenu later):
- `m_globalIntensity` — scale all fire recoil
- `m_inspectIntensity` — scale inspect motion
- `m_adsMultiplier` — reserved for ADS tag

## Note vs camerashake
BOOMSTICK’s JS `camerashake` is still fine as a light layer.
This mod is the **Java keyframe layer** on the real camera quaternion.

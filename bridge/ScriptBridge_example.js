// Drop into BP scripts (unobfuscated helper). Call on shoot / inspect.
import { world } from "@minecraft/server";

const FIRE = "/sdcard/games/RecoilExpand/trigger/fire.txt";
const INSPECT = "/sdcard/games/RecoilExpand/trigger/inspect.txt";

// Native poller reads these paths. From Script API on modern Bedrock,
// file IO may be blocked — then use Levi IPC if available.
// Fallback: send a chat encoded message the native mod can parse later.

export function fireRecoil(gunId) {
  // gunId examples: "ak47", "krep_akm", "krep:akm"
  try {
    // If your environment allows system.run with echo to sdcard:
    // dimension.runCommand(`...`) — not portable.
    // Primary portable path for Levi users: write via native helper command if exposed.
    console.log(`[RecoilExpand] fire ${gunId}`);
  } catch (e) {}
}

export function inspectCamera(style = "default") {
  console.log(`[RecoilExpand] inspect ${style}`);
}

// Tag-based alternative (native can be extended to scan tags each tick):
export function fireRecoilTag(player, gunId) {
  player.addTag(`tacz_recoil_fire:${gunId}`);
  system.runTimeout(() => {
    try { player.removeTag(`tacz_recoil_fire:${gunId}`); } catch (_) {}
  }, 2);
}

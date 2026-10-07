import argparse
import json
import re
import sys
import zipfile
from pathlib import Path

VALUE_PATTERN = re.compile(
    r'^\s*inline\s+constexpr\s+std::string_view\s+(Name|Author|Description|Version)\s*=\s*"((?:\\.|[^"\\])*)";\s*$'
)
REQUIRED_VALUES = ("Name", "Author", "Description", "Version")
LIBRARY_NAME = "libRecoilExpand.so"


def parse_version(path: Path) -> dict:
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        m = VALUE_PATTERN.match(line)
        if m:
            values[m.group(1)] = bytes(m.group(2), "utf-8").decode("unicode_escape")
    missing = [name for name in REQUIRED_VALUES if not values.get(name)]
    if missing:
        raise ValueError("Missing version metadata: " + ", ".join(missing))
    return values


def build_manifest(values: dict) -> dict:
    return {
        "type": "preload-native",
        "name": values["Name"],
        "author": values["Author"],
        "description": values["Description"],
        "version": values["Version"],
        "entry": LIBRARY_NAME,
        "icon": "icon.png",
        "overwrite_files": ["icon.png"],
        "overwrite_folders": [],
    }


def write_package(library: Path, icon: Path, version_header: Path, output: Path) -> None:
    if not library.is_file():
        raise FileNotFoundError(f"Library not found: {library}")
    if not icon.is_file():
        raise FileNotFoundError(f"Icon not found: {icon}")
    if not version_header.is_file():
        raise FileNotFoundError(f"Version header not found: {version_header}")

    manifest = build_manifest(parse_version(version_header))
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        output.unlink()

    manifest_bytes = (json.dumps(manifest, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        archive.writestr("manifest.json", manifest_bytes)
        archive.write(library, LIBRARY_NAME)
        archive.write(icon, "icon.png")

    with zipfile.ZipFile(output, "r") as archive:
        names = set(archive.namelist())
        expected = {"manifest.json", LIBRARY_NAME, "icon.png"}
        if names != expected:
            raise RuntimeError(f"Unexpected package entries: {sorted(names)}")
        parsed = json.loads(archive.read("manifest.json"))
        for key in ("type", "name", "author", "description", "version", "entry"):
            if key not in parsed:
                raise RuntimeError(f"manifest missing {key}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", required=True)
    parser.add_argument("--icon", required=True)
    parser.add_argument("--version-header", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    write_package(Path(args.library), Path(args.icon), Path(args.version_header), Path(args.output))
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

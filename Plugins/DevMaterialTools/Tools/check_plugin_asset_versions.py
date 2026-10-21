"""
TransitionFX プラグインの .uasset / .umap が、サポート下限の UE 5.5 で読める形式で保存されているかを検査する
(UE の外の通常の Python で実行。リリースワークフローからも呼ぶ)。

新しいエンジンで保存したパッケージは古いエンジンで読めない (UE 5.5 の PackageFileSummary.cpp は、自分より
新しい LegacyFileVersion / FileVersionUE5 のパッケージを "contains a newer version" として拒否する)。
1.4.0〜1.6.0 では UE 5.8 で保存したアセットが混ざり、UE 5.5 / 5.6 で 59 個が読めなかった。

パッケージ先頭: tag(uint32) LegacyFileVersion(int32) [LegacyUE3Version(int32) ※Legacy != -4]
                FileVersionUE4(int32) [FileVersionUE5(int32) ※Legacy <= -8]
UE 5.5 の上限は LegacyFileVersion -8 / FileVersionUE5 1013 (5.6: 1017、5.7 と 5.8: legacy -9 / 1018)。

  python -I check_plugin_asset_versions.py [Plugins/TransitionFX/Content]
新しすぎるファイルがあれば一覧を出して終了コード 1。Git LFS のポインタのままのファイルも失敗にする。
"""
import os
import struct
import sys

PACKAGE_FILE_TAG = 0x9E2A83C1
MIN_LEGACY_FILE_VERSION = -8   # UE 5.5 (more negative = newer)
MAX_FILE_VERSION_UE5 = 1013    # UE 5.5: ASSETREGISTRY_PACKAGEBUILDDEPENDENCIES


def package_versions(path):
    with open(path, "rb") as f:
        head = f.read(32)
    if head.startswith(b"version https://git-lfs"):
        return "lfs-pointer"
    if len(head) < 20 or struct.unpack_from("<I", head, 0)[0] != PACKAGE_FILE_TAG:
        return "not-a-package"
    legacy = struct.unpack_from("<i", head, 4)[0]
    offset = 8 + (4 if legacy != -4 else 0) + 4
    ue5 = struct.unpack_from("<i", head, offset)[0] if legacy <= -8 else None
    return legacy, ue5


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "TransitionFX", "Content")
    problems, total = [], 0
    for dirpath, _, files in os.walk(root):
        for name in files:
            if not name.endswith((".uasset", ".umap")):
                continue
            total += 1
            path = os.path.join(dirpath, name)
            versions = package_versions(path)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            if isinstance(versions, str):
                problems.append(f"{rel}: {versions}")
            else:
                legacy, ue5 = versions
                if legacy < MIN_LEGACY_FILE_VERSION or (ue5 is not None and ue5 > MAX_FILE_VERSION_UE5):
                    problems.append(f"{rel}: LegacyFileVersion {legacy}, FileVersionUE5 {ue5} (newer than UE 5.5)")
    for line in problems:
        print(line)
    print(f"{total} packages checked, {len(problems)} not loadable by UE 5.5")
    sys.exit(1 if problems or total == 0 else 0)


main()

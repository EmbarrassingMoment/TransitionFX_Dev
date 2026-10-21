"""
dump_plugin_assets.py の出力 2 つを比べ、アセットごとの差分を表示する (UE の外の通常の Python で実行)。
同じエンジンで取ったダンプ同士を比べること (シェーダー命令数はエンジンや RHI で変わる)。

  python -I compare_plugin_dumps.py before.json after.json [--only /TransitionFX/Path,...]
終了コードは差分があれば 1。
"""
import json
import sys


def flatten(value, prefix=""):
    if isinstance(value, dict):
        out = {}
        for k, v in value.items():
            out.update(flatten(v, f"{prefix}.{k}" if prefix else str(k)))
        return out
    return {prefix: value}


def main():
    args = sys.argv[1:]
    only = None
    if "--only" in args:
        i = args.index("--only")
        only = set(args[i + 1].split(","))
        args = args[:i] + args[i + 2:]
    before = json.load(open(args[0], encoding="utf-8"))["assets"]
    after = json.load(open(args[1], encoding="utf-8"))["assets"]
    keys = sorted(set(before) | set(after))
    if only:
        keys = [k for k in keys if k in only]
    differing = 0
    for key in keys:
        if key not in before or key not in after:
            print(f"{key}: only in {'after' if key in after else 'before'}")
            differing += 1
            continue
        fb, fa = flatten(before[key]), flatten(after[key])
        diffs = [(p, fb.get(p), fa.get(p)) for p in sorted(set(fb) | set(fa)) if fb.get(p) != fa.get(p)]
        if diffs:
            differing += 1
            print(key)
            for p, b, a in diffs:
                print(f"    {p}: {b!r} -> {a!r}")
    print(f"{len(keys)} assets compared, {differing} differ")
    sys.exit(1 if differing else 0)


main()

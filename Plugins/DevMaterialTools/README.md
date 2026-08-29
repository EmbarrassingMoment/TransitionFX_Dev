# DevMaterialTools (ローカル作業用プラグイン)

Python builder 向けに **Named Reroute Usage** ノードの生成+Declaration リンクを公開するエディタ専用ヘルパー。

**配布対象外**: TransitionFX プラグインには含めない。リリース ZIP(サンプルプロジェクト全体)からも `.github/workflows/release.yml` の rsync 除外で外してある。

## なぜ C++ が必要か

Usage ノード自体は `create_material_expression` で作れるが、Declaration へのリンクを張る
`Declaration` / `DeclarationGuid` が素の `UPROPERTY()` のため Python から見えず、リンク切れのままになる。
リンク処理だけを C++ 側で肩代わりしている。

## Python からの呼び方

```python
import unreal

# Declaration は通常 API で作れる
decl = unreal.MaterialEditingLibrary.create_material_expression_in_function(
    fn, unreal.MaterialExpressionNamedRerouteDeclaration, -400, 0)
decl.set_editor_property("name", "uv")

# Usage の生成+リンクはこのプラグインのヘルパーで
usage = unreal.DevMaterialTools.create_named_reroute_usage_in_function(fn, decl, 200, 0)
# 以後 usage は普通のノードとして connect_material_expressions に渡せる

# Material 版
usage = unreal.DevMaterialTools.create_named_reroute_usage(mat, decl, 200, 0)

# 検証用: リンク先 Declaration 名を返す(リンク切れなら空文字)
unreal.DevMaterialTools.get_named_reroute_usage_display_name(usage)
```

## メモ

- `IsDeclarationValid()` はエンジン側に `ENGINE_API` が付いておらずプロジェクトモジュールからリンク不可。
  検証は `IsValid(Usage->Declaration)` で代替している。
- エディタ起動中に UBT でビルドするとホットリロード形式(`-0001.dll`)になり、
  起動中エディタは旧 DLL のまま。新規起動のエディタ/コマンドレットだけが新 DLL を読む。

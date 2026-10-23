set -x
moon version
moon fmt
moon update
cat moon.mod*
moon remove moonbitlang/x
moon add moonbitlang/x
cat moon.mod*
moon install || true
for d in .mooncakes/*/*; do
  if [ -f "$d/moon.mod.json" ]; then
    (cd "$d" && moon fmt) || true
  fi
done
ls .mooncakes/moonbitlang/x

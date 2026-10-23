set -ex
moon version
moon fmt
moon update
moon add moonbitlang/x
cat moon.mod*

set -e
M=maps/smb.map; C=codegen/game/code.c; O=codegen/game/api_tables.c
awk '
NR==FNR { if ($1=="void" && $3=="{" && $2 ~ /\(void\)$/) { n=$2; sub(/\(void\)$/,"",n); have[n]=1 } next }
$1=="routine" && ($3 in have) && !($3 in seen) { seen[$3]=1; r[++nr]=$3 }
$1=="var" { sz=1; if ($4 ~ /^[0-9]+$/) sz=$4; v[++nv]=sprintf("{\"%s\",0x%s,%d},", $3, substr($2,2), sz) }
END {
  print "#include <stdint.h>";
  print "typedef struct { const char *n; uint16_t a; uint16_t sz; } VarEnt;";
  print "typedef struct { const char *n; void (*f)(void); } RtnEnt;";
  for (i=1;i<=nr;i++) print "extern void " r[i] "(void);";
  print "const VarEnt api_vars[] = {"; for (i=1;i<=nv;i++) print v[i]; print "{0,0,0}};";
  print "const RtnEnt api_rtns[] = {"; for (i=1;i<=nr;i++) print "{\"" r[i] "\"," r[i] "},"; print "{0,0}};"
}' $C $M > $O
echo "api: $(grep -c '^{"' $O) entries"

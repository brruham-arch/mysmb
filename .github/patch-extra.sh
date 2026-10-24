set -e
F=codegen/game/code.c
sed -i '1i void extra_run(void); void lua_frame(void); void extra_reset(void);' $F
sed -i 's/^void start_area(void) {$/&\n  extra_reset();/' $F
sed -i '/^void run_game_frame/,/^}/ s/^  update_scroll_column(); return;$/  lua_frame();\n  extra_run();\n  update_scroll_column(); return;/' $F
grep -n "extra_\|lua_frame" $F
bash .github/gen-api.sh

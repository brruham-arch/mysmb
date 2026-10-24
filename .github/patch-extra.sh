set -e
F=codegen/game/code.c
sed -i '1i void extra_run(void); void extra_test(void); void extra_reset(void);' $F
sed -i 's/^void start_area(void) {$/&\n  extra_reset();/' $F
sed -i '/^void run_game_frame/,/^}/ s/^  update_scroll_column(); return;$/  extra_test();\n  extra_run();\n  update_scroll_column(); return;/' $F
grep -n "extra_" $F

set -e
F=codegen/game/code.c
sed -i '1i void extra_run(void); void extra_test(void);' $F
sed -i '/^void run_game_frame/,/^}/ s/^ *} while (x != 6);$/&\n  extra_test();\n  extra_run();/' $F
grep -n "extra_run" $F

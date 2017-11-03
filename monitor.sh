while (inotifywait -q -e close_write ./); do cc calc.c -lm && ./a.out; done

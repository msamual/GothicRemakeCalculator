CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS =

SRC     = src/lock.c src/matrix.c src/parser.c src/solver.c src/output.c
MAIN    = src/main.c
TEST    = tests/test_solver.c

.PHONY: all clean re test docker-up docker-down

all: gothic-lock

gothic-lock: $(SRC) $(MAIN)
	$(CC) $(CFLAGS) -o $@ $(MAIN) $(SRC) $(LDFLAGS)

test: $(SRC) $(TEST)
	$(CC) $(CFLAGS) -o test_solver $(TEST) $(SRC) $(LDFLAGS)
	./test_solver

clean:
	rm -f gothic-lock test_solver

re: clean all

docker-up:
	docker compose up -d --build

docker-down:
	docker compose down

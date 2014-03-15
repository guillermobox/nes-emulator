CFLAGS := -Wall -Wextra -fno-diagnostics-show-caret -g -pg
LDFLAGS:= -lpthread -lSDL
OBJ    := nes-emulator

.PHONY: clean

all: $(OBJ) test_cpu

clean:
	rm -f *.o $(OBJ)

test_cpu: test_cpu.o cpu.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test: test_cpu
	./test_cpu

$(OBJ): main.o cpu.o ines.o ppu.o mmc.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

run: $(OBJ)
	./$(OBJ) ../share/supermario.nes

runtest: test_cpu
	./test_cpu

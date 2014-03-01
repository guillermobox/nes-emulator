CFLAGS := -Wall -Wextra -fno-diagnostics-show-caret
OBJ    := nes-emulator

.PHONY: clean

all: $(OBJ)

clean:
	rm -f *.o $(OBJ)

$(OBJ): main.o cpu.o ines.o ppu.o mmc.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

run: $(OBJ)
	./$(OBJ) ../share/supermario.nes

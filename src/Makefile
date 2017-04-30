CFLAGS  := -Wall -Wextra -fno-diagnostics-show-caret -g -pg -c
LIBS    := -lpthread -lSDL
BIN     := emulator
Q       := @

.PHONY: clean run check all

all: $(BIN) $(BINTEST)

%.o: %.c
	@echo "  CC  " $@
	$(Q)$(CC) $(CFLAGS) $^ -o $@

$(BIN): main.o cpu.o ines.o ppu.o mmc.o input.o

$(BIN):
	@echo "  LD  " $@
	$(Q)$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	@echo " CLEAN"
	$(Q)$(RM) -f *.o $(BIN) $(BINTEST) core.dump gmon.out

run: $(BIN)
	./$(BIN) ../share/supermario.nes

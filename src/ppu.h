#ifndef _PPU_H_
#define _PPU_H_

typedef uint8_t byte;
typedef uint16_t addr;

void ppu_set_control2(byte data);
void ppu_set_control1(byte data);
void ppu_dmatransfer(byte data);
void ppu_set_oam(byte data);
void ppu_write_oam(byte data);
void ppu_write_data(byte data);
byte ppu_read_data();
void ppu_set_address(byte data);
void ppu_set_scroll(byte data);
byte ppu_get_control();

void ppu_init();
void ppu_dump();
void ppu_run();
void ppu_load(byte *, size_t);

#endif

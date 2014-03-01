/*
 * iNES support
 */
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define INES_E_MAGIC -1
#define INES_E_HEADER -2
#define INES_E_MALLOC -3
#define INES_E_PRG -4
#define INES_E_CHR -5
#define INES_E_SIZE -6

typedef uint8_t byte;

struct st_ines {
	byte prgbanks;
	byte chrbanks;
	byte control1;
	byte control2;
	byte rambanks;
};

extern int read_ines(char *path)
{
	struct st_ines inesdata;
	struct stat stats;
	FILE *f;
	byte magic[4], zeros[7];
	byte magic_const[4] = {0x4e, 0x45, 0x53, 0x1a};
	byte zeros_const[7] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	int ret = 0;

	f = fopen(path, "r");

	if (f == NULL) {
		ret = -1;
		goto fullexit;
	}

	if (fread((void*)magic, sizeof(byte), 4, f) != 4) {
		ret = INES_E_MAGIC;
		goto exit;
	}

	if (memcmp(magic, magic_const, 4)) {
		ret = INES_E_MAGIC;
		goto exit;
	}

	if (fread(&inesdata, sizeof(struct st_ines), 1, f) != 1) {
		ret = INES_E_HEADER;
		goto exit;
	}

	if (fread(zeros, sizeof(byte), 7, f) != 7) {
		ret = INES_E_HEADER;
		goto exit;
	}

	if (memcmp(zeros, zeros_const, 7)) {
		ret = INES_E_HEADER;
		goto exit;
	}

	size_t prgsize = inesdata.prgbanks * 16384 * sizeof(byte);
	size_t chrsize = inesdata.chrbanks * 8192 * sizeof(byte);

	byte * PRG = malloc(prgsize);

	if (PRG == NULL) {
		ret = INES_E_MALLOC;
		goto exit;
	}

	byte * CHR = malloc(chrsize);

	if (CHR == NULL) {
		ret = INES_E_MALLOC;
		free(PRG);
		goto exit;
	}

	if (fread(PRG, sizeof(byte), prgsize, f) != prgsize) {
		ret = INES_E_PRG;
		goto freeexit;
	};

	if (fread(CHR, sizeof(byte), chrsize, f) != chrsize) {
		ret = INES_E_CHR;
		goto freeexit;
	};

	stat(path, &stats);

	if (ftell(f) != stats.st_size) {
		ret = INES_E_SIZE;
		goto freeexit;
	};

	printf("PRG: %02x CHR: %02x RAM: %02x CONTROL1: %02x CONTROL2: %02x\n",
			inesdata.prgbanks, inesdata.chrbanks, inesdata.rambanks,
			inesdata.control1, inesdata.control2);

freeexit:
	free(PRG);
	free(CHR);
exit:
	fclose(f);
fullexit:
	return ret;
};

#include <conio.h>	/* for inp*(), outp*() */
#include <i86.h>	/* for int86() */

#include <stdio.h>
#include <stdlib.h>

#define CLI()	do __asm { cli }; while (0)
#define STI()	do __asm { sti }; while (0)

/* I/O Addresses */
#define VGA_ATTR_REG	0x03C0
#define VGA_SEQ_REG	0x03C4
#define VGA_PEL_REG	0x03C6
#define VGA_PALLETE_ADDR_REG	0x03C8
#define VGA_PALLETE_DATA_REG	0x03C9
#define VGA_GR_REG	0x03CE
#define VGA_CRTC_REG	0x03D4
#define VGA_STATUS_REG	0x03DA
#define IOP_FF0A	0xFF0A

#define IMAGE_WIDTH	640UL
#define IMAGE_HEIGHT	480UL
#define IMAGE_SIZE	(IMAGE_WIDTH * IMAGE_HEIGHT)

static char far *const VRAM_WIN = (void far *)0xC0000000UL;

void attr_write(unsigned char index, unsigned char value) {
	(void)inp(VGA_STATUS_REG);	/* Attribute Reg FF reset */
	(void)outp(VGA_ATTR_REG, index);
	(void)outp(VGA_ATTR_REG, value);
}

void attrindex_write(unsigned char value) {
	(void)inp(VGA_STATUS_REG);	/* Attribute Reg FF reset */
	(void)outp(VGA_ATTR_REG, value);
}

void vga_init() {  // 640x480x256 color
	// attribute registers
	attr_write(0x10, 0x41);
	attrindex_write(0x20);
	// sequencer register
	(void)outpw(VGA_SEQ_REG, 0x0C04);
	// graphics control
	(void)outpw(VGA_GR_REG, 0x4005);
	(void)outpw(VGA_GR_REG, 0x0106);
}

void vga_exit() { 
	// attribute registers
	attr_write(0x10, 0x20);
	attrindex_write(0x20);
	// sequencer register
	(void)outpw(VGA_SEQ_REG, 0x0404);
	// graphics control
	(void)outpw(VGA_GR_REG, 0x0305);
	(void)outpw(VGA_GR_REG, 0x0506);
}

static void restore_system_state(union REGS *regs) {
	(void)outpw(VGA_SEQ_REG, 0x0106);
	(void)outpw(VGA_SEQ_REG, 0x3f08);
	(void)outpw(VGA_SEQ_REG, 0x0006);
	vga_exit();

	regs->w.ax = 0x1BCA;
	(void)int86(0x91, regs, regs);

	regs->w.ax = 0x0B00;
	(void)int86(0x91, regs, regs);
}

int main(int argc, char *argv[]) {
	union REGS regs;
	FILE *pal, *img;
	unsigned int i,j,k,voffset,vseg;
	unsigned char r,g,b;
	unsigned char pallete[768]={0};
	unsigned char __far *pic;
	unsigned char pixel;
	const char *image_path;
	const char *palette_path;
	unsigned long remaining;
	unsigned int chunk_size;
	unsigned int window_index;
	unsigned int bytes_read;

	if (argc < 3) {
		printf("usage: %s <image file> <palette file>\n", argv[0]);
		return -1;
	}

	image_path = argv[1];
	palette_path = argv[2];

	pic = malloc(0x8000);
	if(pic == 0){
		printf("malloc failed");
		return -1;
	}

	pal = fopen(palette_path, "rb");
	if(pal==NULL){
		printf("palette open failed");
		return -1;
	}
	if (fread(pallete, 1, 768, pal) != 768) {
		printf("palette read failed");
		fclose(pal);
		return -1;
	}
	fclose(pal);

	img = fopen(image_path, "rb");
	if(img==NULL){
		printf("image open failed");
		return -1;
	}

	remaining = IMAGE_SIZE;


	// system row erase
	regs.w.ax = 0x1B8A;
	(void)int86(0x91, &regs, &regs);

	// cursor erase
	regs.w.ax = 0x0B01;
	(void)int86(0x91, &regs, &regs);

	CLI();
	vga_init();
	(void)outp(VGA_PEL_REG, 0xFF);
	(void)outp(VGA_PALLETE_ADDR_REG, 0x00);
	for (i = 0; i < 768; i+=3) {
		r = pallete[i] >> 4;
		g = pallete[i+1] >> 4;
		b = pallete[i+2] >> 4;
		(void)outp(VGA_PALLETE_DATA_REG, r);
		(void)outp(VGA_PALLETE_DATA_REG, g);
		(void)outp(VGA_PALLETE_DATA_REG, b);
    }

	STI();

	for (j = 0; j < 3;j++){
		// more vram window
		(void)outpw(VGA_SEQ_REG, 0x0106);
		(void)outpw(VGA_SEQ_REG, 0x0108 | j << 12);
		(void)outpw(VGA_SEQ_REG, 0x0006);
		for (k = 0; k < 4 && remaining > 0;k++){
			window_index = k >> 1;
			(void)outpw(IOP_FF0A, 0x40 * window_index);
			chunk_size = remaining > 0x8000UL ? 0x8000U : (unsigned int)remaining;
			bytes_read = (unsigned int)fread(pic, 1, chunk_size, img);
			if (bytes_read != chunk_size) {
				printf("image read failed");
				fclose(img);
				free(pic);
				restore_system_state(&regs);
				return -1;
			}
			for (i = 0; i < chunk_size;i++) {
				VRAM_WIN[k*0x8000+i] = pic[i];
			}
			remaining -= chunk_size;
		}
	}
	fclose(img);

    (void)getch();

	restore_system_state(&regs);
    return 0;
}

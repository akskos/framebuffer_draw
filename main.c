#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>

int fb_fd;

uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo* vinfo) {
	return (r << vinfo->red.offset) | (g << vinfo->green.offset) | (b << vinfo->blue.offset);
}

void print_finfo(struct fb_fix_screeninfo finfo) {
	printf("id string: %s\n", finfo.id);
	printf("smem_start: %lu\n", finfo.smem_start);
	printf("smem_len: %u\n", finfo.smem_len);
	printf("line_length: %u\n", finfo.line_length);
}

void print_vinfo(struct fb_var_screeninfo vinfo) {
	printf("xres: %u\n", vinfo.xres);
	printf("yres: %u\n", vinfo.yres);
	printf("xoffset: %u\n", vinfo.xoffset);
	printf("yoffset: %u\n", vinfo.yoffset);
	printf("bits_per_pixel: %u\n", vinfo.bits_per_pixel);
}

void signal_handler(int sig) {
	close(fb_fd);
	exit(0);
}

int main() {
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	fb_fd = open("/dev/fb0", O_RDWR);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);	
	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);	

	print_finfo(finfo);
	print_vinfo(vinfo);

	long screen_size = vinfo.yres_virtual * finfo.line_length;
	uint8_t* fbp = mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);

	for (;;) {
		for (int i = 0; i < vinfo.yres_virtual; i++) {
			for (int j = 0; j < vinfo.xres_virtual; j++) {
				int offset = i * vinfo.xres_virtual * (vinfo.bits_per_pixel / 8);
				offset += j * (vinfo.bits_per_pixel / 8);
				uint8_t red = rand() % 255;
				uint8_t green = rand() % 255;
				uint8_t blue = rand() % 255;
				*((uint32_t*)(fbp + offset)) = pixel_color(red, green, blue, &vinfo);
			}
		}
		sleep(3);
	}

	return 0;
}

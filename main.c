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

struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

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

void render_pixel(uint8_t* fbp, struct fb_var_screeninfo* vinfo, int x, int y, struct color* c) {
    uint32_t width = vinfo->xres_virtual; 
    int bytes_per_pixel = (int)vinfo->bits_per_pixel / 8;
    int location = (y * width * bytes_per_pixel) + x * bytes_per_pixel;
    *((uint32_t*)(fbp + location)) = pixel_color(c->red, c->green, c->blue, vinfo);
}

void draw_random_pixels(uint8_t* fbp, struct fb_var_screeninfo* vinfo) {
    int width = vinfo->xres_virtual;
    int height = vinfo->yres_virtual;
    for (;;) {
	struct color c;
	c.red = rand() % 255;
	c.green = rand() % 255;
	c.blue = rand() % 255;
	int x = rand() % width;
	int y = rand() % height;
	render_pixel(fbp, vinfo, x, y, &c);
    }
}

int main() {
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	fb_fd = open("/dev/fb0", O_RDWR);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);	
	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);	

	long screen_size = vinfo.yres_virtual * finfo.line_length;
	uint8_t* fbp = mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);

	draw_random_pixels(fbp, &vinfo);

	return 0;
}

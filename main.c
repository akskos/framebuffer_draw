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
#include <png.h>

struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct bounds {
    int width;
    int height;
};

int fb_fd;

void failed(const char* failstr) {
    fprintf(stderr, "failed %s\n", failstr);
    exit(1);
}

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

void render_pixel(uint8_t* fbp, struct fb_var_screeninfo* vinfo, int x, int y, struct color* c, uint32_t line_length) {
    int width = vinfo->xres_virtual; 
    int bytes_per_pixel = (int)vinfo->bits_per_pixel / 8;
    int location = x * bytes_per_pixel + (y * line_length);
    *((uint32_t*)(fbp + location)) = pixel_color(c->red, c->green, c->blue, vinfo);
}

void draw_random_pixels(uint8_t* fbp, struct fb_var_screeninfo* vinfo, struct fb_fix_screeninfo *finfo) {
    int width = vinfo->xres_virtual;
    int height = vinfo->yres_virtual;
    printf("width: %d\n", width);
    for (;;) {
	struct color c;
	c.red = rand() % 255;
	c.green = rand() % 255;
	c.blue = rand() % 255;
	int x = 200 + rand() % 500;
	int y = 100 + rand() % 100;
	render_pixel(fbp, vinfo, x, y, &c, finfo->line_length);
    }
}

png_bytep* read_png_data(const char* filename, struct bounds* b) {
    FILE* fp = fopen(filename, "r");
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
	failed("to create png read struct");
    }
    png_infop info = png_create_info_struct(png);
    if (!info) {
	failed("to create png info struct");
    }
    int rv = setjmp(png_jmpbuf(png));
    if (rv) {
	failed("to setjmp");
    }
    png_init_io(png, fp);
    png_read_info(png, info);
    png_set_palette_to_rgb(png);
    png_set_filler(png, 0xff, PNG_FILLER_AFTER);
    png_read_update_info(png, info);
    png_bytep* row_pointers;
    int width = png_get_image_width(png, info);
    int height = png_get_image_width(png, info);
    b->width = width;
    b->height = height;
    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (int i = 0; i < height; i++) {
	row_pointers[i] = (png_byte*)malloc(png_get_rowbytes(png, info));
    }
    png_read_image(png, row_pointers);

    fclose(fp);
    return row_pointers;
}

void draw_png_data(uint8_t* fbp, struct fb_var_screeninfo* vinfo, struct fb_fix_screeninfo* finfo, png_bytep* row_pointers, int width, int height) {
    for (int i = 0; i < height; i++) {
	png_bytep row = row_pointers[i];
	for (int j = 0; j < width; j++) {
	    png_bytep px = &(row[j * 4]); 
	    struct color c;
	    c.red = px[0];
	    c.green = px[1];
	    c.blue = px[2];
	    render_pixel(fbp, vinfo, j, i, &c, finfo->line_length);
	}
    }
}

int main(int argc, const char** argv) {
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	fb_fd = open("/dev/fb0", O_RDWR);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);	
	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);	

	long screen_size = vinfo.yres_virtual * finfo.line_length;
	uint8_t* fbp = mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);

	if (argc != 2) {
	    fprintf(stderr, "one argument requred\n");
	    exit(1);
	}
	const char* image_path = argv[1];
	struct bounds b;
	png_bytep* row_pointers = read_png_data(image_path, &b);
	draw_png_data(fbp, &vinfo, &finfo, row_pointers, b.width, b.height);
	//draw_random_pixels(fbp, &vinfo, &finfo);

	return 0;
}

/*
Path_tracer - A CPU path tracer

Copyright (C) 2013 Eleni Maria Stea

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Author: Eleni Maria Stea <elene.mst@gmail.com>
*/

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camera.h"
#include "color.h"
#include "intinfo.h"
#include "light.h"
#include "matrix.h"
#include "object.h"
#include "vector.h"
#include "plane.h"
#include "ray.h"
#include "scene.h"
#include "sphere.h"
#include "sphereflake.h"
#include "vector.h"
#include "config.h"

#define DEGTORAD(x)	(M_PI * x / 180.0)

int width = 512;
int height = 512;

SDL_Surface *fbsurf;
Scene scene;
bool use_sdl = true;

Color trace(const Ray &ray, int depth);
Color shade(const Ray &ray, IntInfo *min_info, int depth);
Vector3 reflect(const Vector3 &l, const Vector3 &n);

void render();
void cleanup();
bool write_ppm(const char *fname, uint32_t *pixels, int width, int height);
unsigned long get_msec();

int main(int argc, char **argv) {
	bool scene_loaded = false;

	for (int i=1; i<argc; i++) {
		// if we run with -nosdl, just render and exit
		if (strcmp(argv[i], "-nosdl") == 0) {
			use_sdl = false;
		}
		else if (strcmp(argv[i], "-size") == 0) {
			i++;
			if (!argv[i] || sscanf(argv[i], "%dx%d", &width, &height) < 2) {
				fprintf(stderr, "-size should be followed by WxH\n");
				return 1;
			}
		}
		else {
			if (!scene.load(argv[i])) {
				fprintf(stderr, "failed to load scene file: %s\n", argv[i]);
				return 1;
			}
			scene_loaded = true;
		}
	}

	if (!scene_loaded) {
		fprintf(stderr, "must specify a scene file\n");
		return 1;
	}

	if (use_sdl) {
		SDL_Init(SDL_INIT_VIDEO);
	
		if (!(fbsurf = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE))) {
			fprintf(stderr, "set video mode failed\n");
			return 1;
		}
		SDL_WM_SetCaption("Eleni's Raytracer", 0);
	}

	unsigned long start = get_msec();

	render();

	unsigned long msec = get_msec() - start;
	printf("rendering completed in %lu msec\n", msec);

	// if we are not running interactively just quit before the event loop
	if (!use_sdl) {
		cleanup();
		return 0;
	}
	
	SDL_Event ev;
	bool done = false;

	while (!done && SDL_WaitEvent(&ev)) {
		switch(ev.type) {
		case SDL_QUIT:
			done = true;
			break;

		case SDL_KEYDOWN:
			if(ev.key.keysym.sym == SDLK_ESCAPE) {
				done = true;
			}
			break;

		default:
			break;
		}
	}

	cleanup();
	return 0;
}

void cleanup() {
	SDL_Quit();
}

void render() {
	uint32_t *fb;

	if (use_sdl) {
		if (SDL_MUSTLOCK(fbsurf)) {
	        SDL_LockSurface(fbsurf);
		}
		fb = (uint32_t*)fbsurf->pixels;
    } else {
		fb = new uint32_t[width * height];
	}
	
	int r = 0, g = 0, b = 0;

	for (int y = 0; y < height; y++) {

		printf(" rendering: [");
		int progr = 100 * (y + 1) / height;
		for(int i=0; i<100; i+=2) {
			if(i < progr) {
				putchar('=');
			} else if(i - progr > 1) {
				putchar(' ');
			} else {
				putchar('>');
			}
		}
		printf("] %d%%\r", progr);
		fflush(stdout);

		for (int x = 0; x < width; x++) {
			Ray ray = scene.get_camera()->get_primary_ray(x, y);

			Color color = trace(ray, MAX_DEPTH); 

			color.x = color.x > 1.0 ? 1.0 : color.x;
			color.y = color.y > 1.0 ? 1.0 : color.y;
			color.z = color.z > 1.0 ? 1.0 : color.z;

			color = color * 255;

			r = color.x;
			g = color.y;
			b = color.z;

			*fb = ((uint32_t) r << 16) | ((uint32_t) g << 8) | (uint32_t) b;
			fb++;
		}
	}

	putchar('\n');
	
	if (use_sdl) {
		if (SDL_MUSTLOCK(fbsurf)) {
    	    SDL_UnlockSurface(fbsurf);
	    }
		SDL_Flip(fbsurf);
	}
	else {
		// output the image
		if (!write_ppm("out.ppm", fb - width * height, width, height)) {
			fprintf(stderr, "failed to write image: out.ppm\n");
		}
	}
}

Color trace(const Ray &ray, int depth) {
	IntInfo min_info;
	bool isect = scene.intersection(ray, &min_info);
	if (isect) {
		return shade(ray, &min_info, depth);
	}

	return Color(0, 0, 0);
}

Color shade(const Ray &ray, IntInfo* min_info, int depth) {

	if (!depth) 
		return Color(0, 0, 0);

	Vector3 n = min_info->normal;
	Vector3 p = min_info->i_point;
	Vector3 v = normalize(ray.origin - p);

	const Material *mat = min_info->object->get_material();
	Color color = scene.get_ambient() * mat->kd;
	
	for (int i = 0; i < (int)scene.lights.size(); i++) {
		Light *light = scene.lights[i];

		Ray sray;
		sray.origin = p;
		sray.dir = light->position - p;

		if (!scene.intersection(sray, 0)) {
			Vector3 l = normalize(sray.dir);
			Vector3 lr = reflect(l, n); 

			double d = dot(n, l);
			if (d < 0.0) {
				d = 0;
			}

			double lrdotv = dot(lr, v);
			if(lrdotv < 0.0) {
				lrdotv = 0.0;
			}

			double s = pow(lrdotv, mat->specexp);
			color = color + (d * mat->kd + s * mat->ks) * light->color;

		}
	}

	if (mat->kr > 0.0) {
		Ray refray;
		refray.origin = p;
		refray.dir = reflect(-ray.dir, n);
		color = color + mat->kr * trace(refray, depth-1) * mat->ks;
	}

	return color;
}

bool write_ppm(const char *fname, uint32_t *pixels, int width, int height) {
	FILE *fp;

	if(!(fp = fopen(fname, "wb"))) {
		return false;
	}

	fprintf(fp, "P6\n%d %d\n255\n", width, height);

	int imgsz = width * height;
	for(int i=0; i<imgsz; i++) {
		uint32_t pix = *pixels++;
		int r = (pix >> 16) & 0xff;
		int g = (pix >> 8) & 0xff;
		int b = pix & 0xff;

		fputc(r, fp);
		fputc(g, fp);
		fputc(b, fp);
	}
	fclose(fp);
	return true;
}

#if defined(unix) || defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>

unsigned long get_msec() {
	struct timeval tv;
	static struct timeval tv0;

	gettimeofday(&tv, 0);

	if(tv0.tv_sec == 0 && tv0.tv_usec == 0) {
		tv0 = tv;
		return 0;
	}

	return (tv.tv_sec - tv0.tv_sec) * 1000 + (tv.tv_usec - tv0.tv_usec) / 1000;
}

#elif defined(WIN32) || defined(__WIN32__)
#include <windows.h>

unsigned long get_msec() {
	return timeGetTime();
}

#else
#error "unsupported platform"
#endif

/* -*- Mode: C; tab-width: 4 -*- */
/* sproingies.c - 3D sproingies */

/*-
 *  sproingies.c - Copyright 1996 by Ed Mackey, freely distributable.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Revision History:
 * See sproingiewrap.c
 */

#include <stdlib.h>
#include <math.h>
#include "../include/GL/gl.h"
#include "../include/zbuffer.h"
#include "gllist.h"
#include "sproingies.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define LRAND()         ((long) (rand() & 0x7fffffff))
#define MAXSPROING           100
#define TARGET_COUNT         40
#define BOOM_FRAME           50
#define NO_FRAME             (-10)
#define RESET_SPROINGIE_LIFE (-30 + myrand(28))
#define NEW_SPROINGIE_LIFE   (40 + myrand(200))
#define JUMP_LEFT            0
#define JUMP_RIGHT           1

#define FIRST_FRAME          0
#define LAST_FRAME           5
/*-
 * The sproingies have six "real" frames, (s1_1 to s1_6) that show a
 * sproingie jumping off a block, headed down and to the right. 
 * The frames are numbered from 0 (FIRST_FRAME) to 5 (LAST_FRAME). 
 * 
 * There are other frame numbers for special cases (e.g. BOOM_FRAME).
 */

extern const struct gllist *s1_1;
extern const struct gllist *s1_2;
extern const struct gllist *s1_3;
extern const struct gllist *s1_4;
extern const struct gllist *s1_5;
extern const struct gllist *s1_6;
extern const struct gllist *s1_b;


static int
myrand(int range)
{
	return ((int) (((float) range) * LRAND() / (RAND_MAX)));
}

static int smart_sproingies = 0;

static      GLuint
build_TopsSides(int wireframe)
{
	GLuint      dl_num;
	GLfloat     mat_color[4] =
	{0.0, 0.0, 0.0, 1.0};

	dl_num = glGenLists(2);
	if (!dl_num)
		return (0);	/* 0 means out of display lists. */

	/* Surface: Tops */
	glNewList(dl_num, GL_COMPILE);
	mat_color[0] = 0.392157;
	mat_color[1] = 0.784314;
	mat_color[2] = 0.941176;
	if (wireframe)
		glColor3fv(mat_color);
	else {
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_color);
	}
	glEndList();

	/* Surface: Sides */
	glNewList(dl_num + 1, GL_COMPILE);
	if (wireframe)
		glColor3fv(mat_color);
	else {
      /* jwz: in wireframe mode, color tops and sides the same. */
      mat_color[0] = 0.156863;
      mat_color[1] = 0.156863;
      mat_color[2] = 0.392157;
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_color);
	}
	glEndList();
	return (dl_num);
}

static void
LayGround(int sx, int sy, int sz, int width, int height, sp_instance * si)
{
	int         x, y, z, w, h;
	GLenum      begin_polygon;

	if (si->wireframe)
		begin_polygon = GL_LINE_LOOP;
	else
		begin_polygon = GL_POLYGON;

	if (!si->wireframe) {
		if (!si->mono)
			glCallList(si->TopsSides);	/* Render the tops */
		glNormal3f(0.0, 1.0, 0.0);

		for (h = 0; h < height; ++h) {
			x = sx + h;
			y = sy - (h << 1);
			z = sz + h;
			for (w = 0; w < width; ++w) {
				glBegin(begin_polygon);
				glVertex3f(x, y, z);
				glVertex3f(x, y, z - 1);
				glVertex3f(x + 1, y, z - 1);
				glVertex3f(x + 1, y, z);
				glEnd();
				glBegin(begin_polygon);
				glVertex3f(x + 1, y - 1, z);
				glVertex3f(x + 1, y - 1, z - 1);
				glVertex3f(x + 2, y - 1, z - 1);
				glVertex3f(x + 2, y - 1, z);
				glEnd();
				++x;
				--z;
			}
		}
	}
	if (!si->mono)
		glCallList(si->TopsSides + 1);	/* Render the sides */
	if (!si->wireframe)
		glNormal3f(0.0, 0.0, 1.0);

	for (h = 0; h < height; ++h) {
		x = sx + h;
		y = sy - (h << 1);
		z = sz + h;
		for (w = 0; w < width; ++w) {
			glBegin(begin_polygon);
			glVertex3f(x, y, z);
			glVertex3f(x + 1, y, z);
			glVertex3f(x + 1, y - 1, z);
			glVertex3f(x, y - 1, z);
			glEnd();
			glBegin(begin_polygon);
			glVertex3f(x + 1, y - 1, z);
			glVertex3f(x + 2, y - 1, z);
			glVertex3f(x + 2, y - 2, z);
			glVertex3f(x + 1, y - 2, z);
/*-
 * PURIFY 4.0.1 reports an unitialized memory read on the next line when using
 * MesaGL 2.2 and -mono.  This has been fixed in MesaGL 2.3 and later. */
			glEnd();
			++x;
			--z;
		}
	}

	/* Render the other sides */
	if (!si->wireframe)
		glNormal3f(1.0, 0.0, 0.0);

	for (h = 0; h < height; ++h) {
		x = sx + h;
		y = sy - (h << 1);
		z = sz + h;
		for (w = 0; w < width; ++w) {
			glBegin(begin_polygon);
			glVertex3f(x + 1, y, z);
			glVertex3f(x + 1, y, z - 1);
			glVertex3f(x + 1, y - 1, z - 1);
			glVertex3f(x + 1, y - 1, z);
			glEnd();
			glBegin(begin_polygon);
			glVertex3f(x + 2, y - 1, z);
			glVertex3f(x + 2, y - 1, z - 1);
			glVertex3f(x + 2, y - 2, z - 1);
			glVertex3f(x + 2, y - 2, z);
			glEnd();
			++x;
			--z;
		}
	}

	if (si->wireframe) {
		if (!si->mono)
			glCallList(si->TopsSides);	/* Render the tops */

		for (h = 0; h < height; ++h) {
			x = sx + h;
			y = sy - (h << 1);
			z = sz + h;
			for (w = 0; w < width; ++w) {
				glBegin(begin_polygon);
				glVertex3f(x, y, z);
				glVertex3f(x, y, z - 1);
				glVertex3f(x + 1, y, z - 1);
				glVertex3f(x + 1, y, z);
				glEnd();
				glBegin(begin_polygon);
				glVertex3f(x + 1, y - 1, z);
				glVertex3f(x + 1, y - 1, z - 1);
				glVertex3f(x + 2, y - 1, z - 1);
				glVertex3f(x + 2, y - 1, z);
				glEnd();
				++x;
				--z;
			}
		}
	}
}

static void
AdvanceSproingie(int t, sp_instance * si)
{
	int         g_higher, g_back, t2;
	struct sPosColor *thisSproingie = &(si->positions[t]);
	struct sPosColor *S2 = &(si->positions[0]);

	if (thisSproingie->life > 0) {
		if ((++(thisSproingie->frame)) > LAST_FRAME) {
			if (thisSproingie->frame >= BOOM_FRAME) {
				if ((thisSproingie->r -= 0.08) < 0.0)
					thisSproingie->r = 0.0;
				if ((thisSproingie->g -= 0.08) < 0.0)
					thisSproingie->g = 0.0;
				if ((thisSproingie->b -= 0.08) < 0.0)
					thisSproingie->b = 0.0;
				if ((--(thisSproingie->life)) < 1) {
					thisSproingie->life = RESET_SPROINGIE_LIFE;
				}
				return;
			}
			thisSproingie->frame = FIRST_FRAME;

			/* Check for collisions */
			for (t2 = 0; t2 < si->maxsproingies; ++t2) {
				if ((t2 != t) && (thisSproingie->x == S2->x) &&
				    (thisSproingie->y == S2->y) && (thisSproingie->z == S2->z) &&
				    (S2->life > 10) && (S2->frame < LAST_FRAME + 1)) {
#if 0
					if (thisSproingie->life > S2->life) {
						S2->life = 10;
					} else {
#endif
						if (thisSproingie->life > 10) {
							thisSproingie->life = 10;
							thisSproingie->frame = BOOM_FRAME;
							if ((thisSproingie->r += 0.5) > 1.0)
								thisSproingie->r = 1.0;
							if ((thisSproingie->g += 0.5) > 1.0)
								thisSproingie->g = 1.0;
							if ((thisSproingie->b += 0.5) > 1.0)
								thisSproingie->b = 1.0;
						}
#if 0
					}
#endif
				}
				++S2;
			}
		}
		/* Time to disappear... */
		if (!((thisSproingie->life == 10) &&
		      (thisSproingie->frame > FIRST_FRAME) &&
		      (thisSproingie->frame < BOOM_FRAME))) {
			if ((--(thisSproingie->life)) < 1) {
				thisSproingie->life = RESET_SPROINGIE_LIFE;
			} else if (thisSproingie->life < 9) {
				thisSproingie->frame -= 2;
			} 
		}		/* ... else wait here for frame FIRST_FRAME to come about. */
	} else if (++(thisSproingie->life) >= 0) {
		if (1 || t > 1) {
			g_higher = -3 + myrand(5);
			g_back = -2 + myrand(5);
		} else if (t == 1) {
			g_higher = -2 + myrand(3);
			g_back = -1 + myrand(3);
		} else {
			g_higher = -1;
			g_back = 0;
		}

		thisSproingie->x     = (-g_higher - g_back);
		thisSproingie->y     = (g_higher << 1);
		thisSproingie->z     = (g_back - g_higher);
		thisSproingie->life  = NEW_SPROINGIE_LIFE;
		thisSproingie->frame = NO_FRAME;
		thisSproingie->r     = (GLfloat) (40 + myrand(200)) / 255.0;
		thisSproingie->g     = (GLfloat) (40 + myrand(200)) / 255.0;
		thisSproingie->b     = (GLfloat) (40 + myrand(200)) / 255.0;

		for (t2 = 0; t2 < si->maxsproingies; ++t2) {
			if ((t2 != t) && (thisSproingie->x == S2->x) &&
			    (thisSproingie->y == S2->y) && (thisSproingie->z == S2->z) &&
			    (S2->life > 10) && (S2->frame < FIRST_FRAME)) {
				/* If another is already on this place, wait. */
				thisSproingie->life = -1;
			}
			++S2;
		}
	}
}

static void
NextSproingie(sp_instance *si)
{
	int         ddx, t;
	struct sPosColor *thisSproingie = &(si->positions[0]);

	/* Although the sproingies cycle has six frames, the blocks cycle  */
	/* has twelve. After a full cycle (12 frames), re-center positions */
	/* of sproingies                                                   */
	if (++si->sframe > 11) {
		si->sframe = FIRST_FRAME;
		for (t = 0; t < si->maxsproingies; ++t) {
			thisSproingie->x -= 1;
			thisSproingie->y += 2;
			thisSproingie->z -= 1;
			++thisSproingie;
		}
	}

	for (t = 0; t < si->maxsproingies; ++t) {
		AdvanceSproingie(t, si);
	}

	if (si->target_count < 0) {	/* track to current target */
		if (si->target_rx < si->rotx)
			--si->rotx;
		else if (si->target_rx > si->rotx)
			++si->rotx;

		if (si->target_ry < si->roty)
			--si->roty;
		else if (si->target_ry > si->roty)
			++si->roty;

		ddx = (si->target_dist - si->dist) / 8;
		if (ddx)
			si->dist += ddx;
		else if (si->target_dist < si->dist)
			--si->dist;
		else if (si->target_dist > si->dist)
			++si->dist;

		if ((si->target_rx == si->rotx) && (si->target_ry == si->roty) &&
		    (si->target_dist == si->dist)) {
			si->target_count = TARGET_COUNT;
			if (si->target_dist <= 32)
				si->target_count >>= 2;
		}
	} else if (--si->target_count < 0) {	/* make up new target */
		si->target_rx = myrand(100) - 35;
		si->target_ry = -myrand(90);
		si->target_dist = 32 << myrand(2);	/* could be 32, 64, or 128, (previously or 256) */

		if (si->target_dist >= si->dist)	/* no duplicate distances */
			si->target_dist <<= 1;
	}
	/* Otherwise just hang loose for a while here */
}


static void
RenderSproingie(int t, sp_instance * si)
{
	GLfloat     scale, pointsize, mat_color[4] =
	{0.0, 0.0, 0.0, 1.0};

	struct sPosColor *thisSproingie = &(si->positions[t]);

	if (thisSproingie->life < 1)
		return;

	glPushMatrix();

	// if (!si->mono) {
	// 	mat_color[0] = thisSproingie->r;
	// 	mat_color[1] = thisSproingie->g;
	// 	mat_color[2] = thisSproingie->b;
	// 	if (si->wireframe)
	// 		glColor3fv(mat_color);
	// 	else {
	// 		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_color);
	// 	}
	// }

	if (thisSproingie->frame < FIRST_FRAME) {
		// glEnable(GL_CLIP_PLANE0);
		glTranslatef((GLfloat) (thisSproingie->x),
			     (GLfloat) (thisSproingie->y) +
			     ((GLfloat) (thisSproingie->frame) / 9.0),
			     (GLfloat) (thisSproingie->z));

/**		glCallList(si->sproingies[0]);*/
/**/	renderList(si->sproingies[0], si->wireframe);
		// glDisable(GL_CLIP_PLANE0);
	} else if (thisSproingie->frame >= BOOM_FRAME) {
		glTranslatef((GLfloat) (thisSproingie->x) + 0.5,
			     (GLfloat) (thisSproingie->y) + 0.5,
			     (GLfloat) (thisSproingie->z) - 0.5);
		{
			int boom_scale = thisSproingie->frame - BOOM_FRAME;
			if (boom_scale >= 31) boom_scale = 31;
			scale = (GLfloat) (1 << boom_scale);
		}
		glScalef(scale, scale, scale);
		if (!si->wireframe) {
			if (!si->mono)
				glColor3fv(mat_color);
			glDisable(GL_LIGHTING);
		}
		pointsize = (GLfloat) ((BOOM_FRAME + 8) - thisSproingie->frame) -
			(si->dist / 64.0);
		glPointSize((pointsize < 1.0) ? 1.0 : pointsize);
/*-
 * PURIFY 4.0.1 reports an unitialized memory read on the next line when using
 * MesaGL 2.2.  This has been tracked to MesaGL 2.2 src/points.c line 313. */
/**		glCallList(si->SproingieBoom);*/
/**/	renderList(si->SproingieBoom, si->wireframe);
		glPointSize(1.0);
		if (!si->wireframe) {
			glEnable(GL_LIGHTING);
		}
	} else {
		if (thisSproingie->direction == JUMP_LEFT) {
			/* When the sproingie jumps to the left, the frames must be */
			/* rotated and translated */
			glTranslatef((GLfloat) (thisSproingie->x    ),
						 (GLfloat) (thisSproingie->y    ), 
						 (GLfloat) (thisSproingie->z - 1));
			glRotatef((GLfloat) - 90.0, 0.0, 1.0, 0.0);
			if (thisSproingie->frame == LAST_FRAME) {
				thisSproingie->x -= 0;
				thisSproingie->y -= 1;
				thisSproingie->z += 1;
			} 
		} else {
			glTranslatef((GLfloat) (thisSproingie->x),
						 (GLfloat) (thisSproingie->y), 
						 (GLfloat) (thisSproingie->z));
			glRotatef((GLfloat) - 0.0, 0.0, 1.0, 0.0);
			if (thisSproingie->frame == LAST_FRAME) {
				thisSproingie->x += 1;
				thisSproingie->y -= 1;
				thisSproingie->z -= 0;
			}
		}
/* 	} */
/**		glCallList(si->sproingies[thisSproingie->frame]);*/
/**/	renderList(si->sproingies[thisSproingie->frame], si->wireframe);

		/* Every 6 frame cycle... */
		if (thisSproingie->frame == LAST_FRAME) {
			/* ...check if the sproingies have gone out of the bricks */
			if (((thisSproingie->x - thisSproingie->z == 6) &&
				 (2*thisSproingie->x + thisSproingie->y == 6)) ||
				((thisSproingie->z - thisSproingie->x == 5) &&
				 (2*thisSproingie->x + thisSproingie->y == -5))) {
				/* If they have, then they die */
				if (thisSproingie->life > 0 && thisSproingie->frame < BOOM_FRAME && thisSproingie->frame > FIRST_FRAME) {
					thisSproingie->frame = BOOM_FRAME;
				}
			} else {
				/* If not, they choose a direction for the next hop */
				if (smart_sproingies) {
					if ((thisSproingie->x - thisSproingie->z == 5) &&   
						(2*thisSproingie->x + thisSproingie->y == 5)) {
						thisSproingie->direction = JUMP_LEFT;
					} else if ((thisSproingie->z - thisSproingie->x == 4) &&   
						 (2*thisSproingie->x + thisSproingie->y == -4)) {
						thisSproingie->direction = JUMP_RIGHT;
					} else {
						thisSproingie->direction = myrand(2);
					}
				} else {
					thisSproingie->direction = myrand(2);
				}
			}
		}
	}

	glPopMatrix();

}

static void
ComputeGround(sp_instance * si)
{
	int         g_higher, g_back, g_width, g_height;

	/* higher: x-1, y+2, z-1 */
	/* back: x-1, y, z+1 */

	if (si->groundlevel == 0) {
		g_back = 2;
		g_width = 5;
	} else if (si->groundlevel == 1) {
		g_back = 4;
		g_width = 8;
	} else {
		g_back = 8;
		g_width = 16;
	}

	if ((g_higher = si->dist >> 3) < 4)
		g_higher = 4;
	if (g_higher > 16)
		g_higher = 16;
	g_height = g_higher << 1;

	if (si->rotx < -10)
		g_higher += (g_higher >> 2);
	else if (si->rotx > 10)
		g_higher -= (g_higher >> 2);

	GLfloat clr[4] = {0.5, 0.0, 0.0, 0.0};
	GLfloat clr2[4] = {1.0, 0.0, 0.0, 0.0};
	GLfloat clr3[4] = {0.0, 0.0, 0.0, 0.0};

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, clr);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, clr3);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, clr3);

	/* startx, starty, startz, width, height */
	LayGround((-g_higher - g_back), (g_higher << 1), (g_back - g_higher),
		  (g_width), (g_height), si);

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, clr2);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, clr3);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, clr3);

}

void
DisplaySproingies(sp_instance *si)
{
	int         t;
	GLfloat     position[] =
	{8.0, 5.0, -2.0, 0.1};

	if (si->wireframe)
		glClear(GL_COLOR_BUFFER_BIT);
	else
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	glTranslatef(0.0, 0.0, -(GLfloat) (si->dist) / 16.0);	/* viewing transform  */
	glRotatef((GLfloat) si->rotx, 1.0, 0.0, 0.0);
	glRotatef((GLfloat) si->roty, 0.0, 1.0, 0.0);

	if (!si->wireframe)
		glLightfv(GL_LIGHT0, GL_POSITION, position);

	glTranslatef((GLfloat) si->sframe * (-1.0 / 12.0) - 0.75,
		     (GLfloat) si->sframe * (2.0 / 12.0) - 0.5,
		     (GLfloat) si->sframe * (-1.0 / 12.0) + 0.75);

	if (si->wireframe)
		ComputeGround(si);

	for (t = 0; t < si->maxsproingies; ++t) {
		RenderSproingie(t, si);
	}

	if (!si->wireframe)
		ComputeGround(si);

	glPopMatrix();
	glFlush();
}

void
NextSproingieDisplay(sp_instance *si)
{
	NextSproingie(si);
/*        if (pause) usleep(pause);  don't do this! -jwz */
	DisplaySproingies(si);
}

void
ReshapeSproingies(int width, int height)
{
  double h = (GLfloat) height / (GLfloat) width;  
  int y = 0;

  if (width > height * 5) {   /* tiny window: show middle */
    height = width * 9/16;
    y = -height/2;
    h = height / (GLfloat) width;
  }

  glViewport(0, y, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

// void glhPerspectivef2(float *matrix, float fovyInDegrees, float aspectRatio,
//                       float znear, float zfar)

  // gluPerspective(65.0, 1/h, 0.1, 2000.0);	/* was 200000.0 */

	float ymax, xmax;
	ymax = 0.1 * tanf(65.0 * M_PI / 360.0);
   //  // ymin = -ymax;
   //  // xmin = -ymax * aspectRatio;
   xmax = ymax * h;
   //  glhFrustumf2(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);

  glFrustum(-xmax, xmax, -ymax, ymax, 0.1, 2000.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void
CleanupSproingies(sp_instance *si)
{
   if (! si) return;

	if (si->TopsSides) {
		glDeleteLists(si->TopsSides, 2);
	}
	if (si->positions) {
		free((si->positions));
		si->positions = NULL;
	}
}

void
InitSproingies(sp_instance *si, int wfmode, int grnd, int mspr, int smrtspr,
			   int mono)
{
	GLfloat     ambient[] =
	{0.2, 0.2, 0.2, 1.0};
	GLfloat     position[] =
	{10.0, 1.0, 1.0, 10.0};
	GLfloat     mat_diffuse[] =
	{0.8, 0.8, 0.8, 1.0};
	GLfloat     mat_specular[] =
	{0.8, 0.8, 0.8, 1.0};
	GLfloat     mat_shininess[] =
	{50.0};

	int         t;

	memset (si, 0, sizeof(*si));

	if (mspr < 0)
		mspr = 0;
	if (mspr >= MAXSPROING)
		mspr = MAXSPROING - 1;

	smart_sproingies = smrtspr; 

	si->rotx = 0;
	si->roty = -45;
	si->dist = (16 << 2);
	si->sframe = 0;
	si->target_count = 0;
	si->mono = mono;

	si->wireframe = si->flatshade = 0;

	if (wfmode == 2)
		si->flatshade = 1;
	else if (wfmode)
		si->wireframe = 1;

	si->groundlevel = grnd;
	si->maxsproingies = mspr;

	if (si->maxsproingies) {
		si->positions = (struct sPosColor *) calloc(si->maxsproingies,
						  sizeof (struct sPosColor));

		if (!(si->positions))
			si->maxsproingies = 0;
	}
	for (t = 0; t < si->maxsproingies; ++t) {
		si->positions[t].x = 0;
		si->positions[t].y = 0;
		si->positions[t].z = 0;
		si->positions[t].life = (-t * ((si->maxsproingies > 19) ? 1 : 4)) - 2;
		si->positions[t].frame = FIRST_FRAME;
		si->positions[t].direction = myrand(2);
	}

	si->sproingies[0]=s1_1;
	si->sproingies[1]=s1_2;
	si->sproingies[2]=s1_3;
	si->sproingies[3]=s1_4;
	si->sproingies[4]=s1_5;
	si->sproingies[5]=s1_6;
	si->SproingieBoom=s1_b;

	if (si->wireframe) {
		glShadeModel(GL_FLAT);
		glDisable(GL_LIGHTING);
	} else {
		if (si->flatshade) {
			glShadeModel(GL_FLAT);
			position[0] = 1.0;
			position[3] = 0.0;
		}
		// glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		// glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);

		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_POSITION, position);

		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

		/* glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE); */
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);

		glFrontFace(GL_CW);
		/* glEnable(GL_NORMALIZE); */
	}
}

/* End of sproingies.c */


#include <SDL/SDL.h>


typedef struct {
	GLfloat     view_rotx, view_roty, view_rotz;
	GLint       gear1, gear2, gear3;
	GLfloat     angle;
	GLuint      limit;
	GLuint      count;
	int         mono;
	sp_instance si;
} sproingiesstruct;

static sproingiesstruct sproingies[1] = {0};


void init_sproingies(int w, int h)
{
	sproingiesstruct *sp;
	int wfmode = 0, grnd = 0, mspr = 5, smrt_spr = 0;

	sp = &sproingies[0];

	sp->mono = 1;

	/* wireframe, ground, maxsproingies */
	InitSproingies(&sp->si, wfmode, grnd, mspr, smrt_spr, sp->mono);

	ReshapeSproingies(w, h);

	DisplaySproingies(&sp->si);
}


int main(int argc, char** argv) {
	// initialize SDL video:
	int winSizeX = 640;
	int winSizeY = 480;
	unsigned int fps = 0;
	unsigned int dith_map_id = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "ERROR: cannot initialize SDL video.\n");
		return 1;
	}

	SDL_Surface* screen = NULL;

	if ((screen = SDL_SetVideoMode(winSizeX, winSizeY, 32, SDL_SWSURFACE)) == 0)
	{
		fprintf(stderr, "ERROR: Video mode set failed.\n");
		return 1;
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption(argv[0], 0);

	ZBuffer* frameBuffer = NULL;
	
	if (TGL_FEATURE_RENDER_BITS == 32)
		frameBuffer = ZB_open(winSizeX, winSizeY, ZB_MODE_RGBA, 0);
	else
		frameBuffer = ZB_open(winSizeX, winSizeY, ZB_MODE_5R6G5B, 0);
	
	if (!frameBuffer)
	{
		printf("\nZB_open failed!");
		exit(1);
	}
	
	glInit(frameBuffer);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	
	init_sproingies(winSizeX, winSizeY);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	// glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);

	glSetEnableSpecular(GL_FALSE);

	ZB_setDitheringMap(frameBuffer, 9);

	int isRunning = 1;

	// variables for timing:
	unsigned int frames = 0;
	unsigned int tNow = SDL_GetTicks();
	unsigned int tLastFps = tNow;

	while (isRunning) {
		++frames;
		tNow = SDL_GetTicks();

		SDL_Event evt;
		while (SDL_PollEvent(&evt))
		{			
			switch (evt.type) {
			case SDL_KEYDOWN:
				switch (evt.key.keysym.sym) {
				case SDLK_m:
					dith_map_id++;
					if (dith_map_id >= NUM_DITHER_MAPS)
					{
						dith_map_id = 0;
					}
					ZB_setDitheringMap(frameBuffer, dith_map_id);
					break;
				case SDLK_ESCAPE:
				case SDLK_q:
					isRunning = 0;
				default:
					break;
				}
				break;
			
			case SDL_QUIT:
				isRunning = 0;
				break;
			}
		}
		

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		NextSproingieDisplay(&sproingies[0].si);

		// SDL_Delay(20);

		// swap buffers:
		if (SDL_MUSTLOCK(screen) && (SDL_LockSurface(screen) < 0)) {
			fprintf(stderr, "SDL ERROR: Can't lock screen: %s\n", SDL_GetError());
			return 1;
		}

		ZB_copyFrameBufferARGB32(frameBuffer, screen->pixels, screen->pitch * screen->h);

		if (SDL_MUSTLOCK(screen))
			SDL_UnlockSurface(screen);

		SDL_Flip(screen);

		if (fps > 0)
			if ((1000 / fps) > (SDL_GetTicks() - tNow)) {
				SDL_Delay((1000 / fps) - (SDL_GetTicks() - tNow)); // Yay stable framerate!
			}	
		// update fps:
		if (tNow >= tLastFps + 5000) {
			printf("%i frames in %f secs, %f frames per second.\n", frames, (float)(tNow - tLastFps) * 0.001f,
				   (float)frames * 1000.0f / (float)(tNow - tLastFps));
			tLastFps = tNow;
			frames = 0;
		}
	}
	printf("%i frames in %f secs, %f frames per second.\n", frames, (float)(tNow - tLastFps) * 0.001f, (float)frames * 1000.0f / (float)(tNow - tLastFps));

	ZB_close(frameBuffer);
	glClose();

	if (SDL_WasInit(SDL_INIT_VIDEO))
		SDL_QuitSubSystem(SDL_INIT_VIDEO);

	SDL_Quit();
	return 0;
}


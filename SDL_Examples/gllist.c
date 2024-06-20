/* xscreensaver, Copyright (c) 1998-2014 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include "gllist.h"

void
renderList (const struct gllist *list, int wire_p)
{
  while (list)
    {
      if (!wire_p || list->primitive == GL_LINES ||
          list->primitive == GL_POINTS)
        {
          if (list->primitive == GL_TRIANGLES)
          {
            glVertexPointer(3, GL_FLOAT, 3, (const float*)list->data + 3);
            glEnableClientState(GL_VERTEX_ARRAY);
            glDrawArrays(list->primitive, 0, list->points);
            glDisableClientState(GL_VERTEX_ARRAY);
          }
          else if (list->primitive == GL_POINTS)
          {
            glVertexPointer(3, GL_FLOAT, 0, list->data);
            glEnableClientState(GL_VERTEX_ARRAY);
            glDrawArrays(list->primitive, 0, list->points);
            glDisableClientState(GL_VERTEX_ARRAY);
          }
        }
      else
        {
          /* For wireframe, do it the hard way: treat every tuple of
             points as its own line loop.
           */
          const GLfloat *p = (GLfloat *) list->data;
          int i, j, tick, skip, stride;

          switch (list->primitive) {
          case GL_QUADS: tick = 4; break;
          case GL_TRIANGLES: tick = 3; break;
          default: abort(); break; /* write me */
          }

          switch (list->format) {
          case GL_C3F_V3F: case GL_N3F_V3F: skip = 3; stride = 6; break;
          default: abort(); break; /* write me */
          }

          glBegin (GL_LINE_LOOP);
          for (i = 0, j = skip;
               i < list->points;
               i++, j += stride)
            {
              if (i && !(i % tick))
                {
                  glEnd();
                  glBegin (GL_LINE_LOOP);
                }
              glVertex3f (p[j], p[j+1], p[j+2]);
            }
          glEnd();
        }
      list = list->next;
    }
}

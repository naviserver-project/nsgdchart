/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1(the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/.
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis,WITHOUT WARRANTY OF ANY KIND,either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * Alternatively,the contents of this file may be used under the terms
 * of the GNU General Public License(the "GPL"),in which case the
 * provisions of GPL are applicable instead of those above.  If you wish
 * to allow use of your version of this file only under the terms of the
 * GPL and not to allow others to use your version of this file under the
 * License,indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by the GPL.
 * If you do not delete the provisions above,a recipient may use your
 * version of this file under either the License or the GPL.
 *
 * Author Vlad Seryakov vlad@crystalballinc.com
 *
 *
 * GDCHART 0.11.4dev  GDC.H  2 Nov 2000
 * Copyright Bruce Verderaime 1998-2003
 *
 * Note: GDChart is copyrighted code:
 * Permission has been granted to copy, distribute and modify GDChart in any context
 * without fee, including a commercial application, provided that this notice is
 * present in user-accessible supporting documentation.
 *
 * This does not affect your ownership of the derived work itself, and the intent
 * is to assure proper credit for the authors of GDChart, not to interfere with
 * your productive use of GDChart. If you have questions, ask. "Derived works"
 * includes all programs that utilize the library. Credit must be given in
 * user-accessible documentation.
 *
 * This software is provided "AS IS." The copyright holders disclaim all warranties,
 * either express or implied, including but not limited to implied warranties of
 * merchantability and fitness for a particular purpose, with respect to this code and
 * accompanying documentation.
 *
 *
 * nsgdc -- CHART generation module using gdcharts
 *
 * April 2004
 * Vlad Seryakov vlad@crystalballinc.com
 *
 */

#ifndef _NSGDC_H
#define _NSGDC_H

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

#define USE_TCL8X

#include <math.h>
#include "ns.h"
#include <limits.h>
#include <strings.h>
#include <float.h>
#include "gd.h"
#include "gdfonts.h"
#include "gdfontt.h"
#include "gdfontmb.h"
#include "gdfontg.h"
#include "gdfontl.h"

#define VERSION                         "1.1"

#ifndef M_PI
#define M_PI	                        3.14159265358979323846
#define M_PI_2	                        1.57079632679489661923
#endif

#ifndef TRUE
#define TRUE	                        1
#define FALSE	                        0
#endif

#define GDC_NOVALUE			-FLT_MAX
#define GDC_NULL			GDC_NOVALUE

#define ABS(x)			        ((x)<0.0? -(x):(x))
#ifndef MAX
#define MAX(x,y)			((x)>(y)?(x):(y))
#endif
#ifndef MIN
#define MIN(x,y)			((x)<(y)?(x):(y))
#endif
#define TO_RAD(o)			((o)/360.0*(2.0*M_PI))

#define GDC_NOCOLOR			0x1000000L
#define GDC_DFLTCOLOR		        0x2000000L
#define PVRED                           0x00FF0000
#define PVGRN                           0x0000FF00
#define PVBLU                           0x000000FF
#define l2gdcal(c)                      ((c)&PVRED)>>16,((c)&PVGRN)>>8,((c)&0x000000FF)
#define l2gdshd(c)                      (((c)&PVRED)>>16)/2,(((c)&PVGRN)>>8)/2,(((c)&0x000000FF))/2

#define MAX_NOTE_LEN		        19
#define GDC_INTERP_VALUE	        (GDC_NOVALUE/2.0)
#define GDC_INTERP			((GDC_interpolations=TRUE),GDC_INTERP_VALUE)

#define _gdcntrst(bg)		        (((bg)&0x800000?0x000000:0xFF0000) | \
					 ((bg)&0x008000?0x000000:0x00FF00) | \
					 ((bg)&0x000080?0x000000:0x0000FF))

#define clrallocate(im,rawclr)		_clrallocate(im,rawclr,GDC->BGColor)
#define clrshdallocate(im,rawclr)	_clrshdallocate(im,rawclr,GDC->BGColor)

typedef enum {
    GDC_PNG = 0,
    GDC_JPEG = 1,
    GDC_WBMP = 3
} GDC_image_type_t;

/* ordered by size */
enum GDC_font_size {
    GDC_pad = 0,
    GDC_TINY = 1,
    GDC_SMALL = 2,
    GDC_MEDBOLD = 3,
    GDC_LARGE = 4,
    GDC_GIANT = 5,
    GDC_numfonts = 6
};

typedef enum {
    GDC_DESTROY_IMAGE = 0,
    GDC_EXPOSE_IMAGE = 1,
    GDC_REUSE_IMAGE = 2
} GDC_HOLD_IMAGE_T;

typedef struct {
    gdFontPtr f;
    char h;
    char w;
} GDC_FONT_T;

typedef struct {
    int w;
    int h;
} GDC_FONT_SZ_T;

typedef enum {
    GDC_JUSTIFY_RIGHT,
    GDC_JUSTIFY_CENTER,
    GDC_JUSTIFY_LEFT
} GDC_justify_t;

typedef enum {
    GDC_LINE,
    GDC_AREA,
    GDC_BAR,
    GDC_FLOATINGBAR,
    GDC_HILOCLOSE,
    GDC_COMBO_LINE_BAR,
    GDC_COMBO_HLC_BAR,
    GDC_COMBO_LINE_AREA,
    GDC_COMBO_LINE_LINE,
    GDC_COMBO_HLC_AREA,
    GDC_3DHILOCLOSE,
    GDC_3DCOMBO_LINE_BAR,
    GDC_3DCOMBO_LINE_AREA,
    GDC_3DCOMBO_LINE_LINE,
    GDC_3DCOMBO_HLC_BAR,
    GDC_3DCOMBO_HLC_AREA,
    GDC_3DBAR,
    GDC_3DFLOATINGBAR,
    GDC_3DAREA,
    GDC_3DLINE,
    GDC_LINEAREA
} GDC_CHART_T;

typedef enum {
    GDC_STACK_DEPTH,		        /* "behind"(even non-3D) */
    GDC_STACK_SUM,
    GDC_STACK_BESIDE,
    GDC_STACK_LAYER
} GDC_STACK_T;			        /* applies only to num_lines > 1 */

typedef enum {
    GDC_HLC_DIAMOND = 1,
    GDC_HLC_CLOSE_CONNECTED = 2,	/* can't be used w/ CONNECTING */
    GDC_HLC_CONNECTING = 4,	        /* can't be used w/ CLOSE_CONNECTED */
    GDC_HLC_I_CAP = 8
} GDC_HLC_STYLE_T;		        /* can be OR'd */

/* only 1 annotation allowed */
typedef struct {
    float point;		        /* 0 <= point < num_points */
    unsigned long color;
    char note[MAX_NOTE_LEN + 1];	/* NLs ok here */
} GDC_ANNOTATION_T;

typedef enum {
    GDC_SCATTER_TRIANGLE_DOWN,
    GDC_SCATTER_TRIANGLE_UP,
    GDC_SCATTER_CIRCLE
} GDC_SCATTER_IND_T;

typedef struct {
    float point;		        /* 0 <= point < num_points */
    float val;
    int width;	                        /* %(1-100) */
    unsigned long color;
    GDC_SCATTER_IND_T ind;
} GDC_SCATTER_T;

typedef enum {
    GDC_TICK_LABELS = -2,	        /* only at labels */
    GDC_TICK_POINTS = -1,	        /* each num_points */
    GDC_TICK_NONE = 0                   /* > GDC_TICK_NONE *//* points & inter-point */
} GDC_TICK_T;

typedef enum {			        /* backward compatible w/ FALSE/TRUE */
    GDC_BORDER_NONE = 0,
    GDC_BORDER_ALL = 1,		        /* should be GDC_BORDER_Y|Y2|X|TOP */
    GDC_BORDER_X = 2,
    GDC_BORDER_Y = 4,
    GDC_BORDER_Y2 = 8,
    GDC_BORDER_TOP = 16
} GDC_BORDER_T;

typedef struct _GDC_T {
    struct _GDC_T *next,*prev;
    int id;
    GDC_CHART_T type;
    int num_sets;
    int num_points;
    int alloc_num_points;
    int alloc_num_sets;
    char **xlabels;
    char **dlabels;
    float *data;
    char *image_file;
    int width;
    int height;
    char legend;
    int legend_x;
    int legend_y;

    gdImagePtr image;
    GDC_HOLD_IMAGE_T hold;
    GDC_image_type_t image_type;
    int jpeg_quality;
    char *ytitle;
    char *xtitle;
    char *ytitle2;
    char *title;
    enum GDC_font_size title_size;
    enum GDC_font_size ytitle_size;
    enum GDC_font_size xtitle_size;
    enum GDC_font_size yaxisfont_size;
    enum GDC_font_size xaxisfont_size;
    double xaxis_angle;
    char *title_font;
    char *ytitle_font;
    char *xtitle_font;
    char *yaxis_font;
    char *xaxis_font;
    double title_ptsize;
    double ytitle_ptsize;
    double xtitle_ptsize;
    double yaxis_ptsize;
    double xaxis_ptsize;
    char *ylabel_fmt;
    char *ylabel2_fmt;
    char *xlabel_ctl;
    short xlabel_spacing;
    char ylabel_density;
    char interpolations;
    double requested_ymin;
    double requested_ymax;
    double requested_yinterval;
    char Shelf;
    GDC_TICK_T grid;
    GDC_TICK_T ticks;
    GDC_STACK_T stack_type;
    GDC_BORDER_T border;
    char grid_on_top;
    char xaxis;
    char yaxis;
    char yaxis2;
    char yval_style;
    double threed_depth;
    unsigned char threed_angle;
    unsigned char bar_width;
    GDC_HLC_STYLE_T HLC_style;
    unsigned char HLC_cap_width;
    GDC_ANNOTATION_T annotation;
    enum GDC_font_size annotation_font_size;
    char *annotation_font;
    double annotation_ptsize;
    int num_scatter_pts;
    GDC_SCATTER_T *scatter;
    char thumbnail;
    unsigned long BGColor;
    unsigned long GridColor;
    unsigned long LineColor;
    unsigned long PlotColor;
    unsigned long VolColor;
    unsigned long TitleColor;
    unsigned long XTitleColor;
    unsigned long YTitleColor;
    unsigned long YTitle2Color;
    unsigned long XLabelColor;
    unsigned long YLabelColor;
    unsigned long YLabel2Color;
    unsigned long ScatterColor;
    unsigned long LegendColor;
    unsigned long *ExtVolColor;
    unsigned long *SetColor;
    unsigned long *ExtColor;
    char *BGImage;
    char transparent_bg;
    char hard_size;
    int hard_xorig;
    int hard_graphwidth;
    int hard_yorig;
    int hard_grapheight;
} GDC_T;

GDC_T *GDC_alloc();
void GDC_free(GDC_T *gdc);
int GDC_graph(GDC_T *GDC);

#endif

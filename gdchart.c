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
 *          from brv@fred.net http://www.fred.net/brv/chart/
 *
 * Authors
 *
 *     Vlad Seryakov vlad@crystalballinc.com
 */

#include "nsgdchart.h"

static GDC_FONT_T GDC_fontc[GDC_numfonts] = {
   { (gdFontPtr) NULL,8,5 },
   { (gdFontPtr) NULL,8,5 },
   { (gdFontPtr) NULL,12,6 },
   { (gdFontPtr) NULL,13,7 },
   { (gdFontPtr) NULL,16,8 },
   { (gdFontPtr) NULL,15,9 }
};

#define HIGHSET		                0
#define LOWSET		                1
#define CLOSESET	                2
#define SQUARE_SIZE                     8
#define LEGEND_HORIZ_SPACING            10

/* scaled translation onto graph */
#define PX(x)		                (int)(xorig+(setno*xdepth_3D)+(x)*xscl)
#define PY(y)		                (int)(yorig-(setno*ydepth_3D)+(y)*yscl)
#define PV(y)		                (int)(vyorig-(setno*ydepth_3D)+(y)*vyscl)
#define EPSILON		                ((1.0/256.0)/2.0)
#define GET_DEC(x)	                ((x) - (float)(int)(x))
#define F(x,i)	                        (int)((float)((x)-x1)*slope[i]+(float)y1[i])

#define SET_RECT(gdp,x1,x2,y1,y2)	gdp[0].x = gdp[3].x = x1, \
                                        gdp[0].y = gdp[1].y = y1, \
                                        gdp[1].x = gdp[2].x = x2, \
                                        gdp[2].y = gdp[3].y = y2

#define SET_3D_POLY(gdp,x1,x2,y1,y2,xoff,yoff)  gdp[0].x  = x1,gdp[0].y = y1,		  \
                                                gdp[1].x  = x1+(xoff),gdp[1].y = y1-yoff, \
                                                gdp[2].x  = x2+(xoff),gdp[2].y = y2-yoff, \
                                                gdp[3].x  = x2,gdp[3].y = y2

#define SET_3D_BAR(gdp,x1,x2,y1,y2,xoff,yoff)   gdp[0].x  = x1,gdp[0].y = y1,		  \
                                                gdp[1].x  = x1+(xoff),gdp[1].y = y1-yoff, \
                                                gdp[2].x  = x2+(xoff),gdp[2].y = y2-yoff, \
                                                gdp[3].x  = x2,gdp[3].y = y2

#define	HYP_DEPTH	                ((double)((GDC->width+GDC->height)/2) *((double)GDC->threed_depth)/100.0)
#define RAD_DEPTH	                ((double)GDC->threed_angle*2*M_PI/360)
#define	NUM_YPOINTS	                (sizeof(ypoints_2f) / sizeof(float))

#define DO_GRID(x1,y1,x2,y2)	        if(GDC->grid) { \
				          gdImageLine(GDC->image,x1,y1,x2, y2,GridColor); \
				          gdImageLine(GDC->image,x2,y2,x2, yh,GridColor); \
                                        } else


struct YS {
    int y1;
    int y2;
    float slope;
    int lnclr;
    int shclr;
};

struct BS {
    float y1;
    float y2;
    int clr;
    int shclr;
};

/*
 * count(natural) substrings(new line sep) \*
 */
static short cnt_nl(char *nstr,int *len)
{
    short c = 1;
    short max_seg_len = 0;
    short tmplen = 0;

    if(!nstr) {
      if(len) *len = 0;
      return 0;
    }
    while(*nstr) {
      if(*nstr == '\n') {
        ++c;
        max_seg_len = MAX(tmplen,max_seg_len);
        tmplen = 0;
      } else
        ++tmplen;
      ++nstr;
    }
    if(len) *len = MAX(tmplen,max_seg_len);
    return c;
}

static GDC_FONT_SZ_T GDCfnt_sz(char *s,enum GDC_font_size gdfontsz,char *ftfont,double ftfptsz,double rad,char **sts)
{
    GDC_FONT_SZ_T rtnval;
    int len;
    char *err = NULL;

#ifdef HAVE_LIBFREETYPE
    if(ftfont && ftfptsz) {
      int brect[8];
      if((err = gdImageStringFT((gdImagePtr) NULL,&brect[0],0,ftfont,ftfptsz,0.0,0,0,s)) == NULL) {
        rtnval.h = brect[1] - brect[7];
        rtnval.w = brect[2] - brect[0];
        if(sts)	*sts = err;
        return rtnval;
      }
    }
#endif
    rtnval.h = cnt_nl(s,&len) * GDC_fontc[gdfontsz].h;
    rtnval.w = len * GDC_fontc[gdfontsz].w;
    if(sts) *sts = err;
    return rtnval;
}

/*
 * gd out a string with '\n's handle FTs(TTFs) and gd fonts
 * gdImageString() draws from the upper left; gdImageStringFT() draws from
 * lower left(one font height,even with '\n's)! >:-| \*
 */
static int GDCImageStringNL(gdImagePtr im,GDC_FONT_T *f,char *ftfont,
		 double ftptsz,double rad,int x,int y,char *str,
		 int clr,GDC_justify_t justify,char **sts)
{
    int retval = 0;
    char *err = NULL;

#ifdef HAVE_LIBFREETYPE
    if(ftfont && ftptsz) {
      int f1hgt = 0;
      double xs,ys;
      double lftptsz = 0.0;
      char *lftfont =(char *) -1;

      if(!f1hgt ||(lftfont != ftfont || lftptsz != ftptsz)) {
	f1hgt = GDCfnt_sz("Aj",0,ftfont,ftptsz,rad,NULL).h;
	xs =(double) f1hgt *sin(rad);
	ys =(double)(f1hgt - 1) * cos(rad);
      }
      x +=(int) xs;
      y +=(int) ys;
      if((err = gdImageStringFT(im,(int *) NULL,clr,ftfont,ftptsz,rad,x,y,str)) == NULL) {
        if(sts) *sts = err;
        return 0;
      } else {
	 retval = 1;
	 x -=(int) xs;
	 y -=(int) ys;
      }
    }
#endif

    {
	int i = -1;
	int len = -1;
	int max_len;
	short strs_num = cnt_nl(str,&max_len);
	unsigned char sub_str[max_len + 1];

	strs_num = -1;
	do {
	  ++i;
	  ++len;
	  sub_str[len] = *(str + i);
	  if(*(str + i) == '\n' || *(str + i) == '\0') {
      	    int xpos;
	    sub_str[len] = '\0';
	    ++strs_num;
	    switch(justify) {
	     case GDC_JUSTIFY_LEFT:
	         xpos = 0;
	         break;
	     case GDC_JUSTIFY_RIGHT:
	         xpos = f->w *(max_len - len);
	         break;
	     case GDC_JUSTIFY_CENTER:
	     default:
	         xpos = f->w *(max_len - len) / 2;
	    }
	    if(rad == 0.0)
	      gdImageString(im,f->f,x + xpos,y +(f->h - 1) * strs_num,sub_str,clr);
	    else
	      gdImageStringUp(im,f->f,x +(f->h - 1) * strs_num,y - xpos,sub_str,clr);
       	    len = -1;
	  }
	}
	while(*(str + i));
    }
    if(sts) *sts = err;
    return retval;
}

static int qcmpr(const void *a,const void *b)
{
    if(((struct YS *) a)->y2 <((struct YS *) b)->y2) return 1;
    if(((struct YS *) a)->y2 >((struct YS *) b)->y2) return -1;
    return 0;
}

static unsigned long _clrallocate(gdImagePtr im,unsigned long rawclr,unsigned long bgc)
{
    unsigned long _gdccfoo1;
    unsigned long _gdccfoo2;

    _gdccfoo2 = (rawclr == GDC_DFLTCOLOR ? _gdcntrst(bgc): rawclr);
    return (_gdccfoo1=gdImageColorExact(im,l2gdcal(_gdccfoo2))) != -1 ? _gdccfoo1:
            gdImageColorsTotal(im) == gdMaxColors ? gdImageColorClosest(im,l2gdcal(_gdccfoo2)) :
            gdImageColorAllocate(im,l2gdcal(_gdccfoo2));
}

static unsigned long _clrshdallocate(gdImagePtr im,unsigned long rawclr,unsigned long bgc)
{
    unsigned long _gdccfoo1;
    unsigned long _gdccfoo2;

    _gdccfoo2 = (rawclr == GDC_DFLTCOLOR ? _gdcntrst(bgc): rawclr);
    return (_gdccfoo1 = gdImageColorExact(im,l2gdshd(_gdccfoo2))) != -1 ? _gdccfoo1 :
            gdImageColorsTotal(im) == gdMaxColors ? gdImageColorClosest(im,l2gdshd(_gdccfoo2)) :
	    gdImageColorAllocate(im,l2gdshd(_gdccfoo2));
}

/*
 * -- convert a float to a printable string,in form: --
 * -- W N/D --
 * -- where W is whole,N is numerator,D is denominator --
 * -- the frac N/D is one of 2nds,4,8,16,32,64,128,256ths --
 * -- if cannot convert,return str of the float --
 */

static char *price_to_str(char *buf,float price,int *numorator,int *demoninator,int *decimal,char *fltfmt)
{
    int whole = (int) price;
    float rdec, dec = GET_DEC(price),numr;

    *decimal = whole;
    *numorator = *demoninator = 0;

    if(fltfmt && *fltfmt) {
      sprintf(buf,fltfmt,price);
      return buf;
    }

    numr = dec * 256;
    /* check if we have a perfect fration in 256ths */
    rdec = GET_DEC(numr);
    if(rdec < EPSILON);	/* close enough to frac */
    else
    if((1 - rdec) < EPSILON)	/* just over but close enough */
      ++numr;
    else {			/* no frac match */
      sprintf(buf,"%f",price);
      return buf;
    }
    /* now have numr 256ths resolve down */
    if(numr != 0) {
      int cnt = 8;
      while((float)(numr) / 2.0 == (float)(int)(numr / 2)) {
	numr /= 2;
	--cnt;
      }
      /* don't want both whole AND numerator to be - */
      if(whole < 0 && numr < 0.0) numr = -numr;
      *numorator = (int) numr;
      *demoninator = (int) pow(2.0,(float) cnt);
      sprintf(buf,"%d %d/%d",whole,(int) numr,*demoninator);
    } else {
      sprintf(buf,"%d",whole);
    }
    return buf;
}

static void draw_3d_line(gdImagePtr im,int y0,int x1,int x2,int y1[],int y2[],int xdepth,int ydepth,int num_sets,unsigned long clr[],unsigned long clrshd[])
{
    float depth_slope =	xdepth == 0 ? FLT_MAX :(float) ydepth /(float) xdepth;
    float slope[num_sets];
    struct YS ypts[num_sets];
    int i;
    int x;
    gdPoint poly[4];

    for(i = 0; i < num_sets; ++i)
      slope[i] = x2 == x1 ? FLT_MAX :(float)(y2[i] - y1[i]) /(float)(x2 - x1);

    for(x = x1 + 1; x <= x2; ++x) {
      for(i = 0; i < num_sets; ++i) {	/* load set of points */
        ypts[i].y1 = F(x - 1,i);
        ypts[i].y2 = F(x,i);
        ypts[i].lnclr = clr[i];
        ypts[i].shclr = clrshd[i];
        ypts[i].slope = slope[i];
      }
      qsort(ypts,num_sets,sizeof(struct YS),qcmpr);
      /* put out in that order */
      for(i = 0; i < num_sets; ++i) {	/* top */
        SET_3D_POLY(poly,x - 1,x,ypts[i].y1,ypts[i].y2,xdepth,ydepth);
        gdImageFilledPolygon(im,poly,4,-ypts[i].slope > depth_slope ? ypts[i].shclr : ypts[i].lnclr);
        if(x == x1 + 1)	gdImageLine(im,x - 1,ypts[i].y2,x - 1 + xdepth,ypts[i].y2 - ydepth,-ypts[i].slope <= depth_slope ? ypts[i].shclr : ypts[i].lnclr);
      }
    }
}

static void draw_3d_area(gdImagePtr im,int x1,int x2,int y0,int y1,int y2,int xdepth,int ydepth,int clr,int clrshd)
{

    gdPoint poly[4];
    int y_intercept = 0;	/* if xdepth || ydepth */

    if(xdepth || ydepth) {
      float line_slope = x2 == x1 ? FLT_MAX :(float) -(y2 - y1) /(float)(x2 - x1);
      float depth_slope = xdepth == 0 ? FLT_MAX :(float) ydepth /(float) xdepth;

      y_intercept = (y1 > y0 && y2 < y0) || (y1 < y0 && y2 > y0) ?(int)((1.0 / ABS(line_slope)) *(float)(ABS(y1 - y0))) + x1 : 0;

      /* edging along y0 depth */
      gdImageLine(im,x1 + xdepth,y0 - ydepth,x2 + xdepth,y0 - ydepth,clrshd);
      SET_3D_POLY(poly,x1,x2,y1,y2,xdepth,ydepth);	/* top */
      gdImageFilledPolygon(im,poly,4,line_slope > depth_slope ? clrshd : clr);
      SET_3D_POLY(poly,x1,x2,y0,y0,xdepth,ydepth + 1);	/* along y axis */
      gdImageFilledPolygon(im,poly,4,clr);
      SET_3D_POLY(poly,x2,x2,y0,y2,xdepth,ydepth);	/* side */
      gdImageFilledPolygon(im,poly,4,clrshd);
      if(y_intercept) gdImageLine(im,y_intercept,y0,y_intercept + xdepth,y0 - ydepth,clrshd);
      gdImageLine(im,x1,y0,x1 + xdepth,y0 - ydepth,clrshd);	/* edging */
      gdImageLine(im,x2,y0,x2 + xdepth,y0 - ydepth,clrshd);	/* edging */
      gdImageLine(im,x1,y1,x1 + xdepth,y1 - ydepth,clrshd);	/* edging */
      gdImageLine(im,x2,y2,x2 + xdepth,y2 - ydepth,clrshd);	/* edging */
    }

    if(y1 == y2)		/* bar rect */
      SET_RECT(poly,x1,x2,y0,y1);	/* front */
    else {
      poly[0].x = x1;
      poly[0].y = y0;
      poly[1].x = x2;
      poly[1].y = y0;
      poly[2].x = x2;
      poly[2].y = y2;
      poly[3].x = x1;
      poly[3].y = y1;
    }
    gdImageFilledPolygon(im,poly,4,clr);
    gdImageLine(im,x1,y0,x2,y0,clrshd);
    if((xdepth || ydepth) && (y1 < y0 || y2 < y0)) {	/* and only above y0 */
      if(y1 > y0 && y2 < y0)	/* line crosses from below y0 */
	gdImageLine(im,y_intercept,y0,x2,y2,clrshd);
      else
      if(y1 < y0 && y2 > y0)	/* line crosses from above y0 */
        gdImageLine(im,x1,y1,y_intercept,y0,clrshd);
      else			/* completely above */
        gdImageLine(im,x1,y1,x2,y2,clrshd);
    }
}

static void draw_3d_bar(gdImagePtr im,int x1,int x2,int y0,int yhigh,int xdepth,int ydepth,int clr,int clrshd)
{
    gdPoint poly[4];
    int usd = MIN(y0,yhigh);	/* up-side-down bars */

    if(xdepth || ydepth) {
      if(y0 != yhigh) {	/* 0 height? */
        SET_3D_BAR(poly,x2,x2,y0,yhigh,xdepth,ydepth);	/* side */
        gdImageFilledPolygon(im,poly,4,clrshd);
      }
      SET_3D_BAR(poly,x1,x2,usd,usd,xdepth,ydepth);	/* top */
      gdImageFilledPolygon(im,poly,4,clr);
    }
    SET_RECT(poly,x1,x2,y0,yhigh);	/* front */
    gdImageFilledPolygon(im,poly,4,clr);
    if(xdepth || ydepth) gdImageLine(im,x1,usd,x2,usd,clrshd);
}

static int barcmpr(const void *a,const void *b)
{
    if(((struct BS *) a)->y2 <((struct BS *) b)->y2)
	return -1;
    if(((struct BS *) a)->y2 >((struct BS *) b)->y2)
	return 1;
    return 0;
}

/* simple two-point linear interpolation, attempts between first,then nearest */
static void do_interpolations(int num_points,int interp_point,float vals[])
{
    int i,j;
    float v1 = GDC_NOVALUE,v2 = GDC_NOVALUE;
    int p1 = -1,p2 = -1;

    /* find backwards */
    for(i = interp_point - 1; i >= 0 && p1 == -1; --i)
      if(vals[i] != GDC_NOVALUE && vals[i] != GDC_INTERP_VALUE) {
        v1 = vals[i];
        p1 = i;
      }
    /* find forwards */
    for(j = interp_point + 1; j < num_points && p2 == -1; ++j)
      if(vals[j] != GDC_NOVALUE && vals[j] != GDC_INTERP_VALUE) {
        v2 = vals[j];
        p2 = j;
      }
    /* no forward sample,find backwards */
    for(; i >= 0 && p2 == -1; --i)
      if(vals[i] != GDC_NOVALUE && vals[i] != GDC_INTERP_VALUE) {
        v2 = vals[i];
        p2 = i;
      }
    /* no backwards sample,find forwards */
    for(; j < num_points && p1 == -1; ++j)
      if(vals[j] != GDC_NOVALUE && vals[j] != GDC_INTERP_VALUE) {
        v1 = vals[j];
        p1 = j;
      }
    if(p1 == -1 || p2 == -1 || p1 == p2) {
      vals[interp_point] = GDC_NOVALUE;
      return;
    }
    /* Point-slope formula */
    vals[interp_point] = ((v2 - v1) /(float)(p2 - p1)) *(float)(interp_point - p1) + v1;
}

GDC_T *GDC_alloc()
{
    GDC_T *gdc = ns_calloc(1,sizeof(GDC_T));

    gdc->type = GDC_LINE;
    gdc->width = 400;
    gdc->height = 300;
    gdc->image_type = GDC_PNG;
    gdc->jpeg_quality = -1;
    gdc->legend = -1;
    gdc->hold = GDC_DESTROY_IMAGE;
    gdc->title_size = GDC_MEDBOLD;
    gdc->ytitle_size = GDC_MEDBOLD;
    gdc->xtitle_size = GDC_MEDBOLD;
    gdc->yaxisfont_size = GDC_SMALL;
    gdc->xaxisfont_size = GDC_SMALL;
    gdc->xaxis_angle = 90.0;
    gdc->xlabel_spacing = 5;
    gdc->ylabel_density = 80;
    gdc->requested_ymin = GDC_NOVALUE;
    gdc->requested_ymax = GDC_NOVALUE;
    gdc->requested_yinterval = GDC_NOVALUE;
    gdc->Shelf = TRUE;
    gdc->grid = GDC_TICK_LABELS;
    gdc->ticks = GDC_TICK_LABELS;
    gdc->xaxis = TRUE;
    gdc->yaxis = TRUE;
    gdc->yaxis2 = TRUE;
    gdc->yval_style = TRUE;
    gdc->stack_type = GDC_STACK_DEPTH;
    gdc->threed_depth = 5.0;
    gdc->threed_angle = 45;
    gdc->bar_width = 75;
    gdc->HLC_style = GDC_HLC_CLOSE_CONNECTED;
    gdc->HLC_cap_width = 25;
    gdc->annotation_font_size = GDC_SMALL;
    gdc->border = GDC_BORDER_ALL;
    gdc->BGColor = 0xFFFFFFL;
    gdc->GridColor = 0xA0A0A0L;
    gdc->ScatterColor = 0x8080FFL;
    gdc->LineColor = 0x000000;
    gdc->PlotColor = GDC_DFLTCOLOR;
    gdc->VolColor = 0xA0A0FFL;
    gdc->TitleColor = GDC_DFLTCOLOR;
    gdc->XTitleColor = GDC_DFLTCOLOR;
    gdc->YTitleColor = GDC_DFLTCOLOR;
    gdc->YTitle2Color = GDC_DFLTCOLOR;
    gdc->XLabelColor = GDC_DFLTCOLOR;
    gdc->YLabelColor = GDC_DFLTCOLOR;
    gdc->YLabel2Color = GDC_DFLTCOLOR;
    gdc->LegendColor = 0x000000;
    return gdc;
}

void GDC_free(GDC_T *gdc)
{
    int i;
    if(!gdc) return;

    ns_free(gdc->scatter);
    ns_free(gdc->data);
    for(i = 0;i < gdc->num_sets;i++) ns_free(gdc->dlabels[i]);
    ns_free(gdc->dlabels);
    for(i = 0;i < gdc->num_points;i++) ns_free(gdc->xlabels[i]);
    ns_free(gdc->xlabels);
    ns_free(gdc->ytitle);
    ns_free(gdc->xtitle);
    ns_free(gdc->ytitle2);
    ns_free(gdc->title);
    ns_free(gdc->title_font);
    ns_free(gdc->ytitle_font);
    ns_free(gdc->xtitle_font);
    ns_free(gdc->yaxis_font);
    ns_free(gdc->xaxis_font);
    ns_free(gdc->ylabel_fmt);
    ns_free(gdc->ylabel2_fmt);
    ns_free(gdc->xlabel_ctl);
    ns_free(gdc->annotation_font);
    ns_free(gdc->ExtVolColor);
    ns_free(gdc->SetColor);
    ns_free(gdc->ExtColor);
    ns_free(gdc->BGImage);
    if(gdc->image) gdImageDestroy(gdc->image);
    ns_free(gdc);
}

int GDC_graph(GDC_T *GDC)
{
    int i,j;
    int graphwidth;
    int grapheight;
    char set_depth;
    gdImagePtr bg_img = 0;
    int num_sets = GDC->num_sets;
    float xorig,yorig,vyorig = 0;
    float yscl = 0.0;
    float vyscl = 0.0;
    float xscl = 0.0;
    float vhighest = -FLT_MAX;
    float vlowest = FLT_MAX;
    float highest = -FLT_MAX;
    float lowest = FLT_MAX;
    char do_vol = (GDC->type == GDC_COMBO_HLC_BAR ||
		   GDC->type == GDC_COMBO_HLC_AREA ||
		   GDC->type == GDC_COMBO_LINE_BAR ||
		   GDC->type == GDC_COMBO_LINE_AREA ||
		   GDC->type == GDC_COMBO_LINE_LINE ||
		   GDC->type == GDC_3DCOMBO_HLC_BAR ||
		   GDC->type == GDC_3DCOMBO_HLC_AREA ||
		   GDC->type == GDC_3DCOMBO_LINE_BAR ||
		   GDC->type == GDC_3DCOMBO_LINE_AREA ||
		   GDC->type == GDC_3DCOMBO_LINE_LINE);
    char threeD = (GDC->type == GDC_3DAREA ||
		   GDC->type == GDC_3DLINE ||
		   GDC->type == GDC_3DBAR ||
		   GDC->type == GDC_3DFLOATINGBAR ||
		   GDC->type == GDC_3DHILOCLOSE ||
		   GDC->type == GDC_3DCOMBO_HLC_BAR ||
		   GDC->type == GDC_3DCOMBO_HLC_AREA ||
		   GDC->type == GDC_3DCOMBO_LINE_BAR ||
		   GDC->type == GDC_3DCOMBO_LINE_AREA ||
		   GDC->type == GDC_3DCOMBO_LINE_LINE);
    char do_bar = (GDC->type == GDC_3DBAR ||
                   GDC->type == GDC_BAR ||
                   GDC->type == GDC_3DFLOATINGBAR ||
                   GDC->type == GDC_FLOATINGBAR);
    char do_ylbl_fractions = (!GDC->ylabel_fmt || strlen(GDC->ylabel_fmt) == strcspn(GDC->ylabel_fmt,"%geEfF"));
    float ylbl_interval = 0.0;
    int xlbl_hgt = 0;
    int xdepth_3Dtotal = 0;
    int ydepth_3Dtotal = 0;
    int xdepth_3D = 0;		/* affects PX() */
    int ydepth_3D = 0;		/* affects PY() and PV() */
    int hlf_barwdth = 0;	/* half bar widths */
    int hlf_hlccapwdth = 0;	/* half cap widths for HLC_I_CAP and DIAMOND */
    int annote_len = 0;
    int annote_hgt = 0;
    int setno = 0;		/* affects PX() and PY() */
    float *uvol = 0;
    unsigned long BGColor = 0,LineColor = 0,PlotColor = 0;
    unsigned long GridColor = 0,VolColor = 0,AnnoteColor = 0;
    float *uvals[num_sets];
    unsigned long ExtVolColor[GDC->num_points];
    unsigned long ExtColor[num_sets][GDC->num_points];
    unsigned long ExtColorShd[threeD ? num_sets : 1][threeD ? GDC->num_points : 1];

#ifdef HAVE_LIBFREETYPE
    char *gdc_title_font = GDC->title_font;	/* for convienience  */
    char *gdc_ytitle_font = GDC->ytitle_font;	/* in func calls */
    char *gdc_xtitle_font = GDC->xtitle_font;
    char *gdc_xaxis_font = GDC->xaxis_font;
    double gdc_title_ptsize = GDC->title_ptsize;
    double gdc_ytitle_ptsize = GDC->ytitle_ptsize;
    double gdc_xtitle_ptsize = GDC->xtitle_ptsize;
    double gdc_xaxis_ptsize = GDC->xaxis_ptsize;
    double gdc_xaxis_rad = TO_RAD(GDC->xaxis_angle);
    char *gdc_annotation_font = GDC->annotation_font;
    double gdc_annotation_ptsize = GDC->annotation_ptsize;

#else
    char *gdc_title_font = NULL;
    char *gdc_ytitle_font = NULL;
    char *gdc_xtitle_font = NULL;
    char *gdc_xaxis_font = NULL;
    double gdc_title_ptsize = 0.0;
    double gdc_ytitle_ptsize = 0.0;
    double gdc_xtitle_ptsize = 0.0;
    double gdc_xaxis_ptsize = 0.0;
    double gdc_xaxis_rad = GDC->xaxis_angle == 90.0 ? M_PI / 2.0 : 0.0;
    char *gdc_annotation_font = NULL;
    double gdc_annotation_ptsize = 0.0;
#endif
    double sin_xangle = 1.0,cos_xangle = 0.0;

    /* sanity checks */
    if(GDC->num_sets <= 0 || GDC->num_points <= 0 || GDC->width <= 0 || GDC->height <= 0) {
      Ns_Log(Error,"num_sets=%d, num_points=%d, width=%d, height=%d",GDC->num_sets,GDC->num_points,GDC->width,GDC->height);
      return -1;
    }
    if(GDC->type == GDC_HILOCLOSE ||
       GDC->type == GDC_3DHILOCLOSE ||
       GDC->type == GDC_3DCOMBO_HLC_BAR ||
       GDC->type == GDC_3DCOMBO_HLC_AREA ||
       GDC->type == GDC_COMBO_HLC_BAR ||
       GDC->type == GDC_COMBO_HLC_AREA) {
      if(GDC->num_sets != 4) {
        Ns_Log(Error,"GDC_grapth: 4 data sets required, last one combo set");
        return -1;
      }
    }
    if(GDC->type == GDC_FLOATINGBAR || GDC->type == GDC_3DFLOATINGBAR) {
      if(GDC->num_sets != 2) {
        Ns_Log(Error,"GDC_grapth: 2 data sets required");
        return -1;
      }
    }
    /* Last data set is combo data */
    if(do_vol) {
      if(GDC->num_sets < 2) {
        Ns_Log(Error,"GDC_grath: 2 data sets requred, last one is combo set");
        return -2;
      }
      num_sets--;
      uvol = GDC->data + num_sets * GDC->num_points;
    }
    /* Initialize data sets and/or options */
    set_depth = (GDC->stack_type == GDC_STACK_DEPTH) ? num_sets : 1;
    for(i = 0; i < num_sets; ++i) uvals[i] = GDC->data + i * GDC->num_points;
    if(GDC->thumbnail) GDC->grid = FALSE,GDC->xaxis = FALSE,GDC->yaxis = FALSE;

    /* Initialize fonts */
    GDC_fontc[GDC_pad].f = gdFontTiny;
    GDC_fontc[GDC_TINY].f = gdFontTiny;
    GDC_fontc[GDC_SMALL].f = gdFontSmall;
    GDC_fontc[GDC_MEDBOLD].f = gdFontMediumBold;
    GDC_fontc[GDC_LARGE].f = gdFontLarge;
    GDC_fontc[GDC_GIANT].f = gdFontGiant;

    /* ----- calculate interpretations first ----- */
    if(GDC->interpolations) {
      for(i = 0; i < num_sets; ++i)
        for(j = 0; j < GDC->num_points; ++j)
      	  if(uvals[i][j] == GDC_INTERP_VALUE) do_interpolations(GDC->num_points,j,uvals[i]);
      if(do_vol)
	for(j = 0; j < GDC->num_points; ++j)
	  if(uvol[j] == GDC_INTERP_VALUE) do_interpolations(GDC->num_points,j,uvol);
    }

    /* ----- highest & lowest values ----- */
    if(GDC->stack_type == GDC_STACK_SUM)	/* need to walk sideways */
      for(j = 0; j < GDC->num_points; ++j) {
	float set_sum = 0.0;
	for(i = 0; i < num_sets; ++i)
	  if(uvals[i][j] != GDC_NOVALUE) {
	    set_sum += uvals[i][j];
	    highest = MAX(highest,set_sum);
	    lowest = MIN(lowest,set_sum);
	  }
    } else
    if(GDC->stack_type == GDC_STACK_LAYER)	/* need to walk sideways */
      for(j = 0; j < GDC->num_points; ++j) {
	float neg_set_sum = 0.0,pos_set_sum = 0.0;
	for(i = 0; i < num_sets; ++i)
	  if(uvals[i][j] != GDC_NOVALUE) {
	    if(uvals[i][j] < 0.0)
	      neg_set_sum += uvals[i][j];
	    else
	      pos_set_sum += uvals[i][j];
          }
	  lowest = MIN(lowest,MIN(neg_set_sum,pos_set_sum));
	  highest = MAX(highest,MAX(neg_set_sum,pos_set_sum));
    } else
      for(i = 0; i < num_sets; ++i)
        for(j = 0; j < GDC->num_points; ++j)
       	  if(uvals[i][j] != GDC_NOVALUE) {
	    highest = MAX(uvals[i][j],highest);
	    lowest = MIN(uvals[i][j],lowest);
	  }
    if(GDC->scatter)
      for(i = 0; i < GDC->num_scatter_pts; ++i) {
        highest = MAX((GDC->scatter + i)->val,highest);
        lowest = MIN((GDC->scatter + i)->val,lowest);
      }
    if(do_vol) {		/* for now only one combo set allowed */
      for(j = 0; j < GDC->num_points; ++j)
        if(uvol[j] != GDC_NOVALUE) {
          vhighest = MAX(uvol[j],vhighest);
	  vlowest = MIN(uvol[j],vlowest);
	}
      if(vhighest == -FLT_MAX) vhighest = 1.0;
      if(vlowest == FLT_MAX) vlowest = 0.0;
      if(GDC->type == GDC_COMBO_LINE_BAR ||
         GDC->type == GDC_COMBO_HLC_BAR ||
         GDC->type == GDC_COMBO_LINE_AREA ||
         GDC->type == GDC_COMBO_HLC_AREA ||
         GDC->type == GDC_3DCOMBO_LINE_BAR ||
         GDC->type == GDC_3DCOMBO_LINE_AREA ||
         GDC->type == GDC_3DCOMBO_HLC_BAR ||
         GDC->type == GDC_3DCOMBO_HLC_AREA) {
        if(vhighest < 0.0) vhighest = 0.0; else
        if(vlowest > 0.0) vlowest = 0.0;
      }
    }
    if(lowest == FLT_MAX) lowest = 0.0;
    if(highest == -FLT_MAX) highest = 1.0;
    if(GDC->type == GDC_AREA || GDC->type == GDC_LINEAREA ||
       GDC->type == GDC_BAR || GDC->type == GDC_3DBAR ||
       GDC->type == GDC_3DAREA) {
      if(highest < 0.0) highest = 0.0; else
      if(lowest > 0.0) lowest = 0.0;
    }
    if(GDC->requested_ymin != GDC_NOVALUE && GDC->requested_ymin < lowest) lowest = GDC->requested_ymin;
    if(GDC->requested_ymax != GDC_NOVALUE && GDC->requested_ymax > highest) highest = GDC->requested_ymax;

    /* ----- graph height and width within the img height width ----- */
    /* grapheight/height is the actual size of the scalable graph */
    {
      int title_hgt = GDC->title ? 2 + GDCfnt_sz(GDC->title,GDC->title_size,gdc_title_font,gdc_title_ptsize,0.0,NULL).h + 2 : 2;
      int xlabel_hgt = 0;
      int xtitle_hgt = GDC->xtitle ? 1 + GDCfnt_sz(GDC->xtitle,GDC->xtitle_size,gdc_xtitle_font,gdc_xtitle_ptsize,0.0,NULL).h + 1 : 0;
      int ytitle_hgt = GDC->ytitle ? 1 + GDCfnt_sz(GDC->ytitle,GDC->ytitle_size,gdc_ytitle_font,gdc_ytitle_ptsize,M_PI / 2.0,NULL).h + 1 : 0;
      int vtitle_hgt = do_vol && GDC->ytitle2 ? 1 + GDCfnt_sz(GDC->ytitle2,GDC->ytitle_size,gdc_ytitle_font,gdc_ytitle_ptsize,M_PI / 2.0,NULL).h + 1 : 0;
      int ylabel_wth = 0;
      int vlabel_wth = 0;
      int xtics = GDC->ticks &&(GDC->grid || GDC->xaxis) ? 1 + 2 : 0;
      int ytics = GDC->ticks &&(GDC->grid || GDC->yaxis) ? 1 + 3 : 0;
      int vtics = GDC->ticks &&(GDC->yaxis && do_vol) ? 3 + 1 : 0;

      xdepth_3D = threeD ?(int)(cos(RAD_DEPTH) * HYP_DEPTH) : 0;
      ydepth_3D = threeD ?(int)(sin(RAD_DEPTH) * HYP_DEPTH) : 0;
      xdepth_3Dtotal = xdepth_3D * set_depth;
      ydepth_3Dtotal = ydepth_3D * set_depth;
      annote_hgt = GDC->annotation.note[0] ? 1 + (1 + GDCfnt_sz(GDC->annotation.note,GDC->annotation_font_size,gdc_annotation_font,gdc_annotation_ptsize,0.0,NULL).h) + 1 + 2 : 0;
      annote_len = GDC->annotation.note[0] ? GDCfnt_sz(GDC->annotation.note,GDC->annotation_font_size,gdc_annotation_font,gdc_annotation_ptsize,0.0,NULL).w : 0;

      /* find length of "longest"(Y) xaxis label */
      /* find the average "width"(X) xaxis label */
      /*      avg method fails when 2 or 3 very wide are consecutive,with the rest being thin */
      /*      this is most evident with horizontal(0deg) xlabels */
      /*      assume in this case they are quite uniform,e.g.,dates */
      /* find affects on graphwidth/xorig of wildly overhanging angled labels */
      if(GDC->xaxis && GDC->xlabels) {
	int biggest = -INT_MAX,widest = -INT_MAX;
#ifdef HAVE_LIBFREETYPE
        if(gdc_xaxis_rad != M_PI / 2.0 && gdc_xaxis_font && gdc_xaxis_ptsize)
          sin_xangle = sin(gdc_xaxis_rad),cos_xangle = cos(gdc_xaxis_rad);
#endif

        for(i = 0; i < GDC->num_points; ++i) {
	  int len = 0,wdth = 0;
	  if(!GDC->xlabel_ctl || (GDC->xlabel_ctl && GDC->xlabel_ctl[i])) {
	    char *sts;
	    GDC_FONT_SZ_T lftsz = GDCfnt_sz(GDC->xlabels[i],GDC->xaxisfont_size,gdc_xaxis_font,gdc_xaxis_ptsize,gdc_xaxis_rad,&sts);
     	    if(gdc_xaxis_rad == M_PI / 2.0 || (sts && *sts)) {
	      len = lftsz.w;
	      wdth = lftsz.h;
	    } else
            if(gdc_xaxis_rad == 0.0) {	/* protect /0 *//* reverse when horiz. */
	      len = lftsz.h;
	      wdth = lftsz.w;
	    } else {	/* length & width due to the angle */
       	      len = (int)((float) lftsz.w * sin_xangle + (float) lftsz.h * cos_xangle);
      	      wdth =(int)((float) lftsz.h / sin_xangle);
	    }
	  }
	  biggest = MAX(len,biggest);	/* last seg */
	  widest = MAX(wdth,widest);	/* last seg */
	}
	xlbl_hgt = 1 + widest + 1;
	xlabel_hgt = 1 + biggest + 1;
      }
      grapheight = GDC->height - (xtics + xtitle_hgt + xlabel_hgt + title_hgt + annote_hgt + ydepth_3Dtotal + 2);
      if(GDC->hard_size && GDC->hard_grapheight) grapheight = GDC->hard_grapheight;
      GDC->hard_grapheight = grapheight;

      /* ----- y labels intervals ----- */
      {
	/* possible y gridline points */
	static float ypoints_2f[] = {
                     1.0 / 64.0,1.0 / 32.0,1.0 / 16.0,1.0 / 8.0,1.0 / 4.0, 1.0 / 2.0,
     	             1.0,2.0,3.0,5.0,10.0,25.0,50.0,100.0,250.0,500.0,1000.0,2500,5000.0,
                     10000.0,25000.0,50000.0,100000.0,500000.0,1000000,5000000,10000000,50000000 };
        static float ypoints_dec[NUM_YPOINTS] = {
                     0.005,0.01,0.025,0.05,0.1,0.2,0.25,0.5,1.0,2.0,3.0,5.0,10.0,25.0,
           	     50.0,100.0,250.0,500.0,1000.0,2500,5000.0,10000.0,25000.0,50000.0,
                     100000.0,500000.0,1000000,5000000 };
	float tmp_highest;
        float *ypoints = do_ylbl_fractions ? ypoints_2f : ypoints_dec;
        float ylbl_density_space_intvl;
        int max_num_ylbls,longest_ylblen = 0;
        int nmrtr,dmntr,whole,lbl_len;
        char foo[32],pp[32];
        /* maximum y lables that'll fit... */
        max_num_ylbls =	grapheight /(3 + GDC_fontc[GDC->yaxisfont_size == GDC_TINY ? GDC->yaxisfont_size + 1 : GDC->yaxisfont_size].h);
        if(max_num_ylbls < 3) {
          Ns_Log(Error,"GDC_graph: max_num_ylbls < 3");
          return -2;
        }
        /* one "space" interval above + below */
	ylbl_density_space_intvl = (max_num_ylbls - 2.0) * GDC->ylabel_density / 100.0;
	for(i = 1; i < NUM_YPOINTS; ++i) {
          if((highest - lowest) / ypoints[i] < ylbl_density_space_intvl) break;
        }
	ylbl_interval = GDC->requested_yinterval != GDC_NOVALUE &&
                        GDC->requested_yinterval > ypoints[i - 1] ?
                              GDC->requested_yinterval : ypoints[i - 1];

	/* perform floating point remainders */
	/* gronculate largest interval-point < lowest */
	if(lowest != 0.0 && lowest != GDC->requested_ymin) {
	  if(lowest < 0.0) lowest -= ylbl_interval;
	  lowest = ylbl_interval * ((lowest - ypoints[0]) / ylbl_interval);
	}
	/* find smallest interval-point > highest */
	tmp_highest = lowest;
	do {
	  if(GDC->yaxis) {
	    sprintf(foo,do_ylbl_fractions ? "%.0f" : GDC->ylabel_fmt,tmp_highest);
	    lbl_len = ylbl_interval < 1.0 ? strlen(price_to_str(pp,tmp_highest,&nmrtr,&dmntr,&whole,do_ylbl_fractions ? NULL : GDC->ylabel_fmt)) : strlen(foo);
	    longest_ylblen = MAX(longest_ylblen,lbl_len);
	  }
	} while((tmp_highest += ylbl_interval) <= highest);
	ylabel_wth = longest_ylblen * GDC_fontc[GDC->yaxisfont_size].w;
	highest = GDC->requested_ymax == GDC_NOVALUE ? tmp_highest : MAX(GDC->requested_ymax,highest);

        if(do_vol) {
	  float num_yintrvls =(highest - lowest) / ylbl_interval;
	  /* no skyscrapers */
	  if(vhighest != 0.0) vhighest += (vhighest - vlowest) /(num_yintrvls * 2.0);
	  if(vlowest != 0.0) vlowest -=(vhighest - vlowest) /(num_yintrvls * 2.0);

	  if(GDC->yaxis2) {
	    char svlongest[32];
	    int lbl_len_low = sprintf(svlongest,GDC->ylabel2_fmt ? GDC->ylabel2_fmt : "%.0f",vlowest);
	    int lbl_len_high = sprintf(svlongest,GDC->ylabel2_fmt ? GDC->ylabel2_fmt : "%.0f",vhighest);
	    vlabel_wth = 1 + MAX(lbl_len_low,lbl_len_high) * GDC_fontc[GDC->yaxisfont_size].w;
	  }
	}
      }

      graphwidth = GDC->width - (((GDC->hard_size && GDC->hard_xorig) ? GDC->hard_xorig : (ytitle_hgt + ylabel_wth + ytics)) + vtics + vtitle_hgt + vlabel_wth + xdepth_3Dtotal);
      if(GDC->hard_size && GDC->hard_graphwidth) graphwidth = GDC->hard_graphwidth;
      GDC->hard_graphwidth = graphwidth;

      /* ----- scale to img size ----- */
      /* offset to 0 at lower left(where it should be) */
      xscl = (float)(graphwidth - xdepth_3Dtotal) /(float)(GDC->num_points + (do_bar ? 2 : 0));
      yscl = -((float) grapheight) /(float)(highest - lowest);
      if(do_vol) {
	float hilow_diff = vhighest - vlowest == 0.0 ? 1.0 : vhighest - vlowest;
        vyscl = -((float) grapheight) / hilow_diff;
        vyorig =(float) grapheight + ABS(vyscl) * MIN(vlowest,vhighest)	+ ydepth_3Dtotal + title_hgt + annote_hgt;
      }
      xorig =(float)(GDC->width - (graphwidth + vtitle_hgt + vtics + vlabel_wth));
      if(GDC->hard_size && GDC->hard_xorig) xorig = GDC->hard_xorig;
      GDC->hard_xorig = xorig;
      yorig = (float) grapheight + ABS(yscl) * MIN(lowest,highest) + ydepth_3Dtotal + title_hgt + annote_hgt;
      if(GDC->hard_size && GDC->hard_yorig) yorig = GDC->hard_yorig;
      GDC->hard_yorig = yorig;

      hlf_barwdth =(int)((float)(PX(2) - PX(1)) *(((float) GDC->bar_width / 100.0) / 2.0));
      hlf_hlccapwdth = (int)((float)(PX(2) - PX(1)) * (((float) GDC->HLC_cap_width / 100.0) / 2.0));
    }

    /* ----- OK start the graphic ----- */
    if(!(GDC->hold & GDC_REUSE_IMAGE)) {
      if(GDC->image) gdImageDestroy(GDC->image);
      GDC->image = 0;
    }
    if(!GDC->image) GDC->image = gdImageCreate(GDC->width,GDC->height);

    BGColor = gdImageColorAllocate(GDC->image,l2gdcal(GDC->BGColor));
    LineColor = clrallocate(GDC->image,GDC->LineColor);
    PlotColor = clrallocate(GDC->image,GDC->PlotColor);
    GridColor = clrallocate(GDC->image,GDC->GridColor);
    if(do_vol) {
      VolColor = clrallocate(GDC->image,GDC->VolColor);
      for(i = 0; i < GDC->num_points; ++i)
	if(GDC->ExtVolColor)
	  ExtVolColor[i] = clrallocate(GDC->image,GDC->ExtVolColor[i]);
        else
     	  ExtVolColor[i] = VolColor;
    }
    if(GDC->annotation.note[0]) AnnoteColor = clrallocate(GDC->image,GDC->annotation.color);

    /* attempt to import optional background image */
    if(GDC->BGImage) {
      FILE *in = fopen(GDC->BGImage,"rb");
      if(in) {
        /* assume PNG , should determine GDC->type by file extension,option,... */
        if((bg_img = gdImageCreateFromPng(in))) {
          int bgxpos = gdImageSX(bg_img) < GDC->width ? GDC->width / 2 - gdImageSX(bg_img) / 2 : 0;
          int bgypos = gdImageSY(bg_img) < GDC->height ? GDC->height / 2 - gdImageSY(bg_img) / 2 : 0;
          if(gdImageSX(bg_img) > GDC->width || gdImageSY(bg_img) > GDC->height) {	/*  [and center] */
            gdImageCopyResized(GDC->image,bg_img,bgxpos,bgypos,0,0,GDC->width,GDC->height,GDC->width,GDC->height);
          } else		/* just center */
            gdImageCopy(GDC->image,bg_img,bgxpos,bgypos,0,0,GDC->width,GDC->height);
        }
        fclose(in);
      }
    }

    for(j = 0; j < num_sets; ++j) {
      for(i = 0; i < GDC->num_points; ++i)
	if(GDC->ExtColor) {
	  unsigned long ext_clr = *(GDC->ExtColor + GDC->num_points * j + i);
          ExtColor[j][i] = clrallocate(GDC->image,ext_clr);
	  if(threeD) ExtColorShd[j][i] = clrshdallocate(GDC->image,ext_clr);
	} else
        if(GDC->SetColor) {
	  unsigned long set_clr = GDC->SetColor[j];
	  ExtColor[j][i] = clrallocate(GDC->image,set_clr);
	  if(threeD) ExtColorShd[j][i] = clrshdallocate(GDC->image,set_clr);
	} else {
	  ExtColor[j][i] = PlotColor;
	  if(threeD) ExtColorShd[j][i] = clrshdallocate(GDC->image,GDC->PlotColor);
	}
    }

    if(GDC->transparent_bg) gdImageColorTransparent(GDC->image,BGColor);

    if(GDC->title) {
      GDC_FONT_SZ_T tftsz = GDCfnt_sz(GDC->title,GDC->title_size,gdc_title_font,gdc_title_ptsize,0.0,NULL);
      int titlecolor = clrallocate(GDC->image,GDC->TitleColor);
      GDCImageStringNL(GDC->image,&GDC_fontc[GDC->title_size],gdc_title_font,gdc_title_ptsize,0.0,
                       GDC->width / 2 - tftsz.w / 2,1,
		       GDC->title,titlecolor,GDC_JUSTIFY_CENTER,NULL);
    }
    if(GDC->xtitle) {
      GDC_FONT_SZ_T xtftsz = GDCfnt_sz(GDC->xtitle,GDC->xtitle_size,gdc_xtitle_font,gdc_xtitle_ptsize,0.0,NULL);
      int titlecolor = GDC->XTitleColor == GDC_DFLTCOLOR ? PlotColor : clrallocate(GDC->image,GDC->XTitleColor);
      GDCImageStringNL(GDC->image,&GDC_fontc[GDC->xtitle_size],gdc_xtitle_font,gdc_xtitle_ptsize,0.0,
		       GDC->width / 2 - xtftsz.w / 2,
                       GDC->height - 1 - xtftsz.h - 1,
                       GDC->xtitle,titlecolor,GDC_JUSTIFY_CENTER,NULL);
    }
    if(GDC->grid_on_top) goto gdcData;

gdcGrid:
    /* if no grid, on 3D, border needs to handle it */
    if(!GDC->grid && threeD && (GDC->border & (GDC_BORDER_ALL|GDC_BORDER_X|GDC_BORDER_Y))) {
      int x1,x2,y1,y2;
      x1 = PX(0);
      y1 = PY(lowest);
      setno = set_depth;
      x2 = PX(0);
      y2 = PY(lowest);

      gdImageLine(GDC->image,x1,y1,x2,y2,LineColor);	/* depth at origin */
      if((GDC->border == GDC_BORDER_ALL) || (GDC->border & GDC_BORDER_X))
	gdImageLine(GDC->image,x2,y2,PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),y2,LineColor);
      if((GDC->border == GDC_BORDER_ALL) || (GDC->border & GDC_BORDER_Y))
	gdImageLine(GDC->image,x2,PY(highest),x2,y2,LineColor);
      setno = 0;
    }
    if(GDC->grid || GDC->ticks || GDC->yaxis) {	/* grid lines & y label(s) */
      float tmp_y = lowest;
      int labelcolor = GDC->YLabelColor == GDC_DFLTCOLOR ? LineColor : clrallocate(GDC->image,GDC->YLabelColor);
      int label2color = GDC->YLabel2Color == GDC_DFLTCOLOR ? VolColor : clrallocate(GDC->image,GDC->YLabel2Color);

      /* step from lowest to highest puting in labels and grid at interval points */
      /* since now "odd" intervals may be requested,try to step starting at 0,  */
      /* if lowest < 0 < highest                                                  */
      for(i = -1; i <= 1; i += 2) {	/* -1,1 */
	if(i == -1) {
	  if(lowest >= 0.0) continue;
          tmp_y = MIN(0,highest);	/*      step down to lowest */
        }
	if(i == 1) {
	  if(highest <= 0.0) continue;
          tmp_y = MAX(0,lowest);	/*      step up to highest */
        }
	do {
	   int n,d,w;
           char pp[32];
	   char nmrtr[3 + 1],dmntr[3 + 1],whole[8];
	   char all_whole = ylbl_interval < 1.0 ? FALSE : TRUE;
	   char *ylbl_str = price_to_str(pp,tmp_y,&n,&d,&w,do_ylbl_fractions ? 0 : GDC->ylabel_fmt);

	   if(do_ylbl_fractions) {
	     sprintf(nmrtr,"%d",n);
	     sprintf(dmntr,"%d",d);
	     sprintf(whole,"%d",w);
	   }

	   if(GDC->grid || GDC->ticks) {
	     int x1,x2,y1,y2;
     	     /* tics */
	     x1 = PX(0);
	     y1 = PY(tmp_y);
	     if(GDC->ticks) gdImageLine(GDC->image,x1 - 2,y1,x1,y1,GridColor);
	     if(GDC->grid) {
	       setno = set_depth;
	       x2 = PX(0);
	       y2 = PY(tmp_y);	/* w/ new setno */
	       gdImageLine(GDC->image,x1,y1,x2,y2,GridColor);	/* depth for 3Ds */
	       gdImageLine(GDC->image,x2,y2,PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),y2,GridColor);
	       setno = 0;	/* set back to foremost */
	     }
	   }
	   if(GDC->yaxis) {
	     if(do_ylbl_fractions) {
	       if(w || (!w && !n && !d)) {
		 gdImageString(GDC->image,
                               GDC_fontc[GDC->yaxisfont_size].f,
			       PX(0) - 2 - strlen(whole) * GDC_fontc[GDC->yaxisfont_size].w - ((!all_whole) ?
                                   (strlen(nmrtr) * GDC_fontc[GDC->yaxisfont_size - 1].w +
                                   GDC_fontc[GDC->yaxisfont_size].w + strlen(nmrtr) *
                                   GDC_fontc[GDC->yaxisfont_size - 1].w) : 1),
     			       PY(tmp_y) - GDC_fontc[GDC->yaxisfont_size].h / 2,
                               (unsigned char *)whole,
                               labelcolor);
	       }
	       if(n) {
		 gdImageString(GDC->image,
                               GDC_fontc[GDC->yaxisfont_size - 1].f,
			       PX(0) - 2 - strlen(nmrtr) * GDC_fontc[GDC->yaxisfont_size - 1].w -
				  GDC_fontc[GDC->yaxisfont_size].w - strlen(nmrtr) *
                                  GDC_fontc[GDC->yaxisfont_size - 1].w + 1,
	       		       PY(tmp_y) - GDC_fontc[GDC->yaxisfont_size].h / 2 + 1,
                               (unsigned char *) nmrtr,
			       labelcolor);
		 gdImageString(GDC->image,
                               GDC_fontc[GDC->yaxisfont_size].f,
       			       PX(0) - 2 - GDC_fontc[GDC->yaxisfont_size].w - strlen(nmrtr) *
			          GDC_fontc[GDC->yaxisfont_size - 1].w,
			       PY(tmp_y) - GDC_fontc[GDC->yaxisfont_size].h / 2,
                               (unsigned char *) "/",
			       labelcolor);
		 gdImageString(GDC->image,
                               GDC_fontc[GDC->yaxisfont_size - 1].f,
		               PX(0) - 2 - strlen(nmrtr) * GDC_fontc[GDC->yaxisfont_size - 1].w - 2,
			       PY(tmp_y) - GDC_fontc[GDC->yaxisfont_size].h / 2 + 3,
                               (unsigned char *) dmntr,
			       labelcolor);
	       }
	     } else
	       gdImageString(GDC->image,
                             GDC_fontc[GDC->yaxisfont_size].f,
			     PX(0) - 2 - strlen(ylbl_str) * GDC_fontc[GDC->yaxisfont_size].w,
			     PY(tmp_y) - GDC_fontc[GDC->yaxisfont_size].h / 2,
			     (unsigned char *)ylbl_str,
			     labelcolor);

	     if(do_vol && GDC->yaxis2) {
	       char vylbl[16];
	       /* opposite of PV(y) */
	       sprintf(vylbl,GDC->ylabel2_fmt ? GDC->ylabel2_fmt : "%.0f",((float)(PY(tmp_y) +(setno * ydepth_3D) - vyorig)) / vyscl);
      	       setno = set_depth;
	       if(GDC->ticks)
	         gdImageLine(GDC->image,
                             PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),
                             PY(tmp_y),
	       		     PX(GDC->num_points - 1 +(do_bar ? 2 : 0)) + 3,
                             PY(tmp_y),GridColor);
		 if(atof(vylbl) == 0.0)	strcpy(vylbl,"0");
       	         gdImageString(GDC->image,
                               GDC_fontc[GDC->yaxisfont_size].f,
			       PX(GDC->num_points - 1 +(do_bar ? 2 : 0)) + 6,
			       PY(tmp_y) - GDC_fontc[GDC->yaxisfont_size].h / 2,
			       (unsigned char *) vylbl,
                               label2color);
		 setno = 0;
	      }
           }
	} while((i > 0 && ((tmp_y += ylbl_interval) < highest)) || (i < 0 && ((tmp_y -= ylbl_interval) > lowest)));
      }

      /* catch last(bottom) grid line - specific to an "off" requested interval */
      if(GDC->grid && threeD) {
	setno = set_depth;
	gdImageLine(GDC->image,PX(0),PY(lowest),PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),PY(lowest),GridColor);
        setno = 0;		/* set back to foremost */
      }

      /* y axis title */
      if(do_vol && GDC->ytitle2) {
	GDC_FONT_SZ_T ytftsz = GDCfnt_sz(GDC->ytitle2,GDC->ytitle_size,gdc_ytitle_font,gdc_ytitle_ptsize,0.0,NULL);
	int titlecolor = GDC->YTitle2Color == GDC_DFLTCOLOR ? VolColor : clrallocate(GDC->image,GDC->YTitle2Color);
	GDCImageStringNL(GDC->image,&GDC_fontc[GDC->ytitle_size],
			    gdc_ytitle_font,
                            gdc_ytitle_ptsize,
			    M_PI / 2.0,
                            GDC->width -(1 + ytftsz.h),
			    yorig / 2 + ytftsz.w / 2,
                            GDC->ytitle2,
			    titlecolor,
                            GDC_JUSTIFY_CENTER,NULL);
      }

      /* y axis title */
      if(GDC->yaxis && GDC->ytitle) {
	GDC_FONT_SZ_T ytftsz = GDCfnt_sz(GDC->ytitle,GDC->ytitle_size,gdc_ytitle_font,gdc_ytitle_ptsize,0.0,NULL);
        int titlecolor = GDC->YTitleColor == GDC_DFLTCOLOR ? PlotColor : clrallocate(GDC->image,GDC->YTitleColor);
	GDCImageStringNL(GDC->image,&GDC_fontc[GDC->ytitle_size],
	       	            gdc_ytitle_font,
                            gdc_ytitle_ptsize,
			    M_PI / 2.0,1,
                            yorig / 2 + ytftsz.w / 2,
			    GDC->ytitle,
                            titlecolor,
                            GDC_JUSTIFY_CENTER,
			    NULL);
	}
    }
    /* interviening set grids */
    /*  0 < setno < num_sets   non-inclusive,they've already been covered */
    if(GDC->grid && threeD) {
      for(setno = set_depth - 1; setno > 0; --setno) {
	gdImageLine(GDC->image,PX(0),PY(lowest),PX(0),PY(highest),GridColor);
	gdImageLine(GDC->image,PX(0),PY(lowest),PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),PY(lowest),GridColor);
      }
      setno = 0;
    }
    if((GDC->grid || GDC->Shelf) &&
       ((lowest < 0.0 && highest > 0.0) ||
        ((lowest == 0.0 || highest == 0.0) && !(GDC->border & GDC_BORDER_X)))) {
      int x1,x2,y1,y2;
      /* tics */
      x1 = PX(0);
      y1 = PY(0);
      if(GDC->ticks) gdImageLine(GDC->image,x1 - 2,y1,x1,y1,LineColor);
      setno = set_depth;
      x2 = PX(0);
      y2 = PY(0);		/* w/ new setno */
      gdImageLine(GDC->image,x1,y1,x2,y2,LineColor);	/* depth for 3Ds */
      gdImageLine(GDC->image,x2,y2,PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),y2,LineColor);
      setno = 0;		/* set back to foremost */
    }
    /* x ticks and xlables */
    if(GDC->grid || GDC->xaxis) {
      int num_xlbls = graphwidth / ((GDC->xlabel_spacing == SHRT_MAX ? 0 : GDC->xlabel_spacing) + xlbl_hgt);
      int labelcolor = GDC->XLabelColor == GDC_DFLTCOLOR ? LineColor : clrallocate(GDC->image,GDC->XLabelColor);
      for(i = 0; i < GDC->num_points +(do_bar ? 2 : 0); ++i) {
	int xi = do_bar ? i - 1 : i;
	int x1,x2,y1,y2,yh;	/* ticks & grids */
	x1 = PX(i);
	y1 = PY(lowest);
	setno = set_depth;
	x2 = PX(i);
	y2 = PY(lowest);
	yh = PY(highest);
	setno = 0;		/* reset to foremost */

	if(i == 0) {    	/* catch 3D Y back corner */
          if(GDC->grid) {
	    gdImageLine(GDC->image,x1,y1,x2,y2,GridColor);
	    gdImageLine(GDC->image,x2,y2,x2,yh,GridColor);
          }
        }
	/* labeled points */
	if((!GDC->xlabel_ctl && ((i %(1 + GDC->num_points / num_xlbls) == 0) || num_xlbls >= GDC->num_points ||
               GDC->xlabel_spacing == SHRT_MAX)) || (GDC->xlabel_ctl && xi >= 0 && *(GDC->xlabel_ctl + xi))) {
	    /* labeled points tick & grid */
            if(GDC->ticks) {
              gdImageLine(GDC->image,x1,y1,x1,y1+2,GridColor);
            }
            if(GDC->grid) {
              gdImageLine(GDC->image,x1,y1,x2,y2,GridColor);
              gdImageLine(GDC->image,x2,y2,x2,yh,GridColor);
            }
	    if(!do_bar ||(i > 0 && xi < GDC->num_points))
	      if(GDC->xaxis && GDC->xlabels && GDC->xlabels[xi] && *(GDC->xlabels[xi])) {
		char *sts;
		GDC_FONT_SZ_T lftsz = GDCfnt_sz(GDC->xlabels[xi],GDC->xaxisfont_size,gdc_xaxis_font,gdc_xaxis_ptsize,gdc_xaxis_rad,&sts);
		if(gdc_xaxis_rad == M_PI / 2.0 ||(sts && *sts)) {
		  GDCImageStringNL(GDC->image,
                                   &GDC_fontc[GDC->xaxisfont_size],
				   gdc_xaxis_font,
				   gdc_xaxis_ptsize,
                                   M_PI / 2.0,
				   PX(i) - 1 -(lftsz.h / 2),
				   PY(lowest) + 2 + 1 + lftsz.w,
				   GDC->xlabels[xi],
                                   labelcolor,
				   GDC_JUSTIFY_RIGHT,
                                   NULL);
		} else
                if(gdc_xaxis_rad == 0.0)
		  GDCImageStringNL(GDC->image,
                                   &GDC_fontc[GDC->xaxisfont_size],
				   gdc_xaxis_font,
				   gdc_xaxis_ptsize,0.0,
				   PX(i) - 1 -(lftsz.w / 2),
				   PY(lowest) + 2 + 1,GDC->xlabels[xi],
				   labelcolor,
				   GDC_JUSTIFY_CENTER,NULL);
		else
		  GDCImageStringNL(GDC->image,
                                   &GDC_fontc[GDC->xaxisfont_size],
				   gdc_xaxis_font,
				   gdc_xaxis_ptsize,
				   gdc_xaxis_rad,
				   PX(i) - 1 - (int)((double) lftsz.w * cos_xangle +
                                     (double) lftsz.h * gdc_xaxis_rad /(M_PI / 2.0) / 2.0),
				   PY(lowest) + 2 + 1 + (int)((double) lftsz.w * sin_xangle),GDC->xlabels[xi],
				   labelcolor,
                                   GDC_JUSTIFY_RIGHT,
				   NULL);
	      }
	}
	/* every point,on-point */
	if(i > 0) {
	  if(GDC->grid == GDC_TICK_POINTS) {
            if(GDC->grid) {
              gdImageLine(GDC->image,x1,y1,x2,y2,GridColor);
	      gdImageLine(GDC->image,x2,y2,x2,yh,GridColor);
            }
          } else
          if(GDC->grid > GDC_TICK_NONE) {
	      int k;
	      int xt;
	      int prevx = PX(i - 1);
	      int intrv_dist =(x1 - prevx) /(GDC->grid + 1);

              if(GDC->grid) {
	        gdImageLine(GDC->image,x1,y1,x2,y2,GridColor);
                gdImageLine(GDC->image,x2,y2,x2,yh,GridColor);
              }
	      for(k = 0,xt = prevx + intrv_dist;k < GDC->grid && xt < x1; ++k,xt += intrv_dist) {
                if(GDC->grid) {
           	  gdImageLine(GDC->image,xt,y1,xt+xdepth_3Dtotal,y2,GridColor);
	          gdImageLine(GDC->image,xt+xdepth_3Dtotal,y2,xt+xdepth_3Dtotal,yh,GridColor);
                }
              }
	  }
	  if(GDC->ticks == GDC_TICK_POINTS) {      /* --- TICKS --- */
            if(GDC->ticks) gdImageLine(GDC->image,x1,y1,x1,y1+2,GridColor);
	  } else
          if(GDC->ticks > GDC_TICK_NONE) {
	    int k;
	    int xt;
	    int prevx = PX(i - 1);
	    int intrv_dist =(x1 - prevx) /(GDC->ticks + 1);
            if(GDC->ticks) {
              gdImageLine(GDC->image,x1,y1,x1,y1+2,GridColor);
            }
            for(k = 0,xt = prevx + intrv_dist;k < GDC->ticks && xt < x1; ++k,xt += intrv_dist)
              if(GDC->ticks) gdImageLine(GDC->image,xt,y1,xt,y1+2,GridColor);
	  }
	}
      }
    }
    /* ----- volume plotting ----- */
    if(do_vol) {
      setno = set_depth;
      if(GDC->type == GDC_COMBO_HLC_BAR ||
	 GDC->type == GDC_COMBO_LINE_BAR ||
	 GDC->type == GDC_3DCOMBO_LINE_BAR ||
         GDC->type == GDC_3DCOMBO_HLC_BAR) {
	if(uvol[0] != GDC_NOVALUE)
	  draw_3d_bar(GDC->image,PX(0),PX(0) + hlf_barwdth,PV(0),PV(uvol[0]),0,0,ExtVolColor[0],ExtVolColor[0]);
	  for(i = 1; i < GDC->num_points - 1; ++i)
	    if(uvol[i] != GDC_NOVALUE)
	      draw_3d_bar(GDC->image,PX(i) - hlf_barwdth,PX(i) + hlf_barwdth,PV(0),PV(uvol[i]),0,0,ExtVolColor[i],ExtVolColor[i]);
	  if(uvol[i] != GDC_NOVALUE)
	    draw_3d_bar(GDC->image,PX(i) - hlf_barwdth,PX(i),PV(0),PV(uvol[i]),0,0,ExtVolColor[i],ExtVolColor[i]);
      } else
	if(GDC->type == GDC_COMBO_HLC_AREA ||
	   GDC->type == GDC_COMBO_LINE_AREA ||
	   GDC->type == GDC_3DCOMBO_LINE_AREA ||
	   GDC->type == GDC_3DCOMBO_HLC_AREA) {
	  for(i = 1; i < GDC->num_points; ++i)
	    if(uvol[i - 1] != GDC_NOVALUE && uvol[i] != GDC_NOVALUE)
	      draw_3d_area(GDC->image,PX(i - 1),PX(i),PV(0),PV(uvol[i - 1]),PV(uvol[i]),0,0,ExtVolColor[i],ExtVolColor[i]);
	} else
	  if(GDC->type == GDC_COMBO_LINE_LINE || GDC->type == GDC_3DCOMBO_LINE_LINE) {
	    for(i = 1; i < GDC->num_points; ++i)
	      if(uvol[i - 1] != GDC_NOVALUE && uvol[i] != GDC_NOVALUE)
		gdImageLine(GDC->image,PX(i - 1),PV(uvol[i - 1]),PX(i),PV(uvol[i]),ExtVolColor[i]);
	}
	setno = 0;
    }
    /* volume polys done */
    if(GDC->annotation.note[0] && threeD) {	/* back half of annotation line */
      int x1 = PX(GDC->annotation.point +(do_bar ? 1 : 0)),
	  y1 = PY(lowest);
      setno = set_depth;
      gdImageLine(GDC->image,x1,y1,
                  PX(GDC->annotation.point +(do_bar ? 1 : 0)),
                  PY(lowest),AnnoteColor);
      gdImageLine(GDC->image,
                  PX(GDC->annotation.point +(do_bar ? 1 : 0)),
                  PY(lowest),
  		  PX(GDC->annotation.point +(do_bar ? 1 : 0)),
                  PY(highest) - 2,
                  AnnoteColor);
      setno = 0;
    }
    if(GDC->grid_on_top) goto gdcFinish;
    /* ---------- start plotting the data ---------- */
gdcData:
    switch(GDC->type) {
    case GDC_3DBAR:		/* depth,width,y interval need to allow for whitespace between bars */
    case GDC_BAR:
	switch(GDC->stack_type) {
        case GDC_STACK_SUM:
            break;
	case GDC_STACK_DEPTH:
	    for(setno = num_sets - 1; setno >= 0; --setno)	/* back sets first   PX,PY depth */
	      for(i = 0; i < GDC->num_points; ++i)
		if(uvals[setno][i] != GDC_NOVALUE)
		  draw_3d_bar(GDC->image,
                              PX(i +(do_bar ? 1 : 0)) - hlf_barwdth,
			      PX(i +(do_bar ? 1 : 0)) + hlf_barwdth,
			      PY(0),
                              PY(uvals[setno][i]),
                              xdepth_3D,
			      ydepth_3D,
                              ExtColor[setno][i],
			      threeD ? ExtColorShd[setno][i] : ExtColor[setno][i]);
	    setno = 0;
	    break;

	case GDC_STACK_LAYER: {
	    int j = 0;
	    struct BS barset[num_sets];
	    for(i = 0; i < GDC->num_points; ++i) {
	      float lasty_pos = 0.0;
	      float lasty_neg = 0.0;
	      int k;
	      for(j = 0,k = 0; j < num_sets; ++j) {
		if(uvals[j][i] != GDC_NOVALUE) {
		  if(uvals[j][i] < 0.0) {
		    barset[k].y1 = lasty_neg;
		    barset[k].y2 = uvals[j][i] + lasty_neg;
		    lasty_neg = barset[k].y2;
		  } else {
		    barset[k].y1 = lasty_pos;
		    barset[k].y2 = uvals[j][i] + lasty_pos;
		    lasty_pos = barset[k].y2;
		  }
		  barset[k].clr = ExtColor[j][i];
		  barset[k].shclr = threeD ? ExtColorShd[j][i] : ExtColor[j][i];
		  ++k;
		}
	      }
	      qsort(barset,k,sizeof(struct BS),barcmpr);
	      for(j = 0; j < k; ++j) {
		draw_3d_bar(GDC->image,
                            PX(i +(do_bar ? 1 : 0)) - hlf_barwdth,
			    PX(i +(do_bar ? 1 : 0)) + hlf_barwdth,
			    PY(barset[j].y1),
                            PY(barset[j].y2),
			    xdepth_3D,
                            ydepth_3D,
                            barset[j].clr,
			    barset[j].shclr);
	      }
	    }
            }
            break;

	case GDC_STACK_BESIDE: {
	    int new_barwdth = hlf_barwdth / (num_sets / 2.0);
	    for(i = 0; i < GDC->num_points; ++i)
	      for(j = 0; j < num_sets; ++j)
		if(uvals[j][i] != GDC_NOVALUE)
		  draw_3d_bar(GDC->image,
                              PX(i +(do_bar ? 1 : 0)) - hlf_barwdth + new_barwdth * j + 1,
		      	      PX(i +(do_bar ? 1 : 0)) - hlf_barwdth + new_barwdth *(j + 1),
			      PY(0),
                              PY(uvals[j][i]),
                              xdepth_3D,
			      ydepth_3D,
                              ExtColor[j][i],
			      threeD ? ExtColorShd[j][i] : ExtColor[j][i]);
            }
            break;
        }
	break;

    case GDC_3DFLOATINGBAR:
    case GDC_FLOATINGBAR:
	switch(GDC->stack_type) {
        case GDC_STACK_SUM:
        case GDC_STACK_LAYER:
            break;
	case GDC_STACK_DEPTH:
            for(i = 0; i < GDC->num_points; ++i)
      	      if(uvals[0][i] != GDC_NOVALUE &&
                 uvals[1][i] != GDC_NOVALUE &&
                 uvals[1][i] > uvals[0][i])
	        draw_3d_bar(GDC->image,
                            PX(i +(do_bar ? 1 : 0)) - hlf_barwdth,
	      	            PX(i +(do_bar ? 1 : 0)) + hlf_barwdth,
			    PY(uvals[0][i]),
			    PY(uvals[1][i]),
                            xdepth_3D,
			    ydepth_3D,
                            ExtColor[setno][i],
			    threeD ? ExtColorShd[setno][i] : ExtColor[setno][i]);
	    setno = 0;
	    break;

	case GDC_STACK_BESIDE: {
	    int new_barwdth = hlf_barwdth / (num_sets / 2.0);
	    for(i = 0; i < GDC->num_points; ++i)
       	      if(uvals[0][i] != GDC_NOVALUE &&
                 uvals[1][i] != GDC_NOVALUE &&
                 uvals[1][i] > uvals[0][i])
	        draw_3d_bar(GDC->image,
                            PX(i +(do_bar ? 1 : 0)) - hlf_barwdth + new_barwdth * 0 + 1,
	      	            PX(i +(do_bar ? 1 : 0)) - hlf_barwdth + new_barwdth * (0 + 1),
			    PY(uvals[0][i]),
			    PY(uvals[1][i]),
                            xdepth_3D,
			    ydepth_3D,
                            ExtColor[j][i],
			    threeD ? ExtColorShd[j][i] : ExtColor[j][i]);
	    }
	    break;
	}
	break;

    case GDC_AREA:
    case GDC_3DAREA:
	switch(GDC->stack_type) {
	case GDC_STACK_SUM: {
	    float lasty[GDC->num_points];
	    int j = 0;
	    for(i = 1; i < GDC->num_points; ++i)
	      if(uvals[j][i] != GDC_NOVALUE) {
	        lasty[i] = uvals[j][i];
		if(uvals[j][i - 1] != GDC_NOVALUE)
		  draw_3d_area(GDC->image,
                               PX(i - 1),
                               PX(i),
                               PY(0),
                               PY(uvals[j][i - 1]),
                               PY(uvals[j][i]),
                               xdepth_3D,
                               ydepth_3D,
                               ExtColor[j][i],
                               threeD ? ExtColorShd[j][i] : ExtColor[j][i]);
	      }
	      for(j = 1; j < num_sets; ++j)
		for(i = 1; i < GDC->num_points; ++i)
		  if(uvals[j][i] != GDC_NOVALUE && uvals[j][i - 1] != GDC_NOVALUE) {
		    draw_3d_area(GDC->image,
                                 PX(i - 1),
                                 PX(i),
                                 PY(lasty[i]),
                                 PY(lasty[i - 1] + uvals[j][i - 1]),
				 PY(lasty[i] + uvals[j][i]),
				 xdepth_3D,
                                 ydepth_3D,
				 ExtColor[j][i],
				 threeD ? ExtColorShd[j][i] : ExtColor[j][i]);
		    lasty[i] += uvals[j][i];
		  }
	    }
	    break;

	case GDC_STACK_BESIDE:	/* behind w/o depth */
	    for(j = num_sets - 1; j >= 0; --j)	/* back sets 1st (setno = 0) */
	      for(i = 1; i < GDC->num_points; ++i)
		if(uvals[j][i - 1] != GDC_NOVALUE && uvals[j][i] != GDC_NOVALUE)
		  draw_3d_area(GDC->image,
                               PX(i - 1),
                               PX(i),
                               PY(0),
                               PY(uvals[j][i - 1]),
                               PY(uvals[j][i]),
			       xdepth_3D,
                               ydepth_3D,
                               ExtColor[j][i],
			       threeD ? ExtColorShd[j][i] : ExtColor[j][i]);
	    break;

	case GDC_STACK_DEPTH:
	default:
	    for(setno = num_sets - 1; setno >= 0; --setno)	/* back sets first   PX,PY depth */
	      for(i = 1; i < GDC->num_points; ++i)
		if(uvals[setno][i - 1] != GDC_NOVALUE && uvals[setno][i] != GDC_NOVALUE)
		  draw_3d_area(GDC->image,
                               PX(i - 1),
                               PX(i),
                               PY(0),
			       PY(uvals[setno][i - 1]),
			       PY(uvals[setno][i]),
                               xdepth_3D,
			       ydepth_3D,
                               ExtColor[setno][i],
			       threeD ? ExtColorShd[setno][i] : ExtColor[setno][i]);
	    setno = 0;
	}

	break;

    case GDC_LINEAREA:
        // First data set will be the area
        for(i = 1; i < GDC->num_points; ++i)
          if(uvals[0][i-1] != GDC_NOVALUE && uvals[0][i] != GDC_NOVALUE)
            draw_3d_area(GDC->image,
                         PX(i-1),
                         PX(i),
                         PY(0),
                         PY(uvals[0][i-1]),
                         PY(uvals[0][i]),
                         0,
                         0,
                         ExtColor[0][i],
                         ExtColor[0][i]);
        // Draw canvas around the area
        for(i = 1; i < GDC->num_points; ++i)
	    if(uvals[0][i-1] != GDC_NOVALUE && uvals[0][i] != GDC_NOVALUE) {
	      gdImageLine(GDC->image,
                          PX(i-1),
                          PY(uvals[0][i-1]),
                          PX(i),
                          PY(uvals[0][i]),
                          ExtColor[0][i]);
	      gdImageLine(GDC->image,
                          PX(i-1),
                          PY(uvals[0][i-1])+1,
                          PX(i),
                          PY(uvals[0][i])+1,
                          GridColor);
	    } else {
	      if(uvals[0][i-1] != GDC_NOVALUE) gdImageSetPixel(GDC->image,PX(i-1),PY(uvals[0][i-1]),GridColor);
	      if(uvals[0][i] != GDC_NOVALUE) gdImageSetPixel(GDC->image,PX(i),PY(uvals[0][i]),GridColor);
	    }
        // All other data sets just line graphs
        for(j = num_sets - 1; j > 0; --j)
	  for(i = 1; i < GDC->num_points; ++i)
	    if(uvals[j][i-1] != GDC_NOVALUE && uvals[j][i] != GDC_NOVALUE) {
	      gdImageLine(GDC->image,
                          PX(i-1),
                          PY(uvals[j][i-1]),
                          PX(i),
                          PY(uvals[j][i]),
                          ExtColor[j][i]);
	      gdImageLine(GDC->image,
                          PX(i-1),
                          PY(uvals[j][i-1])+1,
                          PX(i),
                          PY(uvals[j][i])+1,
                          ExtColor[j][i]);
	    } else {
	      if(uvals[j][i-1] != GDC_NOVALUE) gdImageSetPixel(GDC->image,PX(i-1),PY(uvals[j][i-1]),ExtColor[j][i]);
	      if(uvals[j][i] != GDC_NOVALUE) gdImageSetPixel(GDC->image,PX(i),PY(uvals[j][i]),ExtColor[j][i]);
	    }
        break;

    case GDC_LINE:
    case GDC_COMBO_LINE_BAR:
    case GDC_COMBO_LINE_AREA:
    case GDC_COMBO_LINE_LINE:
	for(j = num_sets - 1; j >= 0; --j)
	  for(i = 1; i < GDC->num_points; ++i)
	    if(uvals[j][i - 1] != GDC_NOVALUE && uvals[j][i] != GDC_NOVALUE) {
	      gdImageLine(GDC->image,
                          PX(i - 1),
                          PY(uvals[j][i - 1]),
                          PX(i),
                          PY(uvals[j][i]),
                          ExtColor[j][i]);
	      gdImageLine(GDC->image,
                          PX(i - 1),
                          PY(uvals[j][i - 1]) + 1,
                          PX(i),
                          PY(uvals[j][i]) + 1,
                          ExtColor[j][i]);
	    } else {
	      if(uvals[j][i - 1] != GDC_NOVALUE)
		gdImageSetPixel(GDC->image,PX(i - 1),PY(uvals[j][i - 1]),ExtColor[j][i]);
	      if(uvals[j][i] != GDC_NOVALUE)
		gdImageSetPixel(GDC->image,PX(i),PY(uvals[j][i]),ExtColor[j][i]);
	    }
	break;

    case GDC_3DLINE:
    case GDC_3DCOMBO_LINE_BAR:
    case GDC_3DCOMBO_LINE_AREA:
    case GDC_3DCOMBO_LINE_LINE:	{
	int y1[num_sets];
	int y2[num_sets];
	unsigned long clr[num_sets];
	unsigned long clrshd[num_sets];

	for(i = 1; i < GDC->num_points; ++i) {
	  if(GDC->stack_type == GDC_STACK_DEPTH) {
	    for(j = num_sets - 1; j >= 0; --j)
	      if(uvals[j][i - 1] != GDC_NOVALUE && uvals[j][i] != GDC_NOVALUE) {
		setno = j;
		y1[j] = PY(uvals[j][i - 1]);
		y2[j] = PY(uvals[j][i]);
                draw_3d_line(GDC->image,
                             PY(0),
                             PX(i - 1),
                             PX(i),
                             &(y1[j]),
                             &(y2[j]),
                             xdepth_3D,
                             ydepth_3D,
                             1,
                             &(ExtColor[j][i]),
                             &(ExtColorShd[j][i]));
		setno = 0;
	      }
	  } else
          if(GDC->stack_type == GDC_STACK_BESIDE ||
             GDC->stack_type == GDC_STACK_SUM) {
	    int set;
	    float usey1 = 0.0,usey2 = 0.0;
	    for(j = 0,set = 0; j < num_sets; ++j)
	      if(uvals[j][i - 1] != GDC_NOVALUE && uvals[j][i] != GDC_NOVALUE) {
		if(GDC->stack_type == GDC_STACK_SUM) {
		  usey1 += uvals[j][i - 1];
		  usey2 += uvals[j][i];
		} else {
		  usey1 = uvals[j][i - 1];
		  usey2 = uvals[j][i];
		}
		y1[set] = PY(usey1);
		y2[set] = PY(usey2);
		clr[set] = ExtColor[j][i];
		clrshd[set] = ExtColorShd[j][i];
		++set;
	      }
	      draw_3d_line(GDC->image,
                           PY(0),
                           PX(i - 1),
                           PX(i),
                           y1,
                           y2,
                           xdepth_3D,
                           ydepth_3D,
                           set,
                           clr,
                           clrshd);
	  }
	}
	}
	break;

    case GDC_3DHILOCLOSE:
    case GDC_3DCOMBO_HLC_BAR:
    case GDC_3DCOMBO_HLC_AREA: {
	gdPoint poly[4];
        for(i = 1; i < GDC->num_points + 1; ++i)
	  if(uvals[CLOSESET][i - 1] != GDC_NOVALUE) {
	    if((GDC->HLC_style & GDC_HLC_I_CAP) && uvals[LOWSET][i - 1] != GDC_NOVALUE) {
	      SET_3D_POLY(poly,
                          PX(i - 1) - hlf_hlccapwdth,
			  PX(i - 1) + hlf_hlccapwdth,
			  PY(uvals[LOWSET][i - 1]),
			  PY(uvals[LOWSET][i - 1]),
			  xdepth_3D,
                          ydepth_3D);
	      gdImageFilledPolygon(GDC->image,poly,4,ExtColor[LOWSET][i - 1]);
	      gdImagePolygon(GDC->image,poly,4,ExtColorShd[LOWSET][i - 1]);
	  }
	  /* all HLC have vert line */
	  if(uvals[LOWSET][i - 1] != GDC_NOVALUE) {	/* bottom 'half' */
	    SET_3D_POLY(poly,
                        PX(i - 1),
                        PX(i - 1),
	      	        PY(uvals[LOWSET][i - 1]),
			PY(uvals[CLOSESET][i - 1]),
			xdepth_3D,
                        ydepth_3D);
	    gdImageFilledPolygon(GDC->image,poly,4,ExtColor[LOWSET][i - 1]);
	    gdImagePolygon(GDC->image,poly,4,ExtColorShd[LOWSET][i - 1]);
	  }
	  if(uvals[HIGHSET][i - 1] != GDC_NOVALUE) {	/* top 'half' */
	    SET_3D_POLY(poly,
                        PX(i - 1),
                        PX(i - 1),
	      	        PY(uvals[CLOSESET][i - 1]),
			PY(uvals[HIGHSET][i - 1]),
			xdepth_3D,
                        ydepth_3D);
	    gdImageFilledPolygon(GDC->image,poly,4,ExtColor[HIGHSET][i - 1]);
	    gdImagePolygon(GDC->image,poly,4,ExtColorShd[HIGHSET][i -	1]);
	  }
	  /* line at close */
	  gdImageLine(GDC->image,
                      PX(i - 1),
                      PY(uvals[CLOSESET][i - 1]),
	       	      PX(i - 1) + xdepth_3D,
		      PY(uvals[CLOSESET][i - 1]) -
		      ydepth_3D,
		      ExtColorShd[CLOSESET][i - 1]);
	  /* top half 'I' */
	  if(!((GDC->HLC_style & GDC_HLC_DIAMOND) &&
             (PY(uvals[HIGHSET][i - 1]) > PY(uvals[CLOSESET][i - 1]) - hlf_hlccapwdth)) &&
             uvals[HIGHSET][i - 1] != GDC_NOVALUE)
            if(GDC->HLC_style & GDC_HLC_I_CAP) {
      	      SET_3D_POLY(poly,
                          PX(i - 1) - hlf_hlccapwdth,
	       	          PX(i - 1) + hlf_hlccapwdth,
			  PY(uvals[HIGHSET][i - 1]),
			  PY(uvals[HIGHSET][i - 1]),
                          xdepth_3D,
			  ydepth_3D);
	      gdImageFilledPolygon(GDC->image,poly,4,ExtColor[HIGHSET][i - 1]);
	      gdImagePolygon(GDC->image,poly,4,ExtColorShd[HIGHSET][i - 1]);
	    }
            if(i < GDC->num_points && uvals[CLOSESET][i] != GDC_NOVALUE) {
	      if(GDC->HLC_style & GDC_HLC_CLOSE_CONNECTED) {	/* line from prev close */
	        SET_3D_POLY(poly,
                            PX(i - 1),
                            PX(i),
			    PY(uvals[CLOSESET][i - 1]),
			    PY(uvals[CLOSESET][i - 1]),
                            xdepth_3D,
			    ydepth_3D);
		gdImageFilledPolygon(GDC->image,poly,4,ExtColor[CLOSESET][i]);
		gdImagePolygon(GDC->image,poly,4,ExtColorShd[CLOSESET][i]);
	      } else
              if(GDC->HLC_style & GDC_HLC_CONNECTING) {	/* thin connecting line */
	        int y1 = PY(uvals[CLOSESET][i - 1]),
	            y2 = PY(uvals[CLOSESET][i]);
	        draw_3d_line(GDC->image,
                             PY(0),
                             PX(i - 1),
                             PX(i),
                             &y1,
                             &y2,	/* rem only 1 set */
	       	             xdepth_3D,
                             ydepth_3D,
			     1,
			     &(ExtColor[CLOSESET][i]),
			     &(ExtColorShd[CLOSESET][i]));
		/* edge font of it */
		gdImageLine(GDC->image,
                            PX(i - 1),
                            PY(uvals[CLOSESET][i - 1]),
                            PX(i),
		            PY(uvals[CLOSESET][i]),
		            ExtColorShd[CLOSESET][i]);
	      }
	      /* top half 'I' again */
	      if(PY(uvals[CLOSESET][i - 1]) <= PY(uvals[CLOSESET][i]) &&
	         uvals[HIGHSET][i - 1] != GDC_NOVALUE)
	        if(GDC->HLC_style & GDC_HLC_I_CAP) {
	          SET_3D_POLY(poly,
                              PX(i - 1) - hlf_hlccapwdth,
	      		      PX(i - 1) + hlf_hlccapwdth,
			      PY(uvals[HIGHSET][i - 1]),
			      PY(uvals[HIGHSET][i - 1]),
                              xdepth_3D,
			      ydepth_3D);
		  gdImageFilledPolygon(GDC->image,poly,4,ExtColor[HIGHSET][i - 1]);
		  gdImagePolygon(GDC->image,poly,4,ExtColorShd[HIGHSET][i - 1]);
		}
	      }
	      if(GDC->HLC_style & GDC_HLC_DIAMOND) {	/* front */
		poly[0].x = PX(i - 1) - hlf_hlccapwdth;
		poly[0].y = PY(uvals[CLOSESET][i - 1]);
		poly[1].x = PX(i - 1);
		poly[1].y = PY(uvals[CLOSESET][i - 1]) + hlf_hlccapwdth;
		poly[2].x = PX(i - 1) + hlf_hlccapwdth;
		poly[2].y = PY(uvals[CLOSESET][i - 1]);
		poly[3].x = PX(i - 1);
		poly[3].y = PY(uvals[CLOSESET][i - 1]) - hlf_hlccapwdth;
		gdImageFilledPolygon(GDC->image,poly,4,ExtColor[CLOSESET][i - 1]);
		gdImagePolygon(GDC->image,poly,4,ExtColorShd[CLOSESET][i - 1]);
		/* bottom side */
		SET_3D_POLY(poly,
                            PX(i - 1),
                            PX(i - 1) + hlf_hlccapwdth,
			    PY(uvals[CLOSESET][i - 1]) + hlf_hlccapwdth,
			    PY(uvals[CLOSESET][i - 1]),
			    xdepth_3D,ydepth_3D);
		gdImageFilledPolygon(GDC->image,poly,4,ExtColorShd[CLOSESET][i - 1]);
      		/* top side */
		SET_3D_POLY(poly,
                            PX(i - 1),
                            PX(i - 1) + hlf_hlccapwdth,
			    PY(uvals[CLOSESET][i - 1]) - hlf_hlccapwdth,
			    PY(uvals[CLOSESET][i - 1]),
                            xdepth_3D,
                            ydepth_3D);
		gdImageFilledPolygon(GDC->image,poly,4,ExtColor[CLOSESET][i - 1]);
		gdImagePolygon(GDC->image,poly,4,ExtColorShd[CLOSESET][i - 1]);
	      }
	    }
	}
	break;

    case GDC_HILOCLOSE:
    case GDC_COMBO_HLC_BAR:
    case GDC_COMBO_HLC_AREA:
        for(i = 0; i < GDC->num_points; ++i)
          if(uvals[CLOSESET][i] != GDC_NOVALUE) {	/* all HLC have vert line */
            if(uvals[LOWSET][i] != GDC_NOVALUE)
      	      gdImageLine(GDC->image,
                          PX(i),
                          PY(uvals[CLOSESET][i]),
                          PX(i),
                          PY(uvals[LOWSET][i]),
                          ExtColor[LOWSET][i]);
	      if(uvals[HIGHSET][i] != GDC_NOVALUE)
		gdImageLine(GDC->image,
                            PX(i),
                            PY(uvals[HIGHSET][i]),
                            PX(i),
                            PY(uvals[CLOSESET][i]),
                            ExtColor[HIGHSET][i]);
	      if(GDC->HLC_style & GDC_HLC_I_CAP) {
		if(uvals[LOWSET][i] != GDC_NOVALUE)
		  gdImageLine(GDC->image,
                              PX(i) - hlf_hlccapwdth,
                              PY(uvals[LOWSET][i]),
			      PX(i) + hlf_hlccapwdth,
			      PY(uvals[LOWSET][i]),
			      ExtColor[LOWSET][i]);
		if(uvals[HIGHSET][i] != GDC_NOVALUE)
		  gdImageLine(GDC->image,
                              PX(i) - hlf_hlccapwdth,
                              PY(uvals[HIGHSET][i]),
			      PX(i) + hlf_hlccapwdth,
			      PY(uvals[HIGHSET][i]),
			      ExtColor[HIGHSET][i]);
	      }
	      if(GDC->HLC_style & GDC_HLC_DIAMOND) {
		gdPoint cd[4];
		cd[0].x = PX(i) - hlf_hlccapwdth;
		cd[0].y = PY(uvals[CLOSESET][i]);
		cd[1].x = PX(i);
		cd[1].y = PY(uvals[CLOSESET][i]) + hlf_hlccapwdth;
		cd[2].x = PX(i) + hlf_hlccapwdth;
		cd[2].y = PY(uvals[CLOSESET][i]);
		cd[3].x = PX(i);
		cd[3].y = PY(uvals[CLOSESET][i]) - hlf_hlccapwdth;
		gdImageFilledPolygon(GDC->image,cd,4,ExtColor[CLOSESET][i]);
	      }
	    }
	    for(i = 1; i < GDC->num_points; ++i)
	      if(uvals[CLOSESET][i - 1] != GDC_NOVALUE && uvals[CLOSESET][i] != GDC_NOVALUE) {
	       if(GDC->HLC_style & GDC_HLC_CLOSE_CONNECTED)	/* line from prev close */
		 gdImageLine(GDC->image,
                             PX(i - 1),
                             PY(uvals[CLOSESET][i - 1]),
			     PX(i),
			     PY(uvals[CLOSESET][i - 1]),
			     ExtColor[CLOSESET][i]);
	       else
               if(GDC->HLC_style & GDC_HLC_CONNECTING)	/* thin connecting line */
		 gdImageLine(GDC->image,
                             PX(i - 1),
                             PY(uvals[CLOSESET][i - 1]),
			     PX(i),
                             PY(uvals[CLOSESET][i]),
			     ExtColor[CLOSESET][i]);
	      }
	break;
    }
    if(GDC->grid_on_top) goto gdcGrid;

gdcFinish:
    setno = 0;
    /* ---------- scatter points  over all other plots ---------- */
    /* scatters,by their very nature,don't lend themselves to standard array of points */
    /* also,this affords the opportunity to include scatter points onto any GDC->type of chart */
    /* drawing of the scatter point should be an exposed function,so the user can */
    /*  use it to draw a legend,and/or add their own */
    if(GDC->scatter) {
      int scatter_clr[GDC->num_scatter_pts];
      gdPoint ct[3];

      for(i = 0; i < GDC->num_scatter_pts; ++i) {
	int hlf_scatterwdth =(int)((float)(PX(2) - PX(1)) * (((float)((GDC->scatter + i)->width) / 100.0) / 2.0));
        int scat_x = PX((GDC->scatter + i)->point +(do_bar ? 1 : 0)),
            scat_y = PY((GDC->scatter + i)->val);

	if((GDC->scatter + i)->point >= GDC->num_points || (GDC->scatter + i)->point < 0) continue;
	scatter_clr[i] = clrallocate(GDC->image,(GDC->scatter + i)->color);

	switch((GDC->scatter + i)->ind) {
	 case GDC_SCATTER_CIRCLE: {
	     int uc,s = 0,e = 360,fo = 0;
	     if(!do_bar) {
	       if((GDC->scatter + i)->point == 0) {
		 s = 270;
		 e = 270 + 180;
		 fo = 1;
	       } else
	       if((GDC->scatter + i)->point == GDC->num_points - 1) {
		 s = 90;
		 e = 90 + 180;
		 fo = -1;
	       }
             }
             uc = gdImageColorAllocate(GDC->image,l2gdcal(GDC->ScatterColor));
             gdImageArc(GDC->image,scat_x,scat_y,hlf_scatterwdth * 2,hlf_scatterwdth * 2,s,e,uc);
             if(fo) gdImageLine(GDC->image,scat_x,scat_y + hlf_scatterwdth,scat_x,scat_y - hlf_scatterwdth,uc);
             gdImageFillToBorder(GDC->image,scat_x + fo,scat_y,uc,scatter_clr[i]);
             gdImageArc(GDC->image,scat_x,scat_y,hlf_scatterwdth * 2,hlf_scatterwdth * 2,s,e,scatter_clr[i]);
             if(fo) gdImageLine(GDC->image,scat_x,scat_y + hlf_scatterwdth,scat_x,scat_y - hlf_scatterwdth,scatter_clr[i]);
             gdImageColorDeallocate(GDC->image,uc);
	     }
	     break;
	 case GDC_SCATTER_TRIANGLE_UP:
	     ct[0].x = scat_x;
	     ct[0].y = scat_y;
	     ct[1].x = scat_x - hlf_scatterwdth;
	     ct[1].y = scat_y + hlf_scatterwdth;;
	     ct[2].x = scat_x + hlf_scatterwdth;
	     ct[2].y = scat_y + hlf_scatterwdth;
	     if(!do_bar) {
	       if((GDC->scatter + i)->point == 0) ct[1].x = scat_x; else
               if((GDC->scatter + i)->point == GDC->num_points - 1) ct[2].x = scat_x;
             }
             gdImageFilledPolygon(GDC->image,ct,3,scatter_clr[i]);
	     break;
	 case GDC_SCATTER_TRIANGLE_DOWN:
	     ct[0].x = scat_x;
	     ct[0].y = scat_y;
	     ct[1].x = scat_x - hlf_scatterwdth;
	     ct[1].y = scat_y - hlf_scatterwdth;;
	     ct[2].x = scat_x + hlf_scatterwdth;
	     ct[2].y = scat_y - hlf_scatterwdth;
	     if(!do_bar) {
	       if((GDC->scatter + i)->point == 0) ct[1].x = scat_x; else
               if((GDC->scatter + i)->point == GDC->num_points - 1) ct[2].x = scat_x;
             }
             gdImageFilledPolygon(GDC->image,ct,3,scatter_clr[i]);
	     break;
	 }
	}
    }

    /* box it off after plotting so the outline covers any plot lines */
    {
      if(GDC->border == GDC_BORDER_ALL || (GDC->border & GDC_BORDER_X))
	gdImageLine(GDC->image,PX(0),PY(lowest),PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),PY(lowest),LineColor);

      if(GDC->border == GDC_BORDER_ALL ||(GDC->border & GDC_BORDER_TOP)) {
	setno = set_depth;
	gdImageLine(GDC->image,PX(0),PY(highest),PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),PY(highest),LineColor);
	setno = 0;
      }
    }
    if(GDC->border) {
      int x1 = PX(0);
      int y1 = PY(highest);
      int x2 = PX(GDC->num_points - 1 + (do_bar ? 2 : 0));
      int y2 = PY(lowest);

      if(GDC->border == GDC_BORDER_ALL || (GDC->border & GDC_BORDER_Y))
	gdImageLine(GDC->image,x1,PY(lowest),x1,y1,LineColor);
      setno = set_depth;
      if(GDC->border == GDC_BORDER_ALL || (GDC->border & GDC_BORDER_Y) || (GDC->border & GDC_BORDER_TOP))
	gdImageLine(GDC->image,x1,y1,PX(0),PY(highest),LineColor);
      if(GDC->border == GDC_BORDER_ALL || (GDC->border & GDC_BORDER_X) || (GDC->border & GDC_BORDER_Y2))
        gdImageLine(GDC->image,x2,y2,PX(GDC->num_points - 1 + (do_bar ? 2 : 0)),PY(lowest),LineColor);
      if(GDC->border == GDC_BORDER_ALL || (GDC->border & GDC_BORDER_Y2))
        gdImageLine(GDC->image,PX(GDC->num_points - 1 + (do_bar ? 2 : 0)),PY(lowest),PX(GDC->num_points - 1 +(do_bar ? 2 : 0)),PY(highest),LineColor);
      setno = 0;
    }

    if(GDC->Shelf && threeD && ((lowest < 0.0 && highest > 0.0) || ((lowest == 0.0 || highest == 0.0) && !(GDC->border & GDC_BORDER_X)))) {
      int x2 = PX(GDC->num_points - 1 +(do_bar ? 2 : 0));
      int y2 = PY(0);

      gdImageLine(GDC->image,PX(0),PY(0),x2,y2,LineColor);	/* front line */
      setno = set_depth;
      /* depth for 3Ds */
      gdImageLine(GDC->image,x2,y2,PX(GDC->num_points - 1 + (do_bar ? 2 : 0)),PY(0),LineColor);
      setno = 0;		/* set back to foremost */
    }

    if(GDC->annotation.note[0]) {	/* front half of annotation line */
      int x1 = PX(GDC->annotation.point +(do_bar ? 1 : 0)),
          y1 = PY(highest);
      int x2;
      /* front line */
      gdImageLine(GDC->image,x1,PY(lowest) + 1,x1,y1,AnnoteColor);
      if(threeD) {		/* on back plane */
	setno = set_depth;
	x2 = PX(GDC->annotation.point +(do_bar ? 1 : 0));
	/* prspective line */
	gdImageLine(GDC->image,x1,y1,x2,PY(highest),AnnoteColor);
      } else {		/* for 3D done with back line */
	x2 = PX(GDC->annotation.point +(do_bar ? 1 : 0));
	gdImageLine(GDC->image,x1,y1,x1,y1 - 2,AnnoteColor);
      }
      /* line-to and note */
      if(*(GDC->annotation.note)) {	/* any note? */
	if(GDC->annotation.point >=(GDC->num_points / 2)) {	/* note to the left */
	  gdImageLine(GDC->image,x2,PY(highest) - 2,x2 - annote_hgt / 2,PY(highest) - 2 - annote_hgt / 2,AnnoteColor);
	  GDCImageStringNL(GDC->image,&GDC_fontc[GDC->annotation_font_size],
			   gdc_annotation_font,
			   gdc_annotation_ptsize,0.0,
			   x2 - annote_hgt / 2 - 1 - annote_len - 1,
			   PY(highest) - annote_hgt + 1,
			   GDC->annotation.note,AnnoteColor,
			   GDC_JUSTIFY_RIGHT,NULL);
        } else {		/* note to right */
	  gdImageLine(GDC->image,x2,PY(highest) - 2,x2 + annote_hgt / 2,PY(highest) - 2 - annote_hgt / 2,AnnoteColor);
	  GDCImageStringNL(GDC->image,&GDC_fontc[GDC->annotation_font_size],
			   gdc_annotation_font,
			   gdc_annotation_ptsize,0.0,
			   x2 + annote_hgt / 2 + 1 + 1,
			   PY(highest) - annote_hgt + 1,
			   GDC->annotation.note,AnnoteColor,
			   GDC_JUSTIFY_LEFT,NULL);
	}
      }
      setno = 0;
    }
    /* Draw legend */
    if(GDC->legend >= 0) {
      int i,x,y,maxlx = 0,lx,maxly = 0,ly;
      int LegendColor = gdImageColorAllocate(GDC->image,l2gdcal(GDC->LegendColor));

      /* Determine max label sizes */
      for(i = 0; i < GDC->num_sets; i++) {
        if(!GDC->dlabels[i]) continue;
        lx = (strlen(GDC->dlabels[i]) + 1) * GDC_fontc[GDC_SMALL].w;
        ly = GDC_fontc[GDC_SMALL].h;
        lx += SQUARE_SIZE;
        if(ly < SQUARE_SIZE) ly = SQUARE_SIZE;
        maxlx = MAX(maxlx,lx);
        maxly = MAX(maxly,ly);
      }
      /* Actually draw labels */
      for(i = 0; i < GDC->num_sets; i++) {
        if(!GDC->dlabels[i]) continue;
        if(!GDC->legend) {
          if(GDC->legend_x || GDC->legend_y) {
            x = GDC->legend_x;
            y = GDC->legend_y+maxly*i+maxly/2;
          } else {
            x = GDC->width/2-maxlx/2;
            y = GDC->height/2+maxly*i-maxly*(GDC->num_sets-2)/2;
          }
	} else {
          if(GDC->legend_x || GDC->legend_y) {
            x = GDC->legend_x+(maxlx+LEGEND_HORIZ_SPACING)*i;
            y = GDC->legend_y;
          } else {
  	    x = GDC->width/2+((maxlx+LEGEND_HORIZ_SPACING)*(2*i-GDC->num_sets)/2)-maxlx/2;
  	    y = GDC->height/2;
          }
         }
         gdImageFilledRectangle(GDC->image,x,y-1,x+SQUARE_SIZE+1,y+1,ExtColor[i][0]);
         gdImageString(GDC->image,
                       GDC_fontc[GDC_SMALL].f,
	               x+SQUARE_SIZE+GDC_fontc[GDC_SMALL].w,
	               y-GDC_fontc[GDC_SMALL].h/2+1,
	               GDC->dlabels[i],
                       LegendColor);
      }
    }
    if(GDC->image_file) {
      FILE *fp = fopen(GDC->image_file,"wb");
      if(fp) {
        switch(GDC->image_type) {
         case GDC_JPEG:
#ifdef HAVE_JPEG
            gdImageJpeg(GDC->image,fp,GDC->jpeg_quality);
#endif
            break;
         case GDC_WBMP:
            gdImageWBMP(GDC->image,PlotColor,fp);
            break;
         case GDC_PNG:
         default:
            gdImagePng(GDC->image,fp);
        }
        fclose(fp);
      } else
        Ns_Log(Error,"GDC: %s: %s",GDC->image_file,strerror(errno));
    }
    if(bg_img) gdImageDestroy(bg_img);

    if(!(GDC->hold & GDC_EXPOSE_IMAGE)) {
      gdImageDestroy(GDC->image);
      GDC->image = 0;
    }
    return 0;
}

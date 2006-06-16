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
 * nsgdchart -- CHART generation module using gdcharts
 *              from brv@fred.net http://www.fred.net/brv/chart/
 *
 *     Vlad Seryakov vlad@crystalballinc.com
 */

#include "nsgdchart.h"

static int gdcID = 0;
static int gdcDebug = 0;
static GDC_T *gdcList = 0;
static Ns_Mutex gdcMutex;

static int GDCCmd(ClientData arg,Tcl_Interp * interp,int objc,Tcl_Obj * CONST objv[]);
static int GDCInterpInit(Tcl_Interp * interp,void *context);
NS_EXPORT int Ns_ModuleVersion = 1;

NS_EXPORT int Ns_ModuleInit(char *server,char *module)
{
    char *path;

    path = Ns_ConfigGetPath(server,module,NULL);
    Ns_ConfigGetInt(path,"debug",&gdcDebug);
    Ns_TclRegisterTrace(server, GDCInterpInit, 0, NS_TCL_TRACE_CREATE);
    return NS_OK;
}

static int GDCInterpInit(Tcl_Interp * interp,void *context)
{
    Tcl_CreateObjCommand(interp,"ns_gdchart",GDCCmd,NULL,NULL);
    return NS_OK;
}

static int
GDCCmd(ClientData arg,Tcl_Interp * interp,int objc,Tcl_Obj * CONST objv[])
{
    int i,j,k,argc,opt,cmd;
    Tcl_Obj **argv;
    char *val;
    GDC_T *gdc = 0;

    enum cmds {
	cmdCreate,cmdSet,cmdSetData,cmdSetLabels,cmdSetColors,cmdSkipLabels,
        cmdSave,cmdDestroy,cmdImage,cmdReturn
    };
    static const char *Cmds[] = {
	"create","set","setdata","setlabels","setcolors","skiplabels",
        "save","destroy","image","return",
	0
    };

    static const char *Types[] = {
	"line","area","bar","floatingbar","hiloclose","combolinebar","combohlcbar",
        "combolinearea","combolineline","combohlcarea","3dhiloclose",
        "3dcombolinebar","3dcombolinearea","3dcombolineline","3dcombohlcbar",
        "3dcombohlcarea","3dbar","3dfloatingbar","3darea","3dline","linearea",
	0
    };

    enum opts {
        optType,optBgColor,optPlotColor,optTitle,optTitleFont,optTitlePtSize,
        optXTitle,optXTitleFont,optXTitlePtSize,optYTitle,optYTitleFont,optYTitlePtSize,
        optXAxisAngle,optXAxisFont,optXAxisPtSize,optBarWidth,optImageType,
        optBgImage,optYTitle2,optYTitleSize,optVolColor,opt3dDepth,optGrid,optTitleColor,
        optXTitleColor,optYTitleColor,optYTitle2Color,optXLabelColor,optYLabelColor,
        optYLabel2Color,optBorder,optShelf, optStackType,optYLabelFmt,optYLabel2Fmt,optGridColor,
        optTicks,optReqYMin,optReqYMax,optReqYInterval,optAnnoText,optAnnoFont,
        optAnnoFontSize,optAnnoPoint,optAnnoColor,optAnnoPtSize,optXLabelSpacing,
        optYlabelDensity,optInterpolations,optXAxis,optYAxis,optYAxis2,optYValStyle,
        opt3dAngle,optThumbnail,optRandomSetColors,optRandomExtColors,optScatterColor,
        optHoldImage,optHLCStyle,optHLCCapWidth,optWidth,optHeight,optLineColor,
        optLegend,optLegendX,optLegendY,optLegendColor,optHardWidth,optHardHeight,
        optHardXOrig,optHardYOrig,optGridOnTop
    };
    static const char *Opts[] = {
	"type","bgcolor","plotcolor","title","titlefont","titleptsize",
        "xtitle","xtitlefont","xtitleptsize","ytitle","ytitlefont","ytitleptsize",
        "xaxisangle","xaxisfont","xaxisptsize","barwidth","imagetype",
        "bgimage","ytitle2","ytitlesize","volcolor","3ddepth","grid","titlecolor",
        "xtitlecolor","ytitlecolor","ytitle2color","xlabelcolor","ylabelcolor",
        "ylabel2color","border","shelf", "stacktype","ylabelfmt","ylabel2fmt","gridcolor",
        "ticks","reqymin","reqymax","reqyinterval","annotext","annofont",
        "annofontsize","annopoint","annocolor","annoptsize","xlabelspacing","ylabeldensity",
        "interpolations","xaxis","yaxis","yaxis2","yvalstyle","3dangle","thumbnail",
        "randomsetcolors","randomextcolors","scattercolor","holdimage","hlcstyle",
        "hlccapwidth","width","height","linecolor",
        "legend","legendx","legendy","legendcolor","hardwidth","hardheight",
        "hardxorig","hardyorig","gridontop",
	0
    };

    if(objc < 2) {
      Tcl_WrongNumArgs(interp,1,objv,"command ...");
      return TCL_ERROR;
    }
    if(Tcl_GetIndexFromObj(interp,objv[1],Cmds,"command",TCL_EXACT,(int *) &cmd) != TCL_OK)
      return TCL_ERROR;

    if(cmd > cmdCreate) {
      if(objc < 3) {
        Tcl_WrongNumArgs(interp,2,objv,"#chart ...");
        return TCL_ERROR;
      }
      i = atoi(Tcl_GetString(objv[2]));
      Ns_MutexLock(&gdcMutex);
      for(gdc = gdcList;gdc;gdc = gdc->next) if(gdc->id == i) break;
      Ns_MutexUnlock(&gdcMutex);
      if(!gdc) {
        Tcl_WrongNumArgs(interp,3,objv,":unknown chart id");
        return TCL_ERROR;
      }
    }
    if(gdcDebug) Ns_Log(Debug,"ns_gdchart[%d]: %s %s %s",getpid(),Tcl_GetString(objv[1]),Tcl_GetString(objv[2]),Tcl_GetString(objv[3]));
    switch(cmd) {
    case cmdCreate:
	gdc = GDC_alloc();
	// Link new session to global session list
	Ns_MutexLock(&gdcMutex);
	gdc->id = ++gdcID;
	gdc->next = gdcList;
	if(gdcList) gdcList->prev = gdc;
	gdcList = gdc;
	Ns_MutexUnlock(&gdcMutex);
	Tcl_SetObjResult(interp,Tcl_NewIntObj(gdc->id));

    case cmdSet:
	for(i = cmd == cmdCreate ? 2 : 3; i < objc - 1; i += 2) {
	  if(Tcl_GetIndexFromObj(interp,objv[i],Opts,"subcmd",TCL_EXACT,(int *)&opt) != TCL_OK) return TCL_ERROR;
	  val = Tcl_GetString(objv[i+1]);
	  switch(opt) {
	   case optType:
               if(Tcl_GetIndexFromObj(interp,objv[i+1],Types,"type",TCL_EXACT,(int*)&gdc->type) != TCL_OK) return TCL_ERROR;
	       break;
           case optTitle:
               ns_free(gdc->title);
               gdc->title = ns_strdup(val);
               break;
           case optTitleFont:
               ns_free(gdc->title_font);
               gdc->title_font = ns_strdup(val);
               break;
           case optTitlePtSize:
               gdc->title_ptsize = atoi(val);
               break;
           case optXTitle:
               ns_free(gdc->xtitle);
               gdc->xtitle = ns_strdup(val);
               break;
           case optXTitleFont:
               ns_free(gdc->xtitle_font);
               gdc->xtitle_font = ns_strdup(val);
               break;
           case optXTitlePtSize:
               gdc->xtitle_ptsize = atof(val);
               break;
           case optYTitle:
               ns_free(gdc->ytitle);
               gdc->ytitle = ns_strdup(val);
               break;
           case optYTitleFont:
               ns_free(gdc->ytitle_font);
               gdc->ytitle_font = ns_strdup(val);
               break;
           case optYTitlePtSize:
               gdc->ytitle_ptsize = atof(val);
               break;
           case optWidth:
               gdc->width = atoi(val);
               break;
           case optHeight:
               gdc->height = atoi(val);
               break;
           case optHardWidth:
               gdc->hard_graphwidth = atoi(val);
               gdc->hard_size = 1;
               break;
           case optHardHeight:
               gdc->hard_grapheight = atoi(val);
               gdc->hard_size = 1;
               break;
           case optHardXOrig:
               gdc->hard_xorig = atoi(val);
               gdc->hard_size = 1;
               break;
           case optHardYOrig:
               gdc->hard_yorig = atoi(val);
               gdc->hard_size = 1;
               break;
           case optXAxis:
               gdc->xaxis = atoi(val);
               break;
           case optYAxis:
               gdc->yaxis = atoi(val);
               break;
           case optYAxis2:
               gdc->yaxis2 = atoi(val);
               break;
           case optYValStyle:
               gdc->yval_style = atoi(val);
               break;
           case optXAxisAngle:
               gdc->xaxis_angle = atoi(val);
               break;
           case optXAxisFont:
               ns_free(gdc->xaxis_font);
               gdc->xaxis_font = ns_strdup(val);
               break;
           case optXAxisPtSize:
               gdc->xaxis_ptsize = atof(val);
               break;
           case optBarWidth:
               gdc->bar_width = atoi(val);
               break;
           case optGridOnTop:
               gdc->grid_on_top = atoi(val);
               break;
           case optReqYMin:
               gdc->requested_ymin = atoi(val);
               break;
           case optReqYMax:
               gdc->requested_ymax = atoi(val);
               break;
           case optXLabelSpacing:
               gdc->xlabel_spacing = atoi(val);
               break;
           case optYlabelDensity:
               gdc->ylabel_density = atoi(val);
               break;
           case optReqYInterval:
               gdc->requested_yinterval = atoi(val);
               break;
           case optImageType:
               if(!strcasecmp(val,"png")) gdc->image_type = GDC_PNG; else
               if(!strcasecmp(val,"jpeg")) gdc->image_type = GDC_JPEG; else
               if(!strcasecmp(val,"wbmp")) gdc->image_type = GDC_WBMP;
               break;
           case optBgImage:
               ns_free(gdc->BGImage);
               gdc->BGImage = ns_strdup(val);
               break;
           case optYTitle2:
               ns_free(gdc->ytitle2);
               gdc->ytitle2 = ns_strdup(val);
               break;
           case optYTitleSize:
               gdc->ytitle_size = atoi(val);
               break;
           case opt3dDepth:
               gdc->threed_depth = atof(val);
               break;
           case opt3dAngle:
               gdc->threed_angle = atoi(val);
               break;
           case optGrid:
               gdc->grid = atoi(val);
               break;
           case optTicks:
               gdc->ticks = atoi(val);
               break;
           case optBorder:
               gdc->border = atoi(val);
               break;
           case optShelf:
               gdc->Shelf = atoi(val);
               break;
           case optStackType:
               if(!strcasecmp(val,"depth")) gdc->stack_type = GDC_STACK_DEPTH; else
               if(!strcasecmp(val,"sum")) gdc->stack_type = GDC_STACK_SUM; else
               if(!strcasecmp(val,"beside")) gdc->stack_type = GDC_STACK_BESIDE; else
               if(!strcasecmp(val,"layer")) gdc->stack_type = GDC_STACK_LAYER;
               break;
           case optYLabelFmt:
               ns_free(gdc->ylabel_fmt);
               gdc->ylabel_fmt = ns_strdup(val);
               break;
           case optYLabel2Fmt:
               ns_free(gdc->ylabel2_fmt);
               gdc->ylabel2_fmt = ns_strdup(val);
               break;
           case optBgColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->BGColor);
               break;
           case optLineColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->LineColor);
               break;
           case optPlotColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->PlotColor);
               break;
           case optVolColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->VolColor);
               break;
           case optTitleColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->TitleColor);
               break;
           case optXTitleColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->XTitleColor);
               break;
           case optYTitleColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->YTitleColor);
               break;
           case optYTitle2Color:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->YTitle2Color);
               break;
           case optXLabelColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->XLabelColor);
               break;
           case optYLabelColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->YLabelColor);
               break;
           case optYLabel2Color:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->YLabel2Color);
               break;
           case optGridColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->GridColor);
               break;
           case optScatterColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->ScatterColor);
               break;
           case optAnnoText:
               memset(gdc->annotation.note,0,MAX_NOTE_LEN+1);
               strncpy(gdc->annotation.note,val,MAX_NOTE_LEN);
               break;
           case optAnnoFont:
               ns_free(gdc->annotation_font);
               gdc->annotation_font = ns_strdup(val);
               break;
           case optAnnoFontSize:
               gdc->annotation_font_size = atoi(val);
               break;
           case optAnnoPtSize:
               gdc->annotation_ptsize = atof(val);
               break;
           case optAnnoPoint:
               gdc->annotation.point = atoi(val);
               break;
           case optAnnoColor:
               Tcl_GetLongFromObj(interp,objv[i+1],(long*)&gdc->annotation.color);
               break;
           case optInterpolations:
               gdc->interpolations = atoi(val);
               break;
           case optThumbnail:
               gdc->thumbnail = atoi(val);
               break;
           case optRandomSetColors:
               ns_free(gdc->SetColor);
               gdc->SetColor = (unsigned long*)ns_calloc(sizeof(int),gdc->num_sets);
               for(i = 0;i < gdc->num_sets;++i) gdc->SetColor[i] = (int)rand();
               break;
           case optRandomExtColors:
               ns_free(gdc->ExtColor);
               gdc->ExtColor = (unsigned long*)ns_calloc(sizeof(long),gdc->num_sets*gdc->num_points);
               for(k = 0;k < gdc->num_sets;++k)
                 for(j = 0;j < gdc->num_points;j++)
                   *(gdc->ExtColor+(k*gdc->num_points)+j) = (int)rand();
               break;
           case optHoldImage:
               gdc->hold = atoi(val);
               break;
           case optHLCStyle:
               gdc->HLC_style = 0;
               if(strstr(val,"diamond")) gdc->HLC_style |= GDC_HLC_DIAMOND;
               if(strstr(val,"closeconected")) gdc->HLC_style |= GDC_HLC_CLOSE_CONNECTED;
               if(strstr(val,"conecting")) gdc->HLC_style |= GDC_HLC_CONNECTING;
               if(strstr(val,"icap")) gdc->HLC_style |= GDC_HLC_I_CAP;
               break;
           case optHLCCapWidth:
               gdc->HLC_cap_width = atoi(val);
               break;
           case optLegendX:
               gdc->legend_x = atoi(val);
               break;
           case optLegendY:
               gdc->legend_y = atoi(val);
               break;
           case optLegend:
               if(!strcasecmp(val,"none")) gdc->legend = -1; else
               if(!strcasecmp(val,"right")) gdc->legend = 0; else
               if(!strcasecmp(val,"bottom")) gdc->legend = 1; else
               if(!strcasecmp(val,"top")) gdc->legend = 2;
               break;
	  }
	}
	break;

    case cmdDestroy:
	Ns_MutexLock(&gdcMutex);
	if(gdc->prev) gdc->prev->next = gdc->next;
	if(gdc->next) gdc->next->prev = gdc->prev;
	if(gdc == gdcList) gdcList = gdc->next;
	Ns_MutexUnlock(&gdcMutex);
        GDC_free(gdc);
	break;

    case cmdSave:
        ns_free(gdc->image_file);
        gdc->image_file = ns_strdup(Tcl_GetString(objv[3]));
        GDC_graph(gdc);
        break;

    case cmdImage: {
        int size;
        Tcl_Obj *result;
        unsigned char *data;

        gdc->hold = GDC_EXPOSE_IMAGE;
        GDC_graph(gdc);
        if((data = gdImagePngPtr(gdc->image, &size))) {
          result = Tcl_NewByteArrayObj(data,size);
          if(result != NULL) Tcl_SetObjResult(interp,result);
          gdFree(data);
        }
        break;
    }

    case cmdReturn: {
        char *data;
        int size, status = NS_ERROR;
        Ns_Conn *conn = Ns_TclGetConn(interp);

        if(conn == NULL) {
          Tcl_AppendResult(interp, "no connection", NULL);
          return TCL_ERROR;
        }
        gdc->hold = GDC_EXPOSE_IMAGE;
        GDC_graph(gdc);
        if((data = gdImagePngPtr(gdc->image, &size))) {
          status = Ns_ConnReturnData(conn,200,data,size,"image/png");
          gdFree(data);
        }
        Tcl_AppendResult(interp,status == NS_OK ? "1" : "0",NULL);
        break;
    }

    case cmdSetLabels:
        if(Tcl_ListObjGetElements(interp,objv[3],&argc,&argv) != TCL_OK) return TCL_ERROR;
        for(i = 0; i < argc; i++) {
          if(gdc->num_points >= gdc->alloc_num_points) {
            gdc->alloc_num_points += 50;
            gdc->xlabels = (char**)ns_realloc(gdc->xlabels,sizeof(char*)*gdc->alloc_num_points);
          }
          gdc->xlabels[gdc->num_points++] = ns_strdup(Tcl_GetString(argv[i]));
        }
        break;

    case cmdSetColors:
        if(Tcl_ListObjGetElements(interp,objv[3],&argc,&argv) != TCL_OK) return TCL_ERROR;
        ns_free(gdc->SetColor);
        gdc->SetColor = (unsigned long*)ns_calloc(sizeof(unsigned long),gdc->num_sets);
        for(i = 0,j = 0; i < argc && j < gdc->num_sets; i++,j++)
          Tcl_GetLongFromObj(0,argv[i],(long*)&gdc->SetColor[j]);
        break;

    case cmdSkipLabels:
        if(Tcl_ListObjGetElements(interp,objv[3],&argc,&argv) != TCL_OK) return TCL_ERROR;
        ns_free(gdc->xlabel_ctl);
        gdc->xlabel_ctl = (char*)ns_calloc(1,gdc->num_points);
        for(i = 0,j = 0; i < argc && j < gdc->num_points; i++,j++)
          gdc->xlabel_ctl[j] = atoi(Tcl_GetString(argv[i]));
        break;

    case cmdSetData:
        if(objc < 5) {
          Tcl_WrongNumArgs(interp,3,objv,"name datalist");
          return TCL_ERROR;
        }
        if(Tcl_ListObjGetElements(interp,objv[4],&argc,&argv) != TCL_OK) return TCL_ERROR;
        gdc->num_sets++;
        gdc->dlabels = (char**)ns_realloc(gdc->dlabels,sizeof(char*)*gdc->num_sets);
        gdc->dlabels[gdc->num_sets-1] = ns_strdup(Tcl_GetString(objv[3]));
        gdc->data = (float*)ns_realloc(gdc->data,sizeof(float)*gdc->num_sets*gdc->num_points);
	for(i = 0,j = 0; i < argc && j < gdc->num_points; i++,j++)
          *(gdc->data+(gdc->num_sets-1)*gdc->num_points+j) = *(val = Tcl_GetString(argv[i])) == '*' ? GDC_NOVALUE : atof(val);
	break;
    }
    return TCL_OK;
}


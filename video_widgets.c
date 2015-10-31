/*****************************************************************************
 * video_widgets.c : OSD widgets manipulation functions
 *****************************************************************************
 * Copyright (C) 2004-2010 VLC authors and VideoLAN
 * $Id: 201f90773c51c56eeaec6b724f323f62584579a7 $
 *
 * Author: Yoann Peronneau <yoann@videolan.org>
 *         Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <assert.h>

#include <vlc_common.h>
#include <vlc_vout.h>
#include <vlc_vout_osd.h>

#include <vlc_filter.h>

/* fill style enum */
#define STYLE_EMPTY 0
#define STYLE_FILLED 1

/* overlay palette enum */
#define P_WHITE 1
#define P_BLACK 2


/* Size of OSD Icon, relative to larger side of video frame */
#define ICON_SIZE 0.08

/* margin from top-right */
#define ICON_MARGIN 0.07

/* highlight white margin, relative to icon size */
#define ICON_H_MARGIN 0.03

/* global variable, defines current color */
static uint8_t cur_color;

/**
 * Draws a rectangle at the given position in the region.
 * It may be filled (fill == STYLE_FILLED) or empty (fill == STYLE_EMPTY).
 */
static void DrawRect(subpicture_region_t *r, int fill,
                     int x1, int y1, int x2, int y2)
{
    uint8_t *p    = r->p_picture->p->p_pixels;
    int     pitch = r->p_picture->p->i_pitch;

    if (fill == STYLE_FILLED) {
        for (int y = y1; y <= y2; y++) {
            for (int x = x1; x <= x2; x++)
                p[x + pitch * y] = cur_color;
        }
    } else {
        for (int y = y1; y <= y2; y++) {
            p[x1 + pitch * y] = cur_color;
            p[x2 + pitch * y] = cur_color;
        }
        for (int x = x1; x <= x2; x++) {
            p[x + pitch * y1] = cur_color;
            p[x + pitch * y2] = cur_color;
        }
    }
}

/**
 * Draws a triangle at the given position in the region.
 * It may be filled (fill == STYLE_FILLED) or empty (fill == STYLE_EMPTY).
 */
static void DrawTriangle(subpicture_region_t *r, int fill,
                         int x1, int y1, int x2, int y2)
{
    uint8_t *p    = r->p_picture->p->p_pixels;
    int     pitch = r->p_picture->p->i_pitch;
    const int mid = y1 + (y2 - y1) / 2;

    /* TODO factorize it */
    if (x2 >= x1) {
        if (fill == STYLE_FILLED) {
            for (int y = y1; y <= mid; y++) {
                int h = y - y1;
                for (int x = x1; x <= x1 + h && x <= x2; x++) {
                    p[x + pitch * y         ] = cur_color;
                    p[x + pitch * (y2 - h)] = cur_color;
                }
            }
        } else {
            for (int y = y1; y <= mid; y++) {
                int h = y - y1;
                p[x1 +     pitch * y         ] = cur_color;
                p[x1 + h + pitch * y         ] = cur_color;
                p[x1 +     pitch * (y2 - h)] = cur_color;
                p[x1 + h + pitch * (y2 - h)] = cur_color;
            }
        }
    } else {
        if( fill == STYLE_FILLED) {
            for (int y = y1; y <= mid; y++) {
                int h = y - y1;
                for (int x = x1; x >= x1 - h && x >= x2; x--) {
                    p[x + pitch * y       ] = cur_color;
                    p[x + pitch * (y2 - h)] = cur_color;
                }
            }
        } else {
            for (int y = y1; y <= mid; y++) {
                int h = y - y1;
                p[ x1 +     pitch * y       ] = cur_color;
                p[ x1 - h + pitch * y       ] = cur_color;
                p[ x1 +     pitch * (y2 - h)] = cur_color;
                p[ x1 - h + pitch * (y2 - h)] = cur_color;
            }
        }
    }
}

/**
 * Create a region with a white transparent picture.
 */
static subpicture_region_t *OSDRegion(int x, int y, int width, int height)
{
    video_palette_t palette = {
        .i_entries = 3,
        .palette = {
            [0] = { 0xff, 0x80, 0x80, 0x00 },
            [P_WHITE] = { 0xff, 0x80, 0x80, 0xff },
            [P_BLACK] = { 0x00, 0x80, 0x80, 0xff },
        },
    };

    video_format_t fmt;
    video_format_Init(&fmt, VLC_CODEC_YUVP);
    fmt.i_width          =
    fmt.i_visible_width  = width;
    fmt.i_height         =
    fmt.i_visible_height = height;
    fmt.i_sar_num        = 1;
    fmt.i_sar_den        = 1;
    fmt.p_palette        = &palette;

    subpicture_region_t *r = subpicture_region_New(&fmt);
    if (!r)
        return NULL;
    r->i_x = x;
    r->i_y = y;
    memset(r->p_picture->p->p_pixels, 0, r->p_picture->p->i_pitch * height);

    return r;
}

/**
 * Create the region for an OSD slider.
 * Types are: OSD_HOR_SLIDER and OSD_VERT_SLIDER.
 */
static subpicture_region_t *OSDSlider(int type, int position,
                                      const video_format_t *fmt)
{
    const int size = __MAX(fmt->i_visible_width, fmt->i_visible_height);
    const int margin = size * 0.10;

    int x, y;
    int width, height;
    if (type == OSD_HOR_SLIDER) {
        width  = __MAX(fmt->i_visible_width - 2 * margin, 1);
        height = __MAX(fmt->i_visible_height * 0.05,      1);
        x      = __MIN(fmt->i_x_offset + margin, fmt->i_visible_width - width);
        y      = __MAX(fmt->i_y_offset + fmt->i_visible_height - margin, 0);
    } else {
        width  = __MAX(fmt->i_visible_width * 0.025,       1);
        height = __MAX(fmt->i_visible_height - 2 * margin, 1);
        x      = __MAX(fmt->i_x_offset + fmt->i_visible_width - margin, 0);
        y      = __MIN(fmt->i_y_offset + margin, fmt->i_visible_height - height);
    }

    subpicture_region_t *r = OSDRegion(x, y, width, height);
    if( !r)
        return NULL;

    if (type == OSD_HOR_SLIDER) {
        int pos_x = (width - 2) * position / 100;
        DrawRect(r, STYLE_FILLED, pos_x - 1, 2, pos_x + 1, height - 3);
        DrawRect(r, STYLE_EMPTY,  0,         0, width - 1, height - 1);
    } else {
        int pos_mid = height / 2;
        int pos_y   = height - (height - 2) * position / 100;
        DrawRect(r, STYLE_FILLED, 2,         pos_y,   width - 3, height - 3);
        DrawRect(r, STYLE_FILLED, 1,         pos_mid, 1,         pos_mid   );
        DrawRect(r, STYLE_FILLED, width - 2, pos_mid, width - 2, pos_mid   );
        DrawRect(r, STYLE_EMPTY,  0,         0,       width - 1, height - 1);
    }
    return r;
}


/* margin assumes there IS margin, at least one pixel */
static void DrawRect_margin(subpicture_region_t *r, int fill,
                     int x1, int y1, int x2, int y2, int margin) {
    if( margin < 1 ) margin = 1;
    if( x1 + 2 * margin >= x2 ) return;
    if( y1 + 2 * margin >= y2 ) return;
	DrawRect( r, fill, x1 + margin, y1 + margin, x2 - margin, y2 - margin );
}

static void DrawTriangle_margin(subpicture_region_t *r, int fill,
                         int x1, int y1, int x2, int y2, double margin) {
    if( margin < 1 ) margin = 1;
    int x_margin = margin;
	if( x1 >= x2 ) x_margin = -margin;
	else x_margin = margin;
	if( (int)( y1 + 2.4142 * margin ) >= (int)( y2 - 2.4142 * margin ) ) return;
	DrawTriangle( r, fill, x1 + (int)x_margin, y1 + 2.4142 * margin, x2 - (int)x_margin, y2 - 2.4142 * margin );

}


static void draw_pause_icon ( subpicture_region_t *r, int width, int height ) {
        int bar_width = width / 3;
        cur_color = P_WHITE;
        DrawRect( r, STYLE_FILLED, 0, 0, bar_width - 1, height -1 );
        DrawRect( r, STYLE_FILLED, width - bar_width, 0, width - 1, height - 1 );
        cur_color = P_BLACK;
        DrawRect_margin( r, STYLE_FILLED, 0, 0, bar_width - 1, height -1, width * ICON_H_MARGIN );
        DrawRect_margin( r, STYLE_FILLED, width - bar_width, 0, width - 1, height - 1, width * ICON_H_MARGIN );
}

static void draw_play_icon ( subpicture_region_t *r, int width, int height ) {
        int mid   = height >> 1;
        int delta = (width - mid) >> 1;
        int y2    = height - 1;
        cur_color = P_WHITE;
        DrawTriangle(r, STYLE_FILLED, delta, 0, width - delta, y2);
        cur_color = P_BLACK;
        DrawTriangle_margin(r, STYLE_FILLED, delta, 0, width - delta, y2, width * ICON_H_MARGIN);
}

static void draw_speaker_icon ( subpicture_region_t *r, int width, int height ) {
    int mid   = height >> 1;
    int delta = (width - mid) >> 1;
    int y2    = height - 1;
    cur_color = P_WHITE;
    DrawRect(r, STYLE_FILLED, delta, mid / 2, width - delta, height - 1 - mid / 2);
    DrawTriangle(r, STYLE_FILLED, width - delta, 0, delta, y2);
    
    cur_color = P_BLACK;
    DrawRect_margin(r, STYLE_FILLED, delta, mid / 2, width - delta, height - 1 - mid / 2, width * ICON_H_MARGIN );
    DrawTriangle_margin(r, STYLE_FILLED, width - delta, 0, delta, y2, width * ICON_H_MARGIN );
}
 
/**
 * Create the region for an OSD slider.
 * Types are: OSD_PLAY_ICON, OSD_PAUSE_ICON, OSD_SPEAKER_ICON, OSD_MUTE_ICON
 */
static subpicture_region_t *OSDIcon(int type, const video_format_t *fmt)
{
    const float size_ratio   = ICON_SIZE;
    const float margin_ratio = ICON_MARGIN;

    const int size   = __MAX(fmt->i_visible_width, fmt->i_visible_height);
    const int width  = size * size_ratio;
    const int height = size * size_ratio;
    const int x      = fmt->i_x_offset + fmt->i_visible_width - margin_ratio * size - width;
    const int y      = fmt->i_y_offset                        + margin_ratio * size;

    subpicture_region_t *r = OSDRegion(__MAX(x, 0),
                                       __MIN(y, (int)fmt->i_visible_height - height),
                                       width, height);
    if (!r)
        return NULL;
        
    if ( size < 4 ) return r;
    /* In such extreme conditions icon is one of last things to worry about */

    if (type == OSD_PAUSE_ICON) {
		draw_pause_icon( r, width, height );
    } else if (type == OSD_PLAY_ICON) {
		draw_play_icon( r, width, height );
    } else {
    	draw_speaker_icon( r, width, height );
    }
    return r;
}

struct subpicture_updater_sys_t {
    int type;
    int position;
};

static int OSDWidgetValidate(subpicture_t *subpic,
                           bool has_src_changed, const video_format_t *fmt_src,
                           bool has_dst_changed, const video_format_t *fmt_dst,
                           mtime_t ts)
{
    VLC_UNUSED(subpic); VLC_UNUSED(ts);
    VLC_UNUSED(fmt_src); VLC_UNUSED(has_src_changed);
    VLC_UNUSED(fmt_dst);

    if (!has_dst_changed)
        return VLC_SUCCESS;
    return VLC_EGENERIC;
}

static void OSDWidgetUpdate(subpicture_t *subpic,
                          const video_format_t *fmt_src,
                          const video_format_t *fmt_dst,
                          mtime_t ts)
{
    subpicture_updater_sys_t *sys = subpic->updater.p_sys;
    VLC_UNUSED(fmt_src); VLC_UNUSED(ts);

    video_format_t fmt = *fmt_dst;
    fmt.i_width         = fmt.i_width         * fmt.i_sar_num / fmt.i_sar_den;
    fmt.i_visible_width = fmt.i_visible_width * fmt.i_sar_num / fmt.i_sar_den;
    fmt.i_x_offset      = fmt.i_x_offset      * fmt.i_sar_num / fmt.i_sar_den;
    fmt.i_sar_num       = 1;
    fmt.i_sar_den       = 1;

    subpic->i_original_picture_width  = fmt.i_visible_width;
    subpic->i_original_picture_height = fmt.i_visible_height;
    cur_color = P_WHITE;
    if (sys->type == OSD_HOR_SLIDER || sys->type == OSD_VERT_SLIDER)
        subpic->p_region = OSDSlider(sys->type, sys->position, &fmt);
    else
        subpic->p_region = OSDIcon(sys->type, &fmt);
}

static void OSDWidgetDestroy(subpicture_t *subpic)
{
    free(subpic->updater.p_sys);
}

static void OSDWidget(vout_thread_t *vout, int channel, int type, int position)
{
    if (!var_InheritBool(vout, "osd"))
        return;
    if (type == OSD_HOR_SLIDER || type == OSD_VERT_SLIDER)
        position = VLC_CLIP(position, 0, 100);

    subpicture_updater_sys_t *sys = malloc(sizeof(*sys));
    if (!sys)
        return;
    sys->type     = type;
    sys->position = position;

    subpicture_updater_t updater = {
        .pf_validate = OSDWidgetValidate,
        .pf_update   = OSDWidgetUpdate,
        .pf_destroy  = OSDWidgetDestroy,
        .p_sys       = sys,
    };
    subpicture_t *subpic = subpicture_New(&updater);
    if (!subpic) {
        free(sys);
        return;
    }

    subpic->i_channel  = channel;
    subpic->i_start    = mdate();
    subpic->i_stop     = subpic->i_start + 1200000;
    subpic->b_ephemer  = true;
    subpic->b_absolute = true;
    subpic->b_fade     = true;

    vout_PutSubpicture(vout, subpic);
}

void vout_OSDSlider(vout_thread_t *vout, int channel, int position, short type)
{
    OSDWidget(vout, channel, type, position);
}

void vout_OSDIcon(vout_thread_t *vout, int channel, short type )
{
    OSDWidget(vout, channel, type, 0);
}

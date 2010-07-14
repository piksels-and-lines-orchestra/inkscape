/*
 * A container class for filter slots. Allows for simple getting and
 * setting images in filter slots without having to bother with
 * table indexes and such.
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 Niko Kiirala
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <assert.h>
#include <string.h>

#include <2geom/transforms.h>
#include "display/cairo-utils.h"
#include "display/nr-arena-item.h"
#include "display/nr-filter-types.h"
#include "display/nr-filter-gaussian.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"
#include "display/pixblock-scaler.h"
#include "display/pixblock-transform.h"
#include "libnr/nr-pixblock.h"
#include "libnr/nr-blit.h"

__attribute__ ((const))
inline static int _max4(const double a, const double b,
                        const double c, const double d) {
    double ret = a;
    if (b > ret) ret = b;
    if (c > ret) ret = c;
    if (d > ret) ret = d;
    return (int)round(ret);
}

__attribute__ ((const))
inline static int _min4(const double a, const double b,
                        const double c, const double d) {
    double ret = a;
    if (b < ret) ret = b;
    if (c < ret) ret = c;
    if (d < ret) ret = d;
    return (int)round(ret);
}

__attribute__ ((const))
inline static int _max2(const double a, const double b) {
    if (a > b)
        return (int)round(a);
    else
        return (int)round(b);
}

__attribute__ ((const))
inline static int _min2(const double a, const double b) {
    if (a > b)
        return (int)round(b);
    else
        return (int)round(a);
}

namespace Inkscape {
namespace Filters {

FilterSlot::FilterSlot(NRArenaItem *item, cairo_t *bgct, NRRectL const *bgarea,
        cairo_surface_t *graphic, NRRectL const *graphicarea, FilterUnits const &u)
    : _item(item)
    , _source_graphic(graphic)
    , _background_ct(bgct)
    , _source_graphic_area(graphicarea)
    , _background_area(bgarea)
    , _units(u)
    , _last_out(NR_FILTER_SOURCEGRAPHIC)
    , filterquality(FILTER_QUALITY_BEST)
    , blurquality(BLUR_QUALITY_BEST)
{
    using Geom::X;
    using Geom::Y;

    // compute slot bbox
    Geom::Rect bbox(
        Geom::Point(_source_graphic_area->x0, _source_graphic_area->y0),
        Geom::Point(_source_graphic_area->x1, _source_graphic_area->y1));

    Geom::Matrix trans = _units.get_matrix_display2pb();

    Geom::Rect bbox_trans = bbox * trans;
    Geom::Point min = bbox_trans.min();
    Geom::Point max = bbox_trans.max();
    _slot_area.x0 = floor(min[X]);
    _slot_area.y0 = floor(min[Y]);
    _slot_area.x1 = ceil(max[X]);
    _slot_area.y1 = ceil(max[Y]);
}

FilterSlot::~FilterSlot()
{
    for (SlotMap::iterator i = _slots.begin(); i != _slots.end(); ++i) {
        cairo_surface_destroy(i->second);
    }
}

cairo_surface_t *FilterSlot::getcairo(int slot_nr)
{
    //int index = _get_index(slot_nr);
    //assert(index >= 0);

    if (slot_nr == NR_FILTER_SLOT_NOT_SET)
        slot_nr = _last_out;

    SlotMap::iterator s = _slots.find(slot_nr);

    /* If we didn't have the specified image, but we could create it
     * from the other information we have, let's do that */
    if (s == _slots.end()
        && (slot_nr == NR_FILTER_SOURCEGRAPHIC
            || slot_nr == NR_FILTER_SOURCEALPHA
            || slot_nr == NR_FILTER_BACKGROUNDIMAGE
            || slot_nr == NR_FILTER_BACKGROUNDALPHA
            || slot_nr == NR_FILTER_FILLPAINT
            || slot_nr == NR_FILTER_STROKEPAINT))
    {
        switch (slot_nr) {
            case NR_FILTER_SOURCEGRAPHIC: {
                cairo_surface_t *tr = _get_transformed_source_graphic();
                _set_internal(NR_FILTER_SOURCEGRAPHIC, tr);
                cairo_surface_destroy(tr);
            } break;
            case NR_FILTER_BACKGROUNDIMAGE: {
                cairo_surface_t *bg = _get_transformed_background();
                _set_internal(NR_FILTER_BACKGROUNDIMAGE, bg);
                cairo_surface_destroy(bg);
            } break;
            case NR_FILTER_SOURCEALPHA: {
                cairo_surface_t *src = getcairo(NR_FILTER_SOURCEGRAPHIC);
                cairo_surface_t *alpha = ink_cairo_extract_alpha(src);
                _set_internal(NR_FILTER_SOURCEALPHA, alpha);
                cairo_surface_destroy(alpha);
            } break;
            case NR_FILTER_BACKGROUNDALPHA: {
                cairo_surface_t *src = getcairo(NR_FILTER_BACKGROUNDIMAGE);
                cairo_surface_t *ba = ink_cairo_extract_alpha(src);
                _set_internal(NR_FILTER_BACKGROUNDALPHA, ba);
                cairo_surface_destroy(ba);
            } break;
            case NR_FILTER_FILLPAINT: //TODO
            case NR_FILTER_STROKEPAINT: //TODO
            default:
                break;
        }
        s = _slots.find(slot_nr);
    }

    if (s == _slots.end()) {
        // create empty surface
        cairo_surface_t *empty = cairo_surface_create_similar(
            _source_graphic, cairo_surface_get_content(_source_graphic),
            _slot_area.x1 - _slot_area.x0, _slot_area.y1 - _slot_area.y0);
        _set_internal(slot_nr, empty);
        cairo_surface_destroy(empty);
        s = _slots.find(slot_nr);
    }
    return s->second;

    //assert(slot_nr == NR_FILTER_SLOT_NOT_SET ||_slot_number[index] == slot_nr);
    //return _slot[index];
}

cairo_surface_t *FilterSlot::_get_transformed_source_graphic()
{
    Geom::Matrix trans = _units.get_matrix_display2pb();

    cairo_surface_t *tsg = cairo_surface_create_similar(
        _source_graphic, cairo_surface_get_content(_source_graphic),
        _slot_area.x1 - _slot_area.x0, _slot_area.y1 - _slot_area.y0);
    cairo_t *tsg_ct = cairo_create(tsg);

    cairo_translate(tsg_ct, -_slot_area.x0, -_slot_area.y0);
    ink_cairo_transform(tsg_ct, trans);
    cairo_translate(tsg_ct, _source_graphic_area->x0, _source_graphic_area->y0);
    cairo_set_source_surface(tsg_ct, _source_graphic, 0, 0);
    cairo_set_operator(tsg_ct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(tsg_ct);
    cairo_destroy(tsg_ct);

    return tsg;
}

cairo_surface_t *FilterSlot::_get_transformed_background()
{
    Geom::Matrix trans = _units.get_matrix_display2pb();

    cairo_surface_t *bg = cairo_get_target(_background_ct);
    cairo_surface_t *tbg = cairo_surface_create_similar(
        bg, cairo_surface_get_content(bg),
        _slot_area.x1 - _slot_area.x0, _slot_area.y1 - _slot_area.y0);
    cairo_t *tbg_ct = cairo_create(tbg);

    cairo_translate(tbg_ct, -_slot_area.x0, -_slot_area.y0);
    ink_cairo_transform(tbg_ct, trans);
    cairo_translate(tbg_ct, _background_area->x0, _background_area->y0);
    cairo_set_source_surface(tbg_ct, bg, 0, 0);
    cairo_set_operator(tbg_ct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(tbg_ct);
    cairo_destroy(tbg_ct);

    return tbg;
}

cairo_surface_t *FilterSlot::get_result(int res)
{
    Geom::Matrix trans = _units.get_matrix_pb2display();

    cairo_surface_t *r = cairo_surface_create_similar(_source_graphic,
        cairo_surface_get_content(_source_graphic),
        _source_graphic_area->x1 - _source_graphic_area->x0,
        _source_graphic_area->y1 - _source_graphic_area->y0);
    cairo_t *r_ct = cairo_create(r);

    cairo_translate(r_ct, -_source_graphic_area->x0, -_source_graphic_area->y0);
    ink_cairo_transform(r_ct, trans);
    cairo_translate(r_ct, _slot_area.x0, _slot_area.y0);
    cairo_set_source_surface(r_ct, getcairo(res), 0, 0);
    cairo_set_operator(r_ct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(r_ct);
    cairo_destroy(r_ct);

    return r;
}
/*
void FilterSlot::get_final(int slot_nr, NRPixBlock *result) {
    NRPixBlock *final_usr = get(slot_nr);
    Geom::Matrix trans = units.get_matrix_pb2display();

    int size = (result->area.x1 - result->area.x0)
        * (result->area.y1 - result->area.y0)
        * NR_PIXBLOCK_BPP(result);
    memset(NR_PIXBLOCK_PX(result), 0, size);

    if (fabs(trans[1]) > 1e-6 || fabs(trans[2]) > 1e-6) {
        if (filterquality == FILTER_QUALITY_BEST) {
            NR::transform_bicubic(result, final_usr, trans);
        } else {
            NR::transform_nearest(result, final_usr, trans);
        }
    } else if (fabs(trans[0] - 1) > 1e-6 || fabs(trans[3] - 1) > 1e-6) {
        NR::scale_bicubic(result, final_usr, trans);
    } else {
        nr_blit_pixblock_pixblock(result, final_usr);
    }
}*/

void FilterSlot::_set_internal(int slot_nr, cairo_surface_t *surface)
{
    // destroy after referencing
    // this way assigning a surface to a slot it already occupies will not cause errors
    cairo_surface_reference(surface);

    SlotMap::iterator s = _slots.find(slot_nr);
    if (s != _slots.end()) {
        cairo_surface_destroy(s->second);
    }

    _slots[slot_nr] = surface;
}

void FilterSlot::set(int slot_nr, cairo_surface_t *surface)
{
    g_return_if_fail(surface != NULL);

    if (slot_nr == NR_FILTER_SLOT_NOT_SET)
        slot_nr = NR_FILTER_UNNAMED_SLOT;

    _set_internal(slot_nr, surface);
    _last_out = slot_nr;

#if 0
    /* Unnamed slot is for saving filter primitive results, when parameter
     * 'result' is not set. Only the filter immediately after this one
     * can access unnamed results, so we don't have to worry about overwriting
     * previous results in filter chain. On the other hand, we may not
     * overwrite any other image with this one, because they might be
     * accessed later on. */
    int index = ((slot_nr != NR_FILTER_SLOT_NOT_SET)
                 ? _get_index(slot_nr)
                 : _get_index(NR_FILTER_UNNAMED_SLOT));
    assert(index >= 0);
    // Unnamed slot is only for Inkscape::Filters::FilterSlot internal use.
    assert(slot_nr != NR_FILTER_UNNAMED_SLOT);
    assert(slot_nr == NR_FILTER_SLOT_NOT_SET ||_slot_number[index] == slot_nr);

    if (slot_nr == NR_FILTER_SOURCEGRAPHIC || slot_nr == NR_FILTER_BACKGROUNDIMAGE) {
        Geom::Matrix trans = units.get_matrix_display2pb();
        if (fabs(trans[1]) > 1e-6 || fabs(trans[2]) > 1e-6) {
            NRPixBlock *trans_pb = new NRPixBlock;
            int x0 = pb->area.x0;
            int y0 = pb->area.y0;
            int x1 = pb->area.x1;
            int y1 = pb->area.y1;
            int min_x = _min4(trans[0] * x0 + trans[2] * y0 + trans[4],
                              trans[0] * x0 + trans[2] * y1 + trans[4],
                              trans[0] * x1 + trans[2] * y0 + trans[4],
                              trans[0] * x1 + trans[2] * y1 + trans[4]);
            int max_x = _max4(trans[0] * x0 + trans[2] * y0 + trans[4],
                              trans[0] * x0 + trans[2] * y1 + trans[4],
                              trans[0] * x1 + trans[2] * y0 + trans[4],
                              trans[0] * x1 + trans[2] * y1 + trans[4]);
            int min_y = _min4(trans[1] * x0 + trans[3] * y0 + trans[5],
                              trans[1] * x0 + trans[3] * y1 + trans[5],
                              trans[1] * x1 + trans[3] * y0 + trans[5],
                              trans[1] * x1 + trans[3] * y1 + trans[5]);
            int max_y = _max4(trans[1] * x0 + trans[3] * y0 + trans[5],
                              trans[1] * x0 + trans[3] * y1 + trans[5],
                              trans[1] * x1 + trans[3] * y0 + trans[5],
                              trans[1] * x1 + trans[3] * y1 + trans[5]);

            nr_pixblock_setup_fast(trans_pb, pb->mode,
                                   min_x, min_y,
                                   max_x, max_y, true);
            if (trans_pb->size != NR_PIXBLOCK_SIZE_TINY && trans_pb->data.px == NULL) {
                /* TODO: this gets hit occasionally. Worst case scenario:
                 * images are exported in horizontal stripes. One stripe
                 * is not too high, but can get thousands of pixels wide.
                 * Rotate this 45 degrees -> _huge_ image */
                g_warning("Memory allocation failed in Inkscape::Filters::FilterSlot::set (transform)");
                return;
            }
            if (filterquality == FILTER_QUALITY_BEST) {
                NR::transform_bicubic(trans_pb, pb, trans);
            } else {
                NR::transform_nearest(trans_pb, pb, trans);
            }
            nr_pixblock_release(pb);
            delete pb;
            pb = trans_pb;
        } else if (fabs(trans[0] - 1) > 1e-6 || fabs(trans[3] - 1) > 1e-6) {
            NRPixBlock *trans_pb = new NRPixBlock;

            int x0 = pb->area.x0;
            int y0 = pb->area.y0;
            int x1 = pb->area.x1;
            int y1 = pb->area.y1;
            int min_x = _min2(trans[0] * x0 + trans[4],
                              trans[0] * x1 + trans[4]);
            int max_x = _max2(trans[0] * x0 + trans[4],
                              trans[0] * x1 + trans[4]);
            int min_y = _min2(trans[3] * y0 + trans[5],
                              trans[3] * y1 + trans[5]);
            int max_y = _max2(trans[3] * y0 + trans[5],
                              trans[3] * y1 + trans[5]);

            nr_pixblock_setup_fast(trans_pb, pb->mode,
                                   min_x, min_y, max_x, max_y, true);
            if (trans_pb->size != NR_PIXBLOCK_SIZE_TINY && trans_pb->data.px == NULL) {
                g_warning("Memory allocation failed in Inkscape::Filters::FilterSlot::set (scaling)");
                return;
            }
            NR::scale_bicubic(trans_pb, pb, trans);
            nr_pixblock_release(pb);
            delete pb;
            pb = trans_pb;
        }
    }

    if(_slot[index]) {
        nr_pixblock_release(_slot[index]);
        delete _slot[index];
    }
    _slot[index] = pb;
    _last_out = index;
#endif
}

int FilterSlot::get_slot_count()
{
    return _slots.size();
    /*
    int seek = _slot_count;
    do {
        seek--;
    } while (!_slot[seek] && _slot_number[seek] == NR_FILTER_SLOT_NOT_SET);

    return seek + 1;*/
}

int FilterSlot::_get_index(int slot_nr)
{
#if 0
    assert(slot_nr >= 0 ||
           slot_nr == NR_FILTER_SLOT_NOT_SET ||
           slot_nr == NR_FILTER_SOURCEGRAPHIC ||
           slot_nr == NR_FILTER_SOURCEALPHA ||
           slot_nr == NR_FILTER_BACKGROUNDIMAGE ||
           slot_nr == NR_FILTER_BACKGROUNDALPHA ||
           slot_nr == NR_FILTER_FILLPAINT ||
           slot_nr == NR_FILTER_STROKEPAINT ||
           slot_nr == NR_FILTER_UNNAMED_SLOT);

    int index = -1;
    if (slot_nr == NR_FILTER_SLOT_NOT_SET) {
        return _last_out;
    }
    /* Search, if the slot already exists */
    for (int i = 0 ; i < _slot_count ; i++) {
        if (_slot_number[i] == slot_nr) {
            index = i;
            break;
        }
    }

    /* If the slot doesn't already exist, create it */
    if (index == -1) {
        int seek = _slot_count;
        do {
            seek--;
        } while ((seek >= 0) && (_slot_number[seek] == NR_FILTER_SLOT_NOT_SET));
        /* If there is no space for more slots, create more space */
        if (seek == _slot_count - 1) {
            NRPixBlock **new_slot = new NRPixBlock*[_slot_count * 2];
            int *new_number = new int[_slot_count * 2];
            for (int i = 0 ; i < _slot_count ; i++) {
                new_slot[i] = _slot[i];
                new_number[i] = _slot_number[i];
            }
            for (int i = _slot_count ; i < _slot_count * 2 ; i++) {
                new_slot[i] = NULL;
                new_number[i] = NR_FILTER_SLOT_NOT_SET;
            }
            delete[] _slot;
            delete[] _slot_number;
            _slot = new_slot;
            _slot_number = new_number;
            _slot_count *= 2;
        }
        /* Now that there is space, create the slot */
        _slot_number[seek + 1] = slot_nr;
        index = seek + 1;
    }
    return index;
#endif
    return 0;
}

void FilterSlot::set_quality(FilterQuality const q) {
    filterquality = q;
}

void FilterSlot::set_blurquality(int const q) {
    blurquality = q;
}

int FilterSlot::get_blurquality(void) {
    return blurquality;
}

} /* namespace Filters */
} /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :

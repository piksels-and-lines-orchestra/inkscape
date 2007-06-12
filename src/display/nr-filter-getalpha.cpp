/*
 * Functions for extracting alpha channel from NRPixBlocks.
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 Niko Kiirala
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/nr-filter-getalpha.h"
#include "libnr/nr-blit.h"
#include "libnr/nr-pixblock.h"

namespace NR {

NRPixBlock *filter_get_alpha(NRPixBlock *src)
{
    NRPixBlock *dst = new NRPixBlock;
    nr_pixblock_setup_fast(dst, NR_PIXBLOCK_MODE_R8G8B8A8P,
                           src->area.x0, src->area.y0,
                           src->area.x1, src->area.y1, false);
    nr_blit_pixblock_pixblock(dst, src);

    unsigned char *data = NR_PIXBLOCK_PX(dst);
    int end = dst->rs * (dst->area.y1 - dst->area.y0);
    for (int i = 0 ; i < end ; i += 4) {
        data[i + 0] = 0;
        data[i + 1] = 0;
        data[i + 2] = 0;
    }
    dst->empty = false;

    return dst;
}

} // namespace NR

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

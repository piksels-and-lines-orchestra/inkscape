/*
 * feConvolveMatrix filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *
 * Copyright (C) 2007,2009 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <vector>
#include "display/cairo-templates.h"
#include "display/cairo-utils.h"
#include "display/nr-filter-convolve-matrix.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"
#include "display/nr-filter-utils.h"

namespace Inkscape {
namespace Filters {

FilterConvolveMatrix::FilterConvolveMatrix()
{}

FilterPrimitive * FilterConvolveMatrix::create() {
    return new FilterConvolveMatrix();
}

FilterConvolveMatrix::~FilterConvolveMatrix()
{}

template<bool PRESERVE_ALPHA, bool X_LOWER, bool X_UPPER, bool Y_LOWER, bool Y_UPPER>
static inline void convolve2D_XY(unsigned int const x, unsigned int const y, guint32 *const out_data, guint32 const *const in_data, unsigned int const width, unsigned int const height, double const *const kernel, unsigned int const orderX, unsigned int const orderY, unsigned int const targetX, unsigned int const targetY, double const bias) {
    double result_R = 0;
    double result_G = 0;
    double result_B = 0;
    double result_A = 0;

    unsigned int iBegin = Y_LOWER ? targetY-y : 0; // Note that to prevent signed/unsigned problems this requires that y<=targetY (which is true)
    unsigned int iEnd   = Y_UPPER ? height+targetY-y : orderY; // And this requires that y<=height+targetY (which is trivially true), in addition it should be true that height+targetY-y<=orderY (or equivalently y>=height+targetY-orderY, which is true)
    unsigned int jBegin = X_LOWER ? targetX-x : 0;
    unsigned int jEnd   = X_UPPER ? width+targetX-x : orderX;

    for (unsigned int i=iBegin; i<iEnd; i++){
        for (unsigned int j=jBegin; j<jEnd; j++){
            unsigned int index = x - targetX + j + width*(y - targetY + i);
            unsigned int kernel_index = orderX-j-1 + orderX*(orderY-i-1);
            guint32 px = in_data[index];
            EXTRACT_ARGB32(px, a,r,g,b)

            double k = kernel[kernel_index];
            result_R += r * k;
            result_G += g * k;
            result_B += b * k;
            result_A += a * k;
        }
    }

    unsigned int const out_index = x + width*y;
    guint32 ao;
    if (PRESERVE_ALPHA) {
        ao = (in_data[out_index] & 0xff000000) >> 24;
    } else {
        ao = CLAMP_D_TO_U8(result_A + 255*bias);
    }

    guint32 ro = CLAMP_D_TO_U8_ALPHA(result_R + ao*bias, ao); // CLAMP includes rounding!
    guint32 go = CLAMP_D_TO_U8_ALPHA(result_G + ao*bias, ao);
    guint32 bo = CLAMP_D_TO_U8_ALPHA(result_B + ao*bias, ao);

    ASSEMBLE_ARGB32(result, ao,ro,go,bo)

    out_data[out_index] = result;
}

template<bool PRESERVE_ALPHA, bool Y_LOWER, bool Y_UPPER>
static inline void convolve2D_Y(unsigned int const y, guint32 *const out_data, guint32 const *const in_data, unsigned int const width, unsigned int const height, double const *const kernel, unsigned int const orderX, unsigned int const orderY, unsigned int const targetX, unsigned int const targetY, double const bias) {
    // See convolve2D below for rationale.

    unsigned int const lowerEnd = std::min(targetX,width);
    unsigned int const upperBegin = width - std::min<unsigned int>(width,orderX - 1u - targetX);
    unsigned int const midXBegin = std::min(lowerEnd,upperBegin);
    unsigned int const midXEnd = std::max(lowerEnd,upperBegin);

    for (unsigned int x=0; x<midXBegin; x++) {
        convolve2D_XY<PRESERVE_ALPHA,true,false,Y_LOWER,Y_UPPER>(x, y, out_data, in_data, width, height, kernel, orderX, orderY, targetX, targetY, bias);
    }
    if (lowerEnd==upperBegin) {
        // Do nothing, empty mid section
    } else if (lowerEnd<upperBegin) {
        // In the middle no bounds have to be adjusted
        for (unsigned int x=midXBegin; x<midXEnd; x++) {
            convolve2D_XY<PRESERVE_ALPHA,false,false,Y_LOWER,Y_UPPER>(x, y, out_data, in_data, width, height, kernel, orderX, orderY, targetX, targetY, bias);
        }
    } else {
        // In the middle both bounds have to be adjusted
        for (unsigned int x=midXBegin; x<midXEnd; x++) {
            convolve2D_XY<PRESERVE_ALPHA,true,true,Y_LOWER,Y_UPPER>(x, y, out_data, in_data, width, height, kernel, orderX, orderY, targetX, targetY, bias);
        }
    }
    for (unsigned int x=midXEnd; x<width; x++) {
        convolve2D_XY<PRESERVE_ALPHA,false,true,Y_LOWER,Y_UPPER>(x, y, out_data, in_data, width, height, kernel, orderX, orderY, targetX, targetY, bias);
    }
}

template<bool PRESERVE_ALPHA>
static void convolve2D(guint32 *const out_data, guint32 const *const in_data, unsigned int const width, unsigned int const height, double const *const kernel, unsigned int const orderX, unsigned int const orderY, unsigned int const targetX, unsigned int const targetY, double const _bias) {
    double const bias = _bias;

    // For the middle section it should hold that (for all i such that 0<=i<orderY):
    //   0 <= y - targetY + i < height
    //   targetY <= y && y < height + targetY - orderY + 1
    // In other words, for y<targetY i's lower bound needs to be adjusted and for y>=height+targetY-orderY+1 i's upper bound needs to be adjusted.

    unsigned int const lowerEnd = std::min(targetY,height);
    unsigned int const upperBegin = height - std::min<unsigned int>(height,orderY - 1u - targetY);
    unsigned int const midYBegin = std::min(lowerEnd,upperBegin);
    unsigned int const midYEnd = std::max(lowerEnd,upperBegin);

    for (unsigned int y=0; y<midYBegin; y++) {
        convolve2D_Y<PRESERVE_ALPHA,true,false>(y, out_data, in_data, width, height, kernel, orderX, orderY, targetX, targetY, bias);
    }
    if (lowerEnd==upperBegin) {
        // Do nothing, empty mid section
    } else if (lowerEnd<upperBegin) {
        // In the middle no bounds have to be adjusted
        for (unsigned int y=midYBegin; y<midYEnd; y++) {
            convolve2D_Y<PRESERVE_ALPHA,false,false>(y, out_data, in_data, width, height, kernel, orderX, orderY, targetX, targetY, bias);
        }
    } else {
        // In the middle both bounds have to be adjusted
        for (unsigned int y=midYBegin; y<midYEnd; y++) {
            convolve2D_Y<PRESERVE_ALPHA,true,true>(y, out_data, in_data, width, height, kernel, orderX, orderY, targetX, targetY, bias);
        }
    }
    for (unsigned int y=midYEnd; y<height; y++) {
        convolve2D_Y<PRESERVE_ALPHA,false,true>(y, out_data, in_data, width, height, kernel, orderX, orderY, targetX, targetY, bias);
    }
}

/*
struct ConvolveMatrix {
    ConvolveMatrix(guint32 *px, int yskip, int targetX, int targetY, int orderX, int orderY,
            double divisor, double bias, PixelAccessor::EdgeMode emode,
            std::vector<double> const &kernel)
        : _kernel(kernel.size())
//        , _in(in, emode)
        , _tx(targetX), _ty(targetY)
        , _oX(orderX), _oY(orderY)
        , _yskip(yskip)
        , _bias(bias)
    {
        for (unsigned i = 0; i < kernel.size(); ++i) {
            _kernel[i] = kernel[i] / divisor;
        }
    }

    guint32 operator()(int x, int y) {
        int start_x = x - _tX;
        int start_y = y - _tY;

        double ro = 0, go = 0, bo = 0, ao = 0;

        for (int i = 0; i < _oY; ++i) {
            for (int j = 0; j < _oX; ++j) {
                guint32 in = pixelAt(start_x + j, start_y + i);
                EXTRACT_ARGB(in, a,r,g,b)

                unsigned kidx = i*_oY + j;
                double k = kernel[]

                ro += r * 
            }
        }
        
    }

private:
    inline guint32 pixelAt(int x, int y) {
        return *(_px + y * _yskip + x);
    }

    std::vector<double> _kernel;
    guint32 *_px;
    // PixelAccessor _in;
    double _bias;
    int _tX, _tY, _oX, _oY, _yskip;
}; */

void FilterConvolveMatrix::render_cairo(FilterSlot &slot)
{
    static bool bias_warning = false;
    static bool edge_warning = false;

    cairo_surface_t *input = slot.getcairo(_input);

    if (orderX<=0 || orderY<=0) {
        g_warning("Empty kernel!");
        return;
    }
    if (targetX<0 || targetX>=orderX || targetY<0 || targetY>=orderY) {
        g_warning("Invalid target!");
        return;
    }
    if (kernelMatrix.size()!=(unsigned int)(orderX*orderY)) {
        g_warning("kernelMatrix does not have orderX*orderY elements!");
        return;
    }

    cairo_surface_t *out = ink_cairo_surface_create_identical(input);

    if (bias!=0 && !bias_warning) {
        g_warning("It is unknown whether Inkscape's implementation of bias in feConvolveMatrix "
                  "is correct!");
        bias_warning = true;
        // The SVG specification implies that feConvolveMatrix is defined for premultiplied
        // colors (which makes sense). It also says that bias should simply be added to the result
        // for each color (without taking the alpha into account). However, it also says that one
        // purpose of bias is "to have .5 gray value be the zero response of the filter".
        // It seems sensible to indeed support the latter behaviour instead of the former,
        // but this does appear to go against the standard.
        // Note that Batik simply does not support bias!=0
    }
    if (edgeMode!=CONVOLVEMATRIX_EDGEMODE_NONE && !edge_warning) {
        g_warning("Inkscape only supports edgeMode=\"none\" (and a filter uses a different one)!");
        edge_warning = true;
    }

    guint32 *in_data = reinterpret_cast<guint32*>(cairo_image_surface_get_data(input));
    guint32 *out_data = reinterpret_cast<guint32*>(cairo_image_surface_get_data(out));

    int width = cairo_image_surface_get_width(input);
    int height = cairo_image_surface_get_height(input);

    // Set up predivided kernel matrix
    std::vector<double> kernel(kernelMatrix);
    for(size_t i=0; i<kernel.size(); i++) {
        kernel[i] /= divisor; // The code that creates this object makes sure that divisor != 0
    }

    if (preserveAlpha) {
        convolve2D<true>(out_data, in_data, width, height, &kernel.front(), orderX, orderY,
            targetX, targetY, bias);
    } else {
        convolve2D<false>(out_data, in_data, width, height, &kernel.front(), orderX, orderY,
            targetX, targetY, bias);
    }

    slot.set(_output, out);
    cairo_surface_destroy(out);
}

void FilterConvolveMatrix::set_targetX(int coord) {
    targetX = coord;
}

void FilterConvolveMatrix::set_targetY(int coord) {
    targetY = coord;
}

void FilterConvolveMatrix::set_orderX(int coord) {
    orderX = coord;
}

void FilterConvolveMatrix::set_orderY(int coord) {
    orderY = coord;
}

void FilterConvolveMatrix::set_divisor(double d) {
    divisor = d;
}

void FilterConvolveMatrix::set_bias(double b) {
    bias = b;
}

void FilterConvolveMatrix::set_kernelMatrix(std::vector<gdouble> &km) {
    kernelMatrix = km;
}

void FilterConvolveMatrix::set_edgeMode(FilterConvolveMatrixEdgeMode mode){
    edgeMode = mode;
}

void FilterConvolveMatrix::set_preserveAlpha(bool pa){
    preserveAlpha = pa;
}

void FilterConvolveMatrix::area_enlarge(NRRectL &area, Geom::Matrix const &/*trans*/)
{
    //Seems to me that since this filter's operation is resolution dependent,
    // some spurious pixels may still appear at the borders when low zooming or rotating. Needs a better fix.
    area.x0 -= targetX;
    area.y0 -= targetY;
    area.x1 += orderX - targetX - 1; // This makes sure the last row/column in the original image corresponds to the last row/column in the new image that can be convolved without adjusting the boundary conditions).
    area.y1 += orderY - targetY - 1;
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

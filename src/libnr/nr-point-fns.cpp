#include "libnr/nr-point-fns.h"

Geom::Point
snap_vector_midpoint (Geom::Point const &p, Geom::Point const &begin, Geom::Point const &end, double snap)
{
    double length = Geom::distance(begin, end);
    Geom::Point be = (end - begin) / length;
    double r = Geom::dot(p - begin, be);

    if (r < 0.0) return begin;
    if (r > length) return end;

    double snapdist = length * snap;
    double r_snapped = (snap==0) ? r : floor(r/(snapdist + 0.5)) * snapdist;

    return (begin + r_snapped * be);
}

// equivalent to Geom::LineSegment(begin, end).nearestPoint(p)
double
get_offset_between_points (Geom::Point const &p, Geom::Point const &begin, Geom::Point const &end)
{
    double length = Geom::distance(begin, end);
    Geom::Point be = (end - begin) / length;
    double r = Geom::dot(p - begin, be);

    if (r < 0.0) return 0.0;
    if (r > length) return 1.0;

    return (r / length);
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

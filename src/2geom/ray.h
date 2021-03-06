/**
 * \file
 * \brief  Infinite straight ray
 *//*
 * Copyright 2008  Marco Cecchetti <mrcekets at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 */

#ifndef LIB2GEOM_SEEN_RAY_H
#define LIB2GEOM_SEEN_RAY_H

#include <vector>
#include <2geom/point.h>
#include <2geom/bezier-curve.h> // for LineSegment
#include <2geom/exception.h>
#include <2geom/math-utils.h>

namespace Geom
{

/**
 * @brief Straight ray from a specific point to infinity.
 *
 * Rays are "half-lines" - they begin at some specific point and extend in a straight line
 * to infinity.
 *
 * @ingroup Primitives
 */
class Ray {
private:
    Point _origin;
    Point _versor;

public:
    Ray() : _origin(0,0), _versor(1,0) {}
    Ray(Point const& origin, Coord angle)
        : _origin(origin)
    {
        sincos(angle, _versor[Y], _versor[X]);
    }
    Ray(Point const& A, Point const& B) {
        setPoints(A, B);
    }
    Point origin() const { return _origin; }
    Point versor() const { return _versor; }
    void setOrigin(Point const &o) { _origin = o; }
    void setVersor(Point const& v) { _versor = v; }
    Coord angle() const { return std::atan2(_versor[Y], _versor[X]); }
    void setAngle(Coord a) { sincos(a, _versor[Y], _versor[X]); }
    void setPoints(Point const &a, Point const &b) {
        _origin = a;
        _versor = b - a;
        if (are_near(_versor, Point(0,0)) )
            _versor = Point(0,0);
        else
            _versor.normalize();
    }
    bool isDegenerate() const {
        return ( _versor[X] == 0 && _versor[Y] == 0 );
    }
    Point pointAt(Coord t) const {
        return _origin + _versor * t;
    }
    Coord valueAt(Coord t, Dim2 d) const {
        return _origin[d] + _versor[d] * t;
    }
    std::vector<Coord> roots(Coord v, Dim2 d) const {
        std::vector<Coord> result;
        if ( _versor[d] != 0 ) {
            double t = (v - _origin[d]) / _versor[d];
            if (t >= 0)	result.push_back(t);
        } else if (_versor[(d+1)%2] == v) {
            THROW_INFINITESOLUTIONS();
        }
        return result;
    }
    Coord nearestPoint(Point const& point) const {
        if ( isDegenerate() ) return 0;
        double t = dot(point - _origin, _versor);
        if (t < 0) t = 0;
        return t;
    }
    Ray reverse() const {
        Ray result;
        result.setOrigin(_origin);
        result.setVersor(-_versor);
        return result;
    }
    Curve *portion(Coord f, Coord t) const {
        return new LineSegment(pointAt(f), pointAt(t));
    }
    LineSegment segment(Coord f, Coord t) const {
        return LineSegment(pointAt(f), pointAt(t));
    }
    Ray transformed(Affine const& m) const {
        return Ray(_origin * m, (_origin + _versor) * m);
    }
}; // end class Ray

inline
double distance(Point const& _point, Ray const& _ray) {
	double t = _ray.nearestPoint(_point);
	return ::Geom::distance(_point, _ray.pointAt(t));
}

inline
bool are_near(Point const& _point, Ray const& _ray, double eps = EPSILON) {
	return are_near(distance(_point, _ray), 0, eps);
}

inline
bool are_same(Ray const& r1, Ray const& r2, double eps = EPSILON) {
	return are_near(r1.versor(), r2.versor(), eps)
			&& are_near(r1.origin(), r2.origin(), eps);
}

// evaluate the angle between r1 and r2 rotating r1 in cw or ccw direction on r2
// the returned value is an angle in the interval [0, 2PI[
inline
double angle_between(Ray const& r1, Ray const& r2, bool cw = true) {
	double angle = angle_between(r1.versor(), r2.versor());
	if (angle < 0) angle += 2*M_PI;
	if (!cw) angle = 2*M_PI - angle;
	return angle;
}

inline
Ray make_angle_bisector_ray(Ray const& r1, Ray const& r2)
{
    if ( !are_near(r1.origin(), r2.origin()) )
    {
        THROW_RANGEERROR("passed rays have not the same origin");
    }

    Point M = middle_point(r1.pointAt(1), r2.pointAt(1) );
    if (angle_between(r1, r2) > M_PI)  M = 2 * r1.origin() - M;
    return Ray(r1.origin(), M);
}

}  // end namespace Geom

#endif // LIB2GEOM_SEEN_RAY_H

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

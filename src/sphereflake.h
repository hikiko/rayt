/*
Path_tracer - A CPU path tracer

Copyright (C) 2013 Eleni Maria Stea

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Author: Eleni Maria Stea <elene.mst@gmail.com>
*/

#ifndef SPHEREFLAKE_H_
#define SPHEREFLAKE_H_

#include <vector>
#include "object.h"
#include "sphere.h"

class SphereFlake : public Object {
private:
	Sphere *sph;
	SphereFlake *subflakes[6];

	Vector3 center;
	double radius;

public:
	SphereFlake(const Vector3 &center, double radius);
	~SphereFlake();

	bool intersection(const Ray &ray, IntInfo* i_info) const;

	void calc_bbox();

	friend SphereFlake *create_sflake(const Vector3 &center, double radius, int iter);
};

SphereFlake *create_sflake(const Vector3 &center, double radius, int iter);

#endif

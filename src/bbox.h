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

#ifndef BBOX_H_
#define BBOX_H_

#include <vector>
#include "vector.h"
#include "ray.h"
#include "intinfo.h"

class Object;

class BBox {
public:
	Vector3 min, max;

	BBox();
	BBox(const Vector3 &min, const Vector3 &max);
	
	bool intersection(const Ray &ray) const;
};

class BBoxNode {
private:
	BBox bbox;
	std::vector<BBoxNode*> children;
	std::vector<Object*> objects;
public:
	BBoxNode(const BBox &bbox);
	~BBoxNode();
	
	bool intersection(const Ray &ray, IntInfo* inf) const;
	void add_child(BBoxNode* node);
	void add_object(Object* obj);
};

#endif

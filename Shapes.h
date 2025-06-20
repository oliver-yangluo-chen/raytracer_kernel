//#ifndef SHAPES_H
//#define SHAPES_H
#pragma once
#include <math.h>
#include "Misc.h"
//#include "Constants.h"
using namespace std;


struct Intersection {
  double t = -1;
  Vec3 pos;
  Vec3 norm;
};

class Shape{
public:
  Color color;
  virtual Intersection intersection(Ray r) = 0;
};

class Sphere: public Shape{
public:
  Vec3 pos = {0, 0, 0};
  double rad;

  Sphere(Vec3 pos_, double rad_, Color color_);
  Intersection intersection(Ray r);
};

class Plane: public Shape{
public:
  Vec3 normal; //orientation (direction from origin to p), unit vector
  double dist; //multiple of normal (point of intersection)

  Plane(Vec3 p_, double dist_, Color color_);
  Intersection intersection(Ray r);
};
//#endif

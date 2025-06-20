#include "Shapes.h"
using namespace std;


Sphere::Sphere(Vec3 pos_, double rad_, Color color_){
  pos = pos_;
  rad = rad_;
  color = color_;
}

Intersection Sphere::intersection(Ray r){ //return time
  Intersection ans;

  Vec3 oc = r.start - pos;
  double a = dot(r.dir, r.dir);
  double b = 2 * dot(oc, r.dir);
  double c = dot(oc,oc) - rad*rad;
  double discriminant = b*b - 4*a*c;

  if (discriminant < 0) return ans;

  ans.t = (-b - sqrt(discriminant)) / (2*a);
  ans.pos = r(ans.t);
  ans.norm = normalize(pos - ans.pos);

  return ans;
}

Plane::Plane(Vec3 normal_, double dist_, Color color_){
  normal = normal_;
  dist = dist_;
  color = color_;
}

Intersection Plane::intersection(Ray r){
  Intersection ans;

  double denom = dot(normal, r.dir);
  if(denom != 0){
    ans.t =  (dist - (dot(normal, r.start)) / dot(normal, r.dir));
    ans.pos = r(ans.t);
    ans.norm = normal;
  }
  return ans; //else 0 0 0
}

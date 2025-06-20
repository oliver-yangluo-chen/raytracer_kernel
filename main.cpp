#include "Constants.h"
#include "Misc.h"
#include "Shapes.h"
#include "Image.h"
#include "Camera.h"
#include <stdlib.h>
#include <time.h>
using namespace std;


/*
diffuse lighting

World class
contains a vector of shapes
ray -> color
also contains a diffuse / background color

reflection: d - 2 * dot(n, d) * n
*/

int main(){
  srand(time(NULL));

  /*
  Vec3 CAMPOS = {0, 0.5, 0};
  Vec3 CAMFORWARD = {0, 0, 1};
  Vec3 CAMUP = {0, 1, 0};
  */
  Vec3 camright = cross(CAMFORWARD, CAMUP);
  Camera Cam(CAMPOS, CAMFORWARD, CAMUP, camright, FOV, ASPECT);

  Image image(X, Y, WHITE);

  Sphere sphere({0, 0, 50}, 30, RED);
  Sphere sphere2({0, 5, 15}, 4, GREEN);
  Plane plane({0, 1, 0}, 0, BLUE);

  vector<Shape*> shapes = {&sphere2, &plane, &sphere};

  cerr << "finished initial calcs" << endl;
  for(double i = 0; i < X; ++i){
    for(double j = 0; j < Y; ++j){
      double lX = lerp(-1, 1, i/(X-1)); //lerped X
      double lY = lerp(-1, 1, j/(Y-1)); //lerped Y

      Vec3 dest = Cam.view(lX, lY);
      Ray ray = {CAMPOS, dest - CAMPOS};
      ray.dir = normalize(ray.dir);
 
      Color minColor = BLACK;
      image.setPixel(i, j, BLACK);
      double minTime = -1;
      for(const auto shape: shapes){
        const Intersection inter = shape->intersection(ray);
        if (inter.t < 0) continue;
        if (minTime < 0 || inter.t < minTime) {
          //cout << i << "," << j << "\n";
          minTime = inter.t;
          
          Color c = shape->color;
          c.red = 256*(inter.norm.x+1)/2;
          c.green = 256*(inter.norm.y+1)/2;
          c.blue = 256*(inter.norm.z+1)/2;
          image.setPixel(i, j, c);
        }
        /*
        double curTime = 0;
        Color curColor = BLACK;
        if(ANTI_ALIASING){
          for(int k = 0; k < CYCLES; ++k){
            Vec3 deviation = {(double)rand()/RAND_MAX - 0.5, (double)rand()/RAND_MAX - 0.5, (double)rand()/RAND_MAX - 0.5};

            double time = shape->intersection({ray.start, ray.dir + deviation/BLUR});
            if(time > 0) curColor = curColor + shape->color;
            curTime += time;
          }
          curColor = curColor/CYCLES;
          curTime /= CYCLES;
        }
        else{
          curTime = shape->intersection(ray);
          if(curTime > 0) curColor = shape->color;
        }
        if(minTime == -1 || (curTime < minTime && curTime > 0)){
          minTime = curTime;
          minColor = curColor;
        }
        */
      }
      //minColor.red = (int) (127 * (lX + 1));
      //minColor.green = (int) (127 * (lY + 1));
      //minColor.blue = 0;


      //image.setPixel(i, j, minColor);
    }
  }
  image.writeToFile("circle.ppm");
  cerr << "finished drawing" << endl;
}

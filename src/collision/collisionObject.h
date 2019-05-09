#ifndef COLLISIONOBJECT
#define COLLISIONOBJECT

#include <nanogui/nanogui.h>

#include "../clothMesh.h"

using namespace CGL;
using namespace std;
using namespace nanogui;

class CollisionObject {
public:
  virtual void render(GLShader &shader, bool is_paused, bool draw_track) = 0;
  virtual void collide(PointMass &pm) = 0;

private:
  double friction;
};

#endif /* COLLISIONOBJECT */

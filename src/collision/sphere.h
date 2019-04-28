#ifndef COLLISIONOBJECT_SPHERE_H
#define COLLISIONOBJECT_SPHERE_H

#include "../clothMesh.h"
#include "../misc/sphere_drawing.h"
#include "collisionObject.h"

using namespace CGL;
using namespace std;

struct SphereParameters {

};

struct Sphere : public CollisionObject {
public:
  Sphere(const Vector3D &origin, double radius, double friction, int num_lat = 40, int num_lon = 40)
      : origin(origin), radius(radius), radius2(radius * radius),
        friction(friction), m_sphere_mesh(Misc::SphereMesh(num_lat, num_lon)) {}

  void render(GLShader &shader);
  void collide(PointMass &pm);

private:
  Vector3D origin;
  double radius;
  double radius2;
  //Added to File
  double velocity;
  vector<PointMass> spheremass;

  void simulate(double frames_per_sec, double simulation_steps, SphereParameters *cp,
                vector<Vector3D> external_accelerations,
                vector<CollisionObject *> *collision_objects);

  void reset();
  //end of Added Files

  double friction;
  
  Misc::SphereMesh m_sphere_mesh;
};

#endif /* COLLISIONOBJECT_SPHERE_H */

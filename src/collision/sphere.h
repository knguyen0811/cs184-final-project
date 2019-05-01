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
    void render(GLShader &shader);
    void collide(PointMass &pm);
    Sphere(const Vector3D &origin, double radius, double friction, Vector3D &velocity, long double mass=1e-5, int num_lat = 40, int num_lon = 40)
            : origin(origin), startOrigin(origin), radius(radius), radius2(radius * radius), log_radius(std::log10(radius)),
            velocity(velocity), startVelocity(velocity), mass(mass), friction(friction),
            pm(PointMass(origin, false)), m_sphere_mesh(Misc::SphereMesh(num_lat, num_lon)) {}
    Vector3D gravity(Sphere &other_sphere);
    //Vector3D get_pos();
    void add_force(Vector3D force);
    void verlet(double delta_t);
    void reset();
private:
    PointMass pm;
    Vector3D origin;
    Vector3D velocity;
    Vector3D startOrigin;
    Vector3D startVelocity;
    const double radius;
    const double radius2;
    const double log_radius; // for rendering in logarithmic scale
    const long double mass; // integer?
    double friction;

    Misc::SphereMesh m_sphere_mesh;
};

#endif /* COLLISIONOBJECT_SPHERE_H */
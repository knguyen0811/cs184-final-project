#ifndef COLLISIONOBJECT_SPHERE_H
#define COLLISIONOBJECT_SPHERE_H

#include "../clothMesh.h"
#include "../misc/sphere_drawing.h"
#include "collisionObject.h"

#define seconds 1
#define hours 3600
#define days 86400
#define years 31536000
#define sphere_factor 1E9

using namespace CGL;
using namespace std;

struct SphereParameters {
    SphereParameters() {}

    // New Planet Variables
    Vector3D newOrigin;
    Vector3D newVelocity;
    double newRadius;
    long double newMass;
    double minMultiplier = 2.f;
    double maxMultiplier = 3.f;
    bool button_pushed = false;
    int delIndex = 1;
};

struct Sphere : public CollisionObject {
public:
    void render(GLShader &shader, bool is_paused);
    void collide(PointMass &pm);
    Sphere(const Vector3D &origin, double radius, double friction, Vector3D &velocity, long double mass=1e-5, int texture_id=0, int num_lat = 40, int num_lon = 40)
            : origin(origin), startOrigin(origin), radius(radius), radius2(radius * radius), log_radius(std::log10(radius)),
            velocity(velocity), startVelocity(velocity), mass(mass), friction(friction), addTrack(true),
            pm(PointMass(origin, false)), m_sphere_mesh(Misc::SphereMesh(num_lat, num_lon)), texture_id(texture_id) {}
    //Vector3D get_pos();

    // Our Functions
    Vector3D gravity(Sphere &other_sphere);
    void add_force(Vector3D force);
    void verlet(double delta_t);
    void reset();
    void isTrackEnd(Vector3D track_start, double distance);
    void setTextureID(int id);

    // Get Functions
    Vector3D getInitOrigin();
    Vector3D getInitVelocity();
    double getRadius();
    long double getMass();
private:
    PointMass pm;
    Vector3D origin;
    Vector3D velocity;
    std::vector<Vector3D> track = std::vector<Vector3D>();
    Vector3D startOrigin;
    Vector3D startVelocity;
    const double radius;
    const double radius2;
    const double log_radius; // for rendering in logarithmic scale
    const long double mass;
    double friction;
    bool addTrack;
    Misc::SphereMesh m_sphere_mesh;
    int texture_id;
};

#endif /* COLLISIONOBJECT_SPHERE_H */
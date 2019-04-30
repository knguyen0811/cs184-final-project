//
// Created by Milo Piazza on 2019-04-30.
//
#include "clothMesh.h"
#include "misc/sphere_drawing.h"

#ifndef CLOTHSIM_PLANET_H
#define CLOTHSIM_PLANET_H


class Planet {
public:
    Planet(const Vector3D &origin, double radius, double mass, double friction, int num_lat = 40, int num_lon = 40)
            : origin(origin), radius(radius), radius2(radius * radius), log_radius(std::log10(radius)), mass(mass),
              friction(friction), pm(PointMass(origin, false)), m_sphere_mesh(Misc::SphereMesh(num_lat, num_lon)) {}

    void render(GLShader &shader);
    void collide(PointMass &pm);
    Vector3D gravity(Planet &other_planet);
    Vector3D get_pos();
    void add_force(Vector3D force);
    void verlet(double delta_t, double damping = 0.);
private:
    PointMass pm;
    Vector3D origin;
    const double radius;
    const double radius2;
    const double log_radius; // for rendering in logarithmic scale
    const double mass; // integer?
    double friction;

    Misc::SphereMesh m_sphere_mesh;
};


#endif //CLOTHSIM_PLANET_H

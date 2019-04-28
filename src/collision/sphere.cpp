#include <nanogui/nanogui.h>

#include "../clothMesh.h"
#include "../misc/sphere_drawing.h"
#include "sphere.h"

using namespace nanogui;
using namespace CGL;

void Sphere::collide(PointMass &pm) {
    // TODO (Part 3): Handle collisions with spheres.
    // FIXME: Check if inside sphere val < r^2
    double x,y,z;
    x = pow(pm.position.x - origin.x, 2);
    y = pow(pm.position.y - origin.y, 2);
    z = pow(pm.position.z - origin.z, 2);
    double val = x + y + z;
    if (val <= radius2) {
        Vector3D dir, normDir, tanPoint, correct;
        dir = pm.position - origin;
        normDir = dir.unit();
        tanPoint = normDir * radius + origin;
        correct = tanPoint - pm.last_position;
        pm.position = pm.last_position + (1.f - friction) * correct;
    }
}

void Sphere::render(GLShader &shader) {
    // We decrease the radius here so flat triangles don't behave strangely
    // and intersect with the sphere when rendered
    m_sphere_mesh.draw_sphere(shader, origin, radius * 0.92);
}

//Added to File
void Sphere::simulate(double frames_per_sec, double simulation_steps, SphereParameters *cp,
                vector<Vector3D> external_accelerations,
                vector<CollisionObject *> *collision_objects) {
    //Start Code Here


}

void Sphere::reset(){

}
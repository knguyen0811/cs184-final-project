#include <nanogui/nanogui.h>

#include "../clothMesh.h"
#include "../misc/sphere_drawing.h"
#include "sphere.h"

using namespace nanogui;
using namespace CGL;

#include "../clothMesh.h"
#include "../misc/sphere_drawing.h"

#define G 6.67408e-11

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

Vector3D Sphere::gravity(Sphere &other_sphere) {
  Vector3D dir = other_sphere.pm.position - pm.position;
  double r = dir.norm();
  dir.normalize();
  // Divide by r once to normalize dir, then twice more for the gravitation equation
  return dir * G * mass * other_sphere.mass / (r * r);
}

void Sphere::add_force(Vector3D force) {
  pm.forces += force;
}

void Sphere::verlet(double delta_t) {
  Vector3D new_pos = pm.position + velocity + (pm.forces / mass) / 2.f;
  
  // std::cout << "forces: " << pm.forces << "\n";
  // std::cout << "velocity: " << velocity << "\n";
  // std::cout << "mass: " << mass << "\n";
  // std::cout << "forces / mass: " << pm.forces / mass << "\n";
  // std::cout << "Accleration factor: " << (pm.forces / mass) / 2.f << "\n";

  // std::cout << "pos: " << pm.position << "\n";
  // std::cout << "new_pos: " << new_pos << "\n";
  // std::cout << "\n\n\n";

  //pm.last_position = pm.position;
  velocity = new_pos - pm.position;
  pm.position = new_pos;
  pm.forces = Vector3D();
}

void Sphere::render(GLShader &shader) {
  // We decrease the radius here so flat triangles don't behave strangely
  // and intersect with the sphere when rendered
  m_sphere_mesh.draw_sphere(shader, pm.position / 1E6, radius * 0.92);
}

Vector3D Sphere::getInitOrigin() {
    return startOrigin;
}


void Sphere::reset() {
    this->origin = startOrigin;
    this->velocity = startVelocity;
    pm.position = pm.start_position;
    pm.last_position = pm.start_position;
}
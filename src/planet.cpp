//
// Created by Milo Piazza on 2019-04-30.
//
#include "clothMesh.h"
#include "misc/sphere_drawing.h"
#include "planet.h"

#define G 6.67408e-11

/*void Planet::collide(PointMass &pm) {
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
}*/

Vector3D Planet::gravity(Planet &other_planet) {
  Vector3D dir = origin - other_planet.origin;
  double r = dir.norm();
  // Divide by r once to normalize dir, then twice more for the gravitation equation
  return dir * G * mass * other_planet.mass / (r * r * r);
}

void Planet::add_force(Vector3D force) {
  pm.forces += force;
}

void Planet::verlet(double delta_t, double damping) {
  Vector3D new_pos = pm.position + (1 - damping) * (pm.position - pm.last_position) + (pm.forces / mass) * (delta_t * delta_t);
  pm.last_position = pm.position;
  pm.position = new_pos;
  pm.forces = Vector3D();
}

void Planet::render(GLShader &shader) {
  // We decrease the radius here so flat triangles don't behave strangely
  // and intersect with the sphere when rendered
  m_sphere_mesh.draw_sphere(shader, pm.position, log_radius * 0.92);
}

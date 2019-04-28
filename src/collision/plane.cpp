#include "iostream"
#include <nanogui/nanogui.h>

#include "../clothMesh.h"
#include "../galaxySimulator.h"
#include "../leak_fix.h"
#include "plane.h"

using namespace std;
using namespace CGL;

#define SURFACE_OFFSET 0.0001

void Plane::collide(PointMass &pm) {
    // TODO (Part 3): Handle collisions with planes.
    // FIXME: Check if below plane
    if (dot(normal, pm.position - point) < 0) {
        Vector3D dir, tanPoint, correct;
        dir = point - pm.position;
//        tanPoint = pm.position + dot(dir, normal) + normal * SURFACE_OFFSET;
        tanPoint = pm.position + dot(dir, normal);
        correct = tanPoint - pm.last_position + normal * SURFACE_OFFSET;
        pm.position = pm.last_position + (1.f - friction) * correct;
    }
}

void Plane::render(GLShader &shader) {
    nanogui::Color color(0.7f, 0.7f, 0.7f, 1.0f);

    Vector3f sPoint(point.x, point.y, point.z);
    Vector3f sNormal(normal.x, normal.y, normal.z);
    Vector3f sParallel(normal.y - normal.z, normal.z - normal.x,
                       normal.x - normal.y);
    sParallel.normalize();
    Vector3f sCross = sNormal.cross(sParallel);

    MatrixXf positions(3, 4);
    MatrixXf normals(3, 4);

    positions.col(0) << sPoint + 2 * (sCross + sParallel);
    positions.col(1) << sPoint + 2 * (sCross - sParallel);
    positions.col(2) << sPoint + 2 * (-sCross + sParallel);
    positions.col(3) << sPoint + 2 * (-sCross - sParallel);

    normals.col(0) << sNormal;
    normals.col(1) << sNormal;
    normals.col(2) << sNormal;
    normals.col(3) << sNormal;

    if (shader.uniform("u_color", false) != -1) {
        shader.setUniform("u_color", color);
    }
    shader.uploadAttrib("in_position", positions);
    if (shader.attrib("in_normal", false) != -1) {
        shader.uploadAttrib("in_normal", normals);
    }

    shader.drawArray(GL_TRIANGLE_STRIP, 0, 4);
#ifdef LEAK_PATCH_ON
    shader.freeAttrib("in_position");
  if (shader.attrib("in_normal", false) != -1) {
    shader.freeAttrib("in_normal");
  }
#endif
}

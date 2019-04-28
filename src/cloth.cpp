#include <iostream>
#include <math.h>
#include <random>
#include <vector>

#include "cloth.h"
#include "collision/plane.h"
#include "collision/sphere.h"

using namespace std;

Cloth::Cloth(double width, double height, int num_width_points,
             int num_height_points, float thickness) {
    this->width = width;
    this->height = height;
    this->num_width_points = num_width_points;
    this->num_height_points = num_height_points;
    this->thickness = thickness;

    buildGrid();
    buildClothMesh();
}

Cloth::~Cloth() {
    point_masses.clear();
    springs.clear();

    if (clothMesh) {
        delete clothMesh;
    }
}

int Cloth::getIndex(int i, int j) {
    return j*num_width_points+i;
}

void Cloth::buildGrid() {
    // TODO (Part 1): Build a grid of masses and springs.
    // FIXME: Point masses
    double x, y, z;
    if (orientation == HORIZONTAL) {
        z = 0;
        for (int j = 0; j < num_height_points; j++) {
            x = 0;
            for (int i = 0; i < num_width_points; i++) {
                Vector3D pos(x, 1, z);
                PointMass pm(pos, false);
                point_masses.emplace_back(pm);
                x += width/num_width_points;
            }
            z += height/num_height_points;
        }
    } else {
        y = 0;
        for (int j = 0; j < num_height_points; j++) {
            x = 0;
            for (int i = 0; i < num_width_points; i++) {
                z = ((rand() / RAND_MAX) - 1.f) / 1000.f;
                Vector3D pos(x, y, z);
                PointMass pm(pos, false);
                point_masses.emplace_back(pm);
                x += width/num_width_points;
            }
            y += height/num_height_points;
        }
    }

    for (int pj = 0; pj < pinned.size(); pj++) {
        int i = pinned[pj][0];
        int j = pinned[pj][1];
        int index = getIndex(i, j);
        point_masses[index].pinned = true;

    }

    // FIXME: Springs
    for (int j = 0; j < num_height_points; j++) {
        for (int i = 0; i < num_width_points; i++) {
            int index = getIndex(i, j);
            int leftIndex = getIndex(i-1, j);
            int aboveIndex = getIndex(i, j-1);
            int upperLeftIndex = getIndex(i-1, j-1);
            int upperRightIndex = getIndex(i+1, j-1);
            int twoLeftIndex = getIndex(i-2, j);
            int twoAboveIndex = getIndex(i, j-2);
            PointMass *curr = &point_masses[index];
            // NOTE: Structural Left
            if (leftIndex >= j*num_width_points) {
                PointMass *left = &point_masses[leftIndex];
                Spring springLeft(curr, left, STRUCTURAL);
                springs.push_back(springLeft);
            }
            // NOTE: Structural Above
            if (aboveIndex >= 0) {
                PointMass *above = &point_masses[aboveIndex];
                Spring springAbove(curr, above, STRUCTURAL);
                springs.push_back(springAbove);
            }
            // NOTE: Shearing Upper Left
            if (upperLeftIndex >= 0 && upperLeftIndex >= (j-1)*num_width_points && upperLeftIndex < j*num_width_points) {
                PointMass *upperLeft = &point_masses[upperLeftIndex];
                Spring springUpperLeft(curr, upperLeft, SHEARING);
                springs.push_back(springUpperLeft);
            }
            // NOTE: Shearing Upper Right
            if (upperRightIndex >= 0 && upperRightIndex < j*num_width_points) {
                PointMass *upperRight = &point_masses[upperRightIndex];
                Spring springUpperRight(curr, upperRight, SHEARING);
                springs.push_back(springUpperRight);
            }
            // NOTE: Bending Left
            if (twoLeftIndex >= j*num_width_points) {
                PointMass *left = &point_masses[twoLeftIndex];
                Spring springLeft(curr, left, BENDING);
                springs.push_back(springLeft);

            }
            // NOTE: Bending Above
            if (twoAboveIndex >= 0) {
                PointMass *above = &point_masses[twoAboveIndex];
                Spring springAbove(curr, above, BENDING);
                springs.push_back(springAbove);
            }
        }
    }
}

void Cloth::simulate(double frames_per_sec, double simulation_steps, ClothParameters *cp,
                     vector<Vector3D> external_accelerations,
                     vector<CollisionObject *> *collision_objects) {
    double mass = width * height * cp->density / num_width_points / num_height_points;
    double delta_t = 1.0f / frames_per_sec / simulation_steps;

    // TODO (Part 2): Compute total force acting on each point mass.
    Vector3D a;
    for (Vector3D e : external_accelerations) { a += e;}
    Vector3D ma = mass * a;
    for (int j = 0; j < num_height_points; j++) {
        for (int i = 0; i < num_width_points; i++) {
            int index = getIndex(i,j);
            PointMass *pm = &point_masses[index];
            pm->forces = Vector3D();
            pm->forces += ma;
        }
    }
    for (Spring s : springs) {
        e_spring_type t = s.spring_type;
        if ((t == STRUCTURAL && cp->enable_structural_constraints)
            || (t == SHEARING && cp->enable_shearing_constraints)
            || (t == BENDING && cp->enable_bending_constraints)) {
            double factor = cp->ks;
            if (t == BENDING) {
                factor *= 0.2f;
            }
            PointMass *a = s.pm_a;
            PointMass *b = s.pm_b;
            Vector3D vec = b->position-a->position;
            // FIXME: abs(fs)?
            double fs = factor * (vec.norm() - s.rest_length);
            Vector3D f = vec.unit() * fs;
            a->forces += f;
            b->forces -= f;
        }
    }

    // TODO (Part 2): Use Verlet integration to compute new point mass positions
    for (int j = 0; j < num_height_points; j++) {
        for (int i = 0; i < num_width_points; i++) {
            int index = getIndex(i,j);
            PointMass *pm = &point_masses[index];
            if (!pm->pinned) {
                Vector3D newpos = pm->position + (1.f - cp->damping / 100.f) * (pm->position - pm->last_position) + pm->forces / mass * pow(delta_t, 2.f);
                pm->last_position = pm->position;
                pm->position = newpos;
            }
            // NOTE: Optimizations?
//            for (CollisionObject * c : *collision_objects) {
//                c->collide(*pm);
//            }
        }
    }

    // TODO (Part 4): Handle self-collisions.
    build_spatial_map();
    for (int j = 0; j < num_height_points; j++) {
        for (int i = 0; i < num_width_points; i++) {
            int index = getIndex(i,j);
            PointMass *pm = &point_masses[index];
            self_collide(*pm, simulation_steps);
        }
    }

    // TODO (Part 3): Handle collisions with other primitives.
    for (int j = 0; j < num_height_points; j++) {
        for (int i = 0; i < num_width_points; i++) {
            int index = getIndex(i,j);
            PointMass *pm = &point_masses[index];
            for (CollisionObject * c : *collision_objects) {
                c->collide(*pm);
            }
        }
    }

    // TODO (Part 2): Constrain the changes to be such that the spring does not change
    // in length more than 10% per timestep [Provot 1995].
    for (Spring s : springs) {
        e_spring_type t = s.spring_type;
        if ((t == STRUCTURAL && cp->enable_structural_constraints)
            || (t == SHEARING && cp->enable_shearing_constraints)
            || (t == BENDING && cp->enable_bending_constraints)) {
            PointMass *a = s.pm_a;
            PointMass *b = s.pm_b;
            Vector3D vec = b->position-a->position;
            if (vec.norm() > 1.1f*s.rest_length) {
                double correct = vec.norm() - 1.1f*s.rest_length;
                Vector3D c = vec.unit() * correct;
                if (a->pinned) {
                    b->position -= c;
                } else if (b->pinned) {
                    a->position += c;
                } else {
                    a->position += c/2.f;
                    b->position -= c/2.f;
                }
            }
        }
    }
}

void Cloth::build_spatial_map() {
    for (const auto &entry : map) {
        delete(entry.second);
    }
    map.clear();

    // TODO (Part 4): Build a spatial map out of all of the point masses.
    for (int j = 0; j < num_height_points; j++) {
        for (int i = 0; i < num_width_points; i++) {
            int index = getIndex(i,j);
            PointMass *pm = &point_masses[index];
            float key = hash_position(pm->position);
            if (map.find(key) == map.end()) {
                map[key] = new vector<PointMass *>;
                map[key]->push_back(pm);
            } else {
                map[key]->push_back(pm);
            }
        }
    }

}

void Cloth::self_collide(PointMass &pm, double simulation_steps) {
    // TODO (Part 4): Handle self-collision for a given point mass.
    float key = hash_position(pm.position);
    int num_corrections = 0;
    Vector3D c;
    for (PointMass *other : *map[key]) {
        Vector3D pos = pm.position;
        if (other != &pm) {
            Vector3D vec = pm.position - other->position ;
            double dist = vec.norm();
            if (dist < 2.f * thickness) {
                double correct = 2.f*thickness - dist;
                c += vec.unit() * correct;
                num_corrections++;
            }
        }
    }
    if (num_corrections != 0) {
        pm.position += c / simulation_steps / num_corrections;
    }
}

float Cloth::hash_position(Vector3D pos) {
    // TODO (Part 4): Hash a 3D position into a unique float identifier that represents membership in some 3D box volume.
    double w, h, t;
    w = 3.f * width / num_width_points;
    h = 3.f * height / num_height_points;
    t = max(w, h);
    double divx, divy, divz;
    divx = floor(pos.x / w) * 73856093.f;
    divy = floor(pos.y / h) * 19349663.f;
    divz = floor(pos.z / t) * 83492791.f;
    return divx + divy + divz;
}

///////////////////////////////////////////////////////
/// YOU DO NOT NEED TO REFER TO ANY CODE BELOW THIS ///
///////////////////////////////////////////////////////

void Cloth::reset() {
    PointMass *pm = &point_masses[0];
    for (int i = 0; i < point_masses.size(); i++) {
        pm->position = pm->start_position;
        pm->last_position = pm->start_position;
        pm++;
    }
}

void Cloth::buildClothMesh() {
    if (point_masses.size() == 0) return;

    ClothMesh *clothMesh = new ClothMesh();
    vector<Triangle *> triangles;

    // Create vector of triangles
    for (int y = 0; y < num_height_points - 1; y++) {
        for (int x = 0; x < num_width_points - 1; x++) {
            PointMass *pm = &point_masses[y * num_width_points + x];
            // Get neighboring point masses:
            /*                      *
             * pm_A -------- pm_B   *
             *             /        *
             *  |         /   |     *
             *  |        /    |     *
             *  |       /     |     *
             *  |      /      |     *
             *  |     /       |     *
             *  |    /        |     *
             *      /               *
             * pm_C -------- pm_D   *
             *                      *
             */

            float u_min = x;
            u_min /= num_width_points - 1;
            float u_max = x + 1;
            u_max /= num_width_points - 1;
            float v_min = y;
            v_min /= num_height_points - 1;
            float v_max = y + 1;
            v_max /= num_height_points - 1;

            PointMass *pm_A = pm                       ;
            PointMass *pm_B = pm                    + 1;
            PointMass *pm_C = pm + num_width_points    ;
            PointMass *pm_D = pm + num_width_points + 1;

            Vector3D uv_A = Vector3D(u_min, v_min, 0);
            Vector3D uv_B = Vector3D(u_max, v_min, 0);
            Vector3D uv_C = Vector3D(u_min, v_max, 0);
            Vector3D uv_D = Vector3D(u_max, v_max, 0);


            // Both triangles defined by vertices in counter-clockwise orientation
            triangles.push_back(new Triangle(pm_A, pm_C, pm_B,
                                             uv_A, uv_C, uv_B));
            triangles.push_back(new Triangle(pm_B, pm_C, pm_D,
                                             uv_B, uv_C, uv_D));
        }
    }

    // For each triangle in row-order, create 3 edges and 3 internal halfedges
    for (int i = 0; i < triangles.size(); i++) {
        Triangle *t = triangles[i];

        // Allocate new halfedges on heap
        Halfedge *h1 = new Halfedge();
        Halfedge *h2 = new Halfedge();
        Halfedge *h3 = new Halfedge();

        // Allocate new edges on heap
        Edge *e1 = new Edge();
        Edge *e2 = new Edge();
        Edge *e3 = new Edge();

        // Assign a halfedge pointer to the triangle
        t->halfedge = h1;

        // Assign halfedge pointers to point masses
        t->pm1->halfedge = h1;
        t->pm2->halfedge = h2;
        t->pm3->halfedge = h3;

        // Update all halfedge pointers
        h1->edge = e1;
        h1->next = h2;
        h1->pm = t->pm1;
        h1->triangle = t;

        h2->edge = e2;
        h2->next = h3;
        h2->pm = t->pm2;
        h2->triangle = t;

        h3->edge = e3;
        h3->next = h1;
        h3->pm = t->pm3;
        h3->triangle = t;
    }

    // Go back through the cloth mesh and link triangles together using halfedge
    // twin pointers

    // Convenient variables for math
    int num_height_tris = (num_height_points - 1) * 2;
    int num_width_tris = (num_width_points - 1) * 2;

    bool topLeft = true;
    for (int i = 0; i < triangles.size(); i++) {
        Triangle *t = triangles[i];

        if (topLeft) {
            // Get left triangle, if it exists
            if (i % num_width_tris != 0) { // Not a left-most triangle
                Triangle *temp = triangles[i - 1];
                t->pm1->halfedge->twin = temp->pm3->halfedge;
            } else {
                t->pm1->halfedge->twin = nullptr;
            }

            // Get triangle above, if it exists
            if (i >= num_width_tris) { // Not a top-most triangle
                Triangle *temp = triangles[i - num_width_tris + 1];
                t->pm3->halfedge->twin = temp->pm2->halfedge;
            } else {
                t->pm3->halfedge->twin = nullptr;
            }

            // Get triangle to bottom right; guaranteed to exist
            Triangle *temp = triangles[i + 1];
            t->pm2->halfedge->twin = temp->pm1->halfedge;
        } else {
            // Get right triangle, if it exists
            if (i % num_width_tris != num_width_tris - 1) { // Not a right-most triangle
                Triangle *temp = triangles[i + 1];
                t->pm3->halfedge->twin = temp->pm1->halfedge;
            } else {
                t->pm3->halfedge->twin = nullptr;
            }

            // Get triangle below, if it exists
            if (i + num_width_tris - 1 < 1.0f * num_width_tris * num_height_tris / 2.0f) { // Not a bottom-most triangle
                Triangle *temp = triangles[i + num_width_tris - 1];
                t->pm2->halfedge->twin = temp->pm3->halfedge;
            } else {
                t->pm2->halfedge->twin = nullptr;
            }

            // Get triangle to top left; guaranteed to exist
            Triangle *temp = triangles[i - 1];
            t->pm1->halfedge->twin = temp->pm2->halfedge;
        }

        topLeft = !topLeft;
    }

    clothMesh->triangles = triangles;
    this->clothMesh = clothMesh;
}

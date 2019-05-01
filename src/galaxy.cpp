//
// Created by Khang Nguyen on 2019-04-30.
//

#include "galaxy.h"

//TODO: Make dynamically allocated?
Galaxy::Galaxy(vector<Sphere*> *planets) {
    this->planets = planets;
    num_planets = planets->size();
}

Galaxy::~Galaxy() {
    planets->clear();
    num_planets = 0;
}

void Galaxy::simulate(double frames_per_sec, double simulation_steps) {
    double delta_t = 1 / frames_per_sec / simulation_steps;
    Sphere *sphere, *other_planet;
    Vector3D gravity;
    for (int i = 0; i < num_planets; i++) {
        sphere = (*planets)[i];
        for (int j = i + 1; j < num_planets; j++) {
            other_planet = (*planets)[j];
            gravity = sphere->gravity(*other_planet);
            gravity = gravity*1E-5;
            sphere->add_force(gravity);
            other_planet->add_force(-gravity);
        }
    }
    for (auto planet : *planets) {
        //TODO: possibly add damping
        planet->verlet(delta_t);
    }
}
//
// Created by Khang Nguyen on 2019-04-30.
//

#include "galaxy.h"

//TODO: Make dynamically allocated?
Galaxy::Galaxy(vector<Sphere*> *planets) {
    this->planets = planets;
    this->lastPlanetDist = planets->back()->getInitOrigin().norm();
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
            gravity = gravity*1E-5; //TODO SCALED DOWN GRAVITY
            sphere->add_force(gravity);
            other_planet->add_force(-gravity);
        }
    }
    for (auto planet : *planets) {
        //TODO: possibly add damping
        planet->verlet(delta_t);
    }
}

void Galaxy::render(GLShader &shader) {
    
}

void Galaxy::add_planet(Sphere *s) {
    this->planets->push_back(s);
    this->lastPlanetDist = planets->back()->getInitOrigin().norm();
    num_planets = planets->size();
}

void Galaxy::add_planet() {
    // NOTE: Lat and Lon already specified in constructor
//    int sphere_num_lat = 40;
//    int sphere_num_lon = 40;
    std::cout << "Adding planet..\n";
    Vector3D origin, velocity;
    double radius, friction;
    long double mass;

    Sphere *last = planets->back();

    double multiplier = (rand()/RAND_MAX + 0.5f);

    origin = last->getInitOrigin() * multiplier;
    velocity = last->getInitVelocity() * multiplier;
    radius = last->getRadius() * multiplier;
    mass = last->getMass() * multiplier;

    Sphere *s = new Sphere(origin, radius, 1, velocity, mass);
    this->planets->push_back(s);
    this->lastPlanetDist = planets->back()->getInitOrigin().norm();
    num_planets = planets->size();
}

void Galaxy::reset() {
    for (Sphere* s : *planets) {
        s->reset();
    }
}
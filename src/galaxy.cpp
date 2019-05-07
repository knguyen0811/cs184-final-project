//
// Created by Khang Nguyen on 2019-04-30.
//

#include "galaxy.h"

//TODO: Make dynamically allocated?
Galaxy::Galaxy(vector<Sphere*> *planets) {
    this->planets = planets;
    sort(planets->begin(), planets->end(), compareOrigin);
    this->last = planets->back();
    num_planets = planets->size();
}

Galaxy::~Galaxy() {
    planets->clear();
    num_planets = 0;
}

void Galaxy::simulate(double frames_per_sec, double simulation_steps) {
    double delta_t = 1 / frames_per_sec / simulation_steps;

    // std::cout << "DELTA_T:" << delta_t << "\n";
    // std::cout << "Frames per sec:" << frames_per_sec << "\n";
    // std::cout << "Simulation Steps:" << simulation_steps << "\n";

    Sphere *sphere, *other_planet;
    Vector3D gravity;
    for (int i = 0; i < num_planets; i++) {
        sphere = (*planets)[i];
        for (int j = i + 1; j < num_planets; j++) {
            other_planet = (*planets)[j];
            gravity = sphere->gravity(*other_planet);
            gravity = gravity*1E-3*2; //TODO SCALED DOWN GRAVITY
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
    for (Sphere *s : *planets) {
        s->render(shader);
    }
}

void Galaxy::add_planet(Sphere *s) {
    add_planet_helper(s);
}

void Galaxy::add_planet() {
    // NOTE: Lat and Lon already specified in constructor
//    int sphere_num_lat = 40;
//    int sphere_num_lon = 40;
    std::cout << "Adding planet..\n";
    Vector3D origin, velocity;
    double radius, friction;
    long double mass;

    double multiplier = (rand()/RAND_MAX + 1.f);

    origin = last->getInitOrigin() * 1.5f;
    velocity = last->getInitVelocity() * multiplier;
    radius = last->getRadius() * multiplier;
    mass = last->getMass() * multiplier;

    Sphere *s = new Sphere(origin, radius, 1, velocity, mass);
    add_planet_helper(s);
}

void Galaxy::add_planet_helper(Sphere *s) {
    this->planets->push_back(s);
    sort(planets->begin(), planets->end(), compareOrigin);
    this->last = planets->back();
    num_planets = planets->size();
}

void Galaxy::remove_planet() {
    std::cout << "Removing planet..\n";
    planets->pop_back();
    this->last = planets->back();
    num_planets = planets->size();

    // Deallocate Sphere object TODO: NVM ACTUALLY BREAKS SIMULATION
//    delete last;
}

void Galaxy::remove_planet(int index) {
    std::cout << "Removing planet at index..\n";
    planets->erase(planets->begin()+index);
    this->last = planets->back();
    num_planets = planets->size();
}

int Galaxy::size() {
    return num_planets;
}

Sphere* Galaxy::getLastPlanet() {
    return last;
}

void Galaxy::reset() {
    for (Sphere* s : *planets) {
        s->reset();
    }
}
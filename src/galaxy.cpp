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

Galaxy::Galaxy(vector<Sphere *> *planets, vector<Sphere *> *asteroids) {
    this->planets = planets;
    sort(planets->begin(), planets->end(), compareOrigin);
    this->last = planets->back();
    num_planets = planets->size();

    this->asteroids = asteroids;
    num_asteroids = asteroids->size();
}

Galaxy::~Galaxy() {
    planets->clear();
    num_planets = 0;
}

void Galaxy::setTextures(map<string, GLuint*> &tex_file_to_texture) {
  for (auto sphere : *planets) {
    if (tex_file_to_texture.count(sphere->getTexFile())) {
      sphere->texture = tex_file_to_texture[sphere->getTexFile()];
    } else {
      // Assign an arbitrary texture
      sphere->texture = tex_file_to_texture.begin()->second;
    }
  }
  for (auto asteroid : *asteroids) {
    if (tex_file_to_texture.count(asteroid->getTexFile())) {
      asteroid->texture = tex_file_to_texture[asteroid->getTexFile()];
    } else {
      // Assign an arbitrary texture
      asteroid->texture = tex_file_to_texture.begin()->second;
    }
  }
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
            //if (abs(sphere->getMass() - other_planet->getMass()) >= Sphere::gravity_margin) {
                gravity = sphere->gravity(*other_planet);
                sphere->add_force(gravity);
                other_planet->add_force(-gravity);
            //}
        }
    }
    for (auto planet : *planets) {
        planet->verlet(delta_t);
    }

    // Add Asteroid stuff here
    // DEBUG: Placeholder Code for Asteroids, only considers sun's gravitational force
    if (asteroids != nullptr) {
        Sphere *center = (*planets)[0];
        for (Sphere *a : *asteroids) {
            gravity = center->gravity(*a);
            center->add_force(gravity);
            a->add_force(-gravity);
        }

        for (Sphere *a : *asteroids) {
            a->verlet(delta_t);
        }
    }
}

void Galaxy::render(GLShader &shader, bool is_paused) {
    for (Sphere *s : *planets) {
        s->render(shader, is_paused);
    }
    if (asteroids != nullptr) {
        for (Sphere *a : *asteroids) {
            // Set is_paused to true to ignore adding position for tracking
            // Set draw_track to false because we are not draw trail for asteroid belt
            a->render(shader, true);
        }
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
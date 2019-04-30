//
// Created by Milo Piazza on 2019-04-30.
//

#include "Galaxy.h"
#include <vector>

//TODO: Make dynamically allocated?
Galaxy::Galaxy(vector<Planet*>planets) {
  this->planets = planets;
  num_planets = planets.size();
}

void Galaxy::simulate(double frames_per_sec, double simulation_steps) {
  double delta_t = 1 / frames_per_sec / simulation_steps;
  Planet *planet, *other_planet;
  Vector3D gravity, new_pos;
  for (int i = 0; i < num_planets; i++) {
    planet = planets[i];
    for (int j = i + 1; j < num_planets; j++) {
      other_planet = planets[j];
      gravity = planet->gravity(*other_planet);
      planet->add_force(gravity);
      other_planet->add_force(-gravity);
    }
  }
  for (auto planet : planets) {
    //TODO: possibly add damping
    planet->verlet(delta_t);
  }
}
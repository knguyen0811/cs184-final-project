//
// Created by Milo Piazza on 2019-04-30.
//

#ifndef CLOTHSIM_GALAXY_H
#define CLOTHSIM_GALAXY_H

#include <vector>
#include "planet.h"
class Galaxy {
    std::vector<Planet*> planets;
    int num_planets;
    Galaxy(vector<Planet*>planets);
    ~Galaxy();
    void simulate(double frames_per_sec, double simulation_steps);
};


#endif //CLOTHSIM_GALAXY_H

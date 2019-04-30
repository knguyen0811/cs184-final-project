//
// Created by Milo Piazza on 2019-04-30.
//

#ifndef CLOTHSIM_GALAXY_H
#define CLOTHSIM_GALAXY_H

#include <vector>
#include "collision/sphere.h"
class Galaxy {
    std::vector<Sphere*> planets;
    int num_planets;
    Galaxy(vector<Sphere*>planets);
    ~Galaxy();
    void simulate(double frames_per_sec, double simulation_steps);
};


#endif //CLOTHSIM_GALAXY_H

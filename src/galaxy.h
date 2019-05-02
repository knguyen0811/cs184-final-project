//
// Created by Khang Nguyen on 2019-04-30.
//

#ifndef CLOTHSIM_GALAXY_H
#define CLOTHSIM_GALAXY_H

#include <vector>
#include "collision/sphere.h"
class Galaxy {
public:
    // Constructor & Destructor
    Galaxy(vector<Sphere *> *planets);
    ~Galaxy();

    // Functions
    void simulate(double frames_per_sec, double simulation_steps);
    void reset();
    void add_planet(Sphere *s);
    void add_planet();
    void add_planet_helper(Sphere *s);
    void remove_planet();
    void render(GLShader &shader);


    // Comparators
    static bool compareRadius(Sphere *s1, Sphere *s2) {
        return s1->getRadius() < s2->getRadius();
    }

    static bool compareOrigin(Sphere *s1, Sphere *s2) {
        return s1->getInitOrigin().norm() < s2->getInitOrigin().norm();
    }

    // Variables
    int num_planets;
    long double lastPlanetDist;
    std::vector<Sphere*> *planets;
};


#endif //CLOTHSIM_GALAXY_H
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
    Galaxy() {};
    Galaxy(vector<Sphere *> *planets);
    Galaxy(vector<Sphere *> *planets, vector<Sphere *> *asteroids);
    ~Galaxy();

    // Functions
    void simulate(double frames_per_sec, double simulation_steps);
    void reset();
    void add_planet(Sphere *s);
    void add_planet();
    void add_planet_helper(Sphere *s);
    void remove_planet();
    void remove_planet(int index);
    void setTextures(map<string, GLuint*> &tex_file_to_texture);
    int size();
    Sphere* getLastPlanet();
    void render(GLShader &shader, bool is_paused);


    // Comparators
    static bool compareRadius(Sphere *s1, Sphere *s2) {
        return s1->getRadius() < s2->getRadius();
    }

    static bool compareOrigin(Sphere *s1, Sphere *s2) {
        return s1->getInitOrigin().norm() < s2->getInitOrigin().norm();
    }

    static bool compareVelocity(Sphere *s1, Sphere *s2) {
        return s1->getInitVelocity().norm() < s2->getInitVelocity().norm();
    }

    static bool compareMass(Sphere *s1, Sphere *s2) {
        return s1->getMass() < s2->getMass();
    }

    // Variables
    int num_planets;
    int num_asteroids;
    Sphere *last;
    std::vector<Sphere*> *planets;
    std::vector<Sphere*> *asteroids;
};


#endif //CLOTHSIM_GALAXY_H
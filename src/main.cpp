#include <iostream>
#include <fstream>
#include <nanogui/nanogui.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include "misc/getopt.h" // getopt for windows
#else
#include <getopt.h>
#include <unistd.h>
#endif
#include <unordered_set>
#include <stdlib.h> // atoi for getopt inputs
#include <random>

#include "CGL/CGL.h"
#include "collision/plane.h"
#include "collision/sphere.h"
#include "galaxySimulator.h"
#include "json.hpp"
#include "misc/file_utils.h"
#include "galaxy.h"

typedef uint32_t gid_t;

using namespace std;
using namespace nanogui;

using json = nlohmann::json;

#define msg(s) cerr << "[ClothSim] " << s << endl;

const string SPHERE = "sphere";
const string SPHERES = "spheres";
const string PLANE = "plane";
const string CLOTH = "cloth";
const string GENERATE = "generate";

const unordered_set<string> VALID_KEYS = {SPHERE, PLANE, CLOTH, SPHERES, GENERATE};

GalaxySimulator *app = nullptr;
GLFWwindow *window = nullptr;
Screen *screen = nullptr;

void error_callback(int error, const char* description) {
  puts(description);
}

void createGLContexts() {
  if (!glfwInit()) {
    return;
  }

  glfwSetTime(0);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_SAMPLES, 0);
  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_ALPHA_BITS, 8);
  glfwWindowHint(GLFW_STENCIL_BITS, 8);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  // Create a GLFWwindow object
  window = glfwCreateWindow(800, 800, "Cloth Simulator", nullptr, nullptr);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    throw std::runtime_error("Could not initialize GLAD!");
  }
  glGetError(); // pull and ignore unhandled errors like GL_INVALID_ENUM

  glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Create a nanogui screen and pass the glfw pointer to initialize
  screen = new Screen();
  screen->initialize(window, true);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);
  glfwSwapInterval(1);
  glfwSwapBuffers(window);
}

void setGLFWCallbacks() {
  glfwSetCursorPosCallback(window, [](GLFWwindow *, double x, double y) {
    if (!screen->cursorPosCallbackEvent(x, y)) {
      app->cursorPosCallbackEvent(x / screen->pixelRatio(),
                                  y / screen->pixelRatio());
    }
  });

  glfwSetMouseButtonCallback(
      window, [](GLFWwindow *, int button, int action, int modifiers) {
        if (!screen->mouseButtonCallbackEvent(button, action, modifiers) ||
            action == GLFW_RELEASE) {
          app->mouseButtonCallbackEvent(button, action, modifiers);
        }
      });

  glfwSetKeyCallback(
      window, [](GLFWwindow *, int key, int scancode, int action, int mods) {
        if (!screen->keyCallbackEvent(key, scancode, action, mods)) {
          app->keyCallbackEvent(key, scancode, action, mods);
        }
      });

  glfwSetCharCallback(window, [](GLFWwindow *, unsigned int codepoint) {
    screen->charCallbackEvent(codepoint);
  });

  glfwSetDropCallback(window,
                      [](GLFWwindow *, int count, const char **filenames) {
                        screen->dropCallbackEvent(count, filenames);
                        app->dropCallbackEvent(count, filenames);
                      });

  glfwSetScrollCallback(window, [](GLFWwindow *, double x, double y) {
    if (!screen->scrollCallbackEvent(x, y)) {
      app->scrollCallbackEvent(x, y);
    }
  });

  glfwSetFramebufferSizeCallback(window,
                                 [](GLFWwindow *, int width, int height) {
                                   screen->resizeCallbackEvent(width, height);
                                   app->resizeCallbackEvent(width, height);
                                 });
}

void usageError(const char *binaryName) {
  printf("Usage: %s [options]\n", binaryName);
  printf("Required program options:\n");
  printf("  -f     <STRING>    Filename of scene\n");
  printf("  -r     <STRING>    Project root.\n");
  printf("                     Should contain \"shaders/Default.vert\".\n");
  printf("                     Automatically searched for by default.\n");
  printf("  -a     <INT>       Sphere vertices latitude direction.\n");
  printf("  -o     <INT>       Sphere vertices longitude direction.\n");
  printf("\n");
  exit(-1);
}

void incompleteObjectError(const char *object, const char *attribute) {
  cout << "Incomplete " << object << " definition, missing " << attribute << endl;
  exit(-1);
}

long double randomVal(long double min, long double max) {
    // Return random value between min and max
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<long double> dist(min, max);
    long double val = dist(gen);
    return val;
}

 double randomAngle() {
    // Return random angle between 0 and 2PI
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(0, 2*PI);
    long double val = dist(gen);
    return val;
}

Vector3D randomVec(Vector3D min, Vector3D max) {
    // Return random vector between min and max
    Vector3D dir = max - min;
    double dirNorm = dir.norm();
    dir.normalize();
    double factor = randomVal(0, dirNorm);
    return min + factor * dir;
}

Vector3D randomVec(double norm) {
    // Return random vector between min and max
    double angle = randomAngle();
    Vector3D newVec(cos(angle)*norm, sin(angle)*norm, 0);
    return newVec;
}

void generateObjectsFromFile(vector<Sphere *>* planets, vector<Sphere *>* asteroids, int num_spheres, int num_asteroids=0) {
    Vector3D sphereOrigMin, sphereOrigMax, sphereVelMin, sphereVelMax;
//    double sphereRadiusMin, sphereRadiusMax, friction=0.3f;
    long double sphereMassMin, sphereMassMax;
    double sphereRadiusMin=20, sphereRadiusMax=30, friction=0.3f;

    Vector3D origin, velocity;
    double radius;
    long double mass;

    if (planets->empty()) {
        for (int i = 0; i < num_spheres; i++) {
            if (planets->empty()) {
                // Generate the star of the solar system
                origin = Vector3D(0,0,0);
                velocity = Vector3D(0,0,0);
                radius = randomVal(7, 15);
                mass = randomVal(1E30, 5E30);

                Sphere *new_sphere = new Sphere(origin, radius, friction, velocity, mass);
                planets->push_back(new_sphere);
            } else {
                // Generate rest of the planets
                Vector3D origSeed(5.79E10,0,0);
                Vector3D vecSeed(0,4740,0);
                velocity = randomVec(5 * vecSeed, 10 * vecSeed); // Make velocity large
//                velocity = 10 * vecSeed;
                mass = randomVal(1E23, 6E24);
                radius = randomVal(1, 6);

                if (planets->size() == 1) {
                    // Create first planet
                    origin = randomVec(origSeed, 2 * origSeed);

                    Sphere *new_sphere = new Sphere(origin, radius, friction, velocity, mass);
                    planets->push_back(new_sphere);
                } else {
                    Sphere* last = *std::max_element(planets->begin()+1, planets->end(), Galaxy::compareOrigin);
                    Vector3D farthestOrig = last->getInitOrigin();
                    sphereOrigMin = farthestOrig * 1.5f;
                    sphereOrigMax = farthestOrig * 2.f;

                    origin = randomVec(sphereOrigMin, sphereOrigMax);

                    Sphere *new_sphere = new Sphere(origin, radius, friction, velocity, mass);
                    planets->push_back(new_sphere);
                }
            }
        }
    } else {
        sort(planets->begin(), planets->end(), Galaxy::compareOrigin);

        for (int i = 0; i < num_spheres; i++) {
            Sphere* last = *std::max_element(planets->begin()+1, planets->end(), Galaxy::compareOrigin);
            Vector3D farthestOrig = last->getInitOrigin();
            sphereOrigMin = farthestOrig * 1.5f;
            sphereOrigMax = farthestOrig * 2.f;

            sphereVelMin = (*std::min_element(planets->begin()+1, planets->end(), Galaxy::compareVelocity))->getInitVelocity();
            sphereVelMax = (*std::max_element(planets->begin()+1, planets->end(), Galaxy::compareVelocity))->getInitVelocity();

            sphereRadiusMin = (*std::min_element(planets->begin()+1, planets->end(), Galaxy::compareRadius))->getRadius();
            sphereRadiusMax = (*std::max_element(planets->begin()+1, planets->end(), Galaxy::compareRadius))->getRadius();

            sphereMassMin = (*std::min_element(planets->begin()+1, planets->end(), Galaxy::compareMass))->getMass();
            sphereMassMax = (*std::max_element(planets->begin()+1, planets->end(), Galaxy::compareMass))->getMass();

            origin = randomVec(sphereOrigMin, sphereOrigMax);
            velocity = randomVec(sphereVelMin, sphereVelMax);
            radius = randomVal(sphereRadiusMin, sphereRadiusMax);
            mass = randomVal(sphereMassMin, sphereMassMax);

            Sphere *new_sphere = new Sphere(origin, radius, friction, velocity, mass);
            planets->push_back(new_sphere);
        }
    }
    if (num_asteroids) {
        double angle = randomAngle();
        Vector3D astVelMin(cos(angle)*17900, sin(angle)*17900, 0);
        Vector3D astVelMax(cos(angle)*30000, sin(angle)*30000, 0);
        double astDist, astRadiusMin=2, astRadiusMax=4;
        long double astMassMin=2.8E21, astMassMax=3.2E21;

        Sphere* last = *std::max_element(planets->begin()+1, planets->end(), Galaxy::compareOrigin);
        double lastDist = 1.f * last->getInitOrigin().norm();

        for (int j = 0; j < num_asteroids; j++) {
            astDist = randomVal(lastDist, 1.1f * lastDist);

            origin = randomVec(astDist);
            velocity = randomVec(astVelMin, astVelMax);
            radius = randomVal(astRadiusMin, astRadiusMax);
            mass = randomVal(astMassMin, astMassMax);

            Sphere *new_sphere = new Sphere(origin, radius, friction, velocity, mass);
            asteroids->push_back(new_sphere);
        }
    }
}

bool loadObjectsFromFile(string filename, vector<Sphere *>* planets, int* num_spheres, int* num_asteroids, int sphere_num_lat, int sphere_num_lon) {
  // Read JSON from file
  ifstream i(filename);
  if (!i.good()) {
    return false;
  }
  json j;
  i >> j;

  std::vector<double> coordinateVals = std::vector<double>();
  // Loop over objects in scene
  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    string key = it.key();

    // Check that object is valid
    unordered_set<string>::const_iterator query = VALID_KEYS.find(key);
    if (query == VALID_KEYS.end()) {
      cout << "Invalid scene object found: " << key << endl;
      exit(-1);
    }

    // Retrieve object
    json object = it.value();

    // Parse object depending on type (cloth, sphere, or plane)
    if (key == SPHERES) {
      // the object under key "spheres" will be an array of bodies.
      Vector3D origin, velocity;
      double radius, friction;
      long double mass;
      for (auto& sphere_element : object) {
        auto it_origin = sphere_element.find("origin");
        if (it_origin != sphere_element.end()) {
          vector<double> vec_origin = *it_origin;
          origin = Vector3D(vec_origin[0], vec_origin[1], vec_origin[2]);
          coordinateVals.push_back(vec_origin[0]);
          coordinateVals.push_back(vec_origin[1]);
          coordinateVals.push_back(vec_origin[2]);
        } else {
          incompleteObjectError("sphere", "origin");
        }

        auto it_radius = sphere_element.find("radius");
        if (it_radius != sphere_element.end()) {
          radius = *it_radius;
        } else {
          incompleteObjectError("sphere", "radius");
        }

        auto it_friction = sphere_element.find("friction");
        if (it_friction != sphere_element.end()) {
          friction = *it_friction;
        } else {
          incompleteObjectError("sphere", "friction");
        }

        auto it_velocity = sphere_element.find("velocity");
        if (it_velocity != sphere_element.end()) {
          vector<double> vec_velocity = *it_velocity;
          velocity = Vector3D(vec_velocity[0], vec_velocity[1], vec_velocity[2]);
        } else {
          incompleteObjectError("sphere", "velocity");
        }

        std::cout << "velocity: " << velocity << "\n";

        auto it_mass = sphere_element.find("mass");
        if (it_mass != sphere_element.end()) {
          mass = *it_mass;
        } else {
          incompleteObjectError("sphere", "mass");
        }
        
        Sphere *new_sphere = new Sphere(origin, radius, friction, velocity, mass, sphere_num_lat, sphere_num_lon);
        planets->push_back(new_sphere);
      }
    }
    if (key == GENERATE) {
        auto it_spheres = object.find("spheres");
        if (it_spheres != object.end()) {
            *num_spheres = *it_spheres;
        } else {
            cout << "num_spheres not specified" << endl;
        }

        auto it_asteroids = object.find("asteroids");
        if (it_asteroids != object.end()) {
            *num_asteroids = *it_asteroids;
        } else {
            cout << "num_asteroids not specified" << endl;
        }
    }
  }

  i.close();

  coordinateVals.erase(std::remove(coordinateVals.begin(), coordinateVals.end(), 0), coordinateVals.end());
  sort(coordinateVals.begin(), coordinateVals.end());
  double val = coordinateVals.front();
  int count = 0;
  while (val >= 10) {
    val /= 10;
    count++;
  }
  Sphere::sphere_factor = pow(10, count);
  
  return true;
}

bool is_valid_project_root(const std::string& search_path) {
    std::stringstream ss;
    ss << search_path;
    ss << "/";
    ss << "shaders/Default.vert";
    
    return FileUtils::file_exists(ss.str());
}

// Attempt to locate the project root automatically
bool find_project_root(const std::vector<std::string>& search_paths, std::string& retval) {
  
  for (std::string search_path : search_paths) {
    if (is_valid_project_root(search_path)) {
      retval = search_path;
      return true;
    }
  }
  return false;
}

int main(int argc, char **argv) {
  // Attempt to find project root
  std::vector<std::string> search_paths = {
    ".",
    "..",
    "../.."
  };
  std::string project_root;
  bool found_project_root = find_project_root(search_paths, project_root);
  
  SphereParameters sp;
  vector<Sphere *> planets;
  vector<Sphere *> asteroids;
  int num_spheres = 0;
  int num_asteroids = 0;

  int c;
  
  int sphere_num_lat = 40;
  int sphere_num_lon = 40;
  
  std::string file_to_load_from;
  bool file_specified = false;
  
  while ((c = getopt (argc, argv, "f:r:a:o:")) != -1) {
    switch (c) {
      case 'f': {
        file_to_load_from = optarg;
        file_specified = true;
        break;
      }
      case 'r': {
        project_root = optarg;
        if (!is_valid_project_root(project_root)) {
          std::cout << "Warn: Could not find required file \"shaders/Default.vert\" in specified project root: " << project_root << std::endl;
        }
        found_project_root = true;
        break;
      }
      case 'a': {
        int arg_int = atoi(optarg);
        if (arg_int < 1) {
          arg_int = 1;
        }
        sphere_num_lat = arg_int;
        break;
      }
      case 'o': {
        int arg_int = atoi(optarg);
        if (arg_int < 1) {
          arg_int = 1;
        }
        sphere_num_lon = arg_int;
        break;
      }
      default: {
        usageError(argv[0]);
        break;
      }
    }
  }
  
  if (!found_project_root) {
    std::cout << "Error: Could not find required file \"shaders/Default.vert\" anywhere!" << std::endl;
    return -1;
  } else {
    std::cout << "Loading files starting from: " << project_root << std::endl;
  }

  if (!file_specified) { // No arguments, default initialization
    std::stringstream def_fname;
    def_fname << project_root;
    def_fname << "/scene/rocky_planets_gen.json";
    file_to_load_from = def_fname.str();
  }
  
  bool success = loadObjectsFromFile(file_to_load_from, &planets, &num_spheres, &num_asteroids, sphere_num_lat, sphere_num_lon);
  if (!success) {
    std::cout << "Warn: Unable to load from file: " << file_to_load_from << std::endl;
  }

  glfwSetErrorCallback(error_callback);

  createGLContexts();

  // Initialize the GalaxySimulator object
  if (num_spheres != 0 || num_asteroids != 0) {
      generateObjectsFromFile(&planets, &asteroids, num_spheres, num_asteroids);
  }
  Galaxy galaxy(&planets, &asteroids);
  app = new GalaxySimulator(project_root, screen);
  app->loadSphereParameters(&sp);
  app->loadGalaxy(&galaxy);
  app->init();

  // Call this after all the widgets have been defined

  screen->setVisible(true);
  screen->performLayout();

  // Attach callbacks to the GLFW window

  setGLFWCallbacks();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    app->drawContents();

    // Draw nanogui
    screen->drawContents();
    screen->drawWidgets();

    glfwSwapBuffers(window);

    if (!app->isAlive()) {
      glfwSetWindowShouldClose(window, 1);
    }
  }

  return 0;
}

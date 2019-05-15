#include <cmath>
#include <glad/glad.h>

#include <CGL/vector3D.h>
#include <nanogui/nanogui.h>

#include "galaxySimulator.h"
#include "leak_fix.h"

#include "camera.h"
#include "collision/plane.h"
#include "collision/sphere.h"
#include "misc/camera_info.h"
#include "misc/file_utils.h"
// Needed to generate stb_image binaries. Should only define in exactly one source file importing stb_image.h.
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

using namespace nanogui;
using namespace std;

Vector3D load_texture(int frame_idx, GLuint handle, const char* where) {
  Vector3D size_retval;
  
  if (strlen(where) == 0) return size_retval;
  
  glActiveTexture(GL_TEXTURE0 + frame_idx);
  glBindTexture(GL_TEXTURE_2D, handle);
  
  
  int img_x, img_y, img_n;
  unsigned char* img_data = stbi_load(where, &img_x, &img_y, &img_n, 3);
  size_retval.x = img_x;
  size_retval.y = img_y;
  size_retval.z = img_n;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_x, img_y, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);
  stbi_image_free(img_data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  
  return size_retval;
}

void load_cubemap(int frame_idx, GLuint handle, const std::vector<std::string>& file_locs) {
  glActiveTexture(GL_TEXTURE0 + frame_idx);
  glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
  for (int side_idx = 0; side_idx < 6; ++side_idx) {
    
    int img_x, img_y, img_n;
    unsigned char* img_data = stbi_load(file_locs[side_idx].c_str(), &img_x, &img_y, &img_n, 3);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side_idx, 0, GL_RGB, img_x, img_y, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);
    stbi_image_free(img_data);
    std::cout << "Side " << side_idx << " has dimensions " << img_x << ", " << img_y << std::endl;

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }
}

void GalaxySimulator::load_textures() {
  //TODO: replace with map to texture id
  vector<string> texture_paths;
  set<string> files;
  const string texture_directory = m_project_root + "/textures";
  bool success = FileUtils::list_files_in_directory(texture_directory, files);
  if (!success) {
    std::cout << "Error: could not find the texture folder!" << endl;
  }
  std::string file_extension;
  std::string texture_name;
  for (auto file : files) {
    FileUtils::split_filename(file, texture_name, file_extension);
    if (file_extension == "png") {
      texture_paths.emplace_back(texture_directory + "/" + file);
    }
  }
  gl_textures = new GLuint[texture_paths.size()];
  gl_texture_sizes = new Vector3D[texture_paths.size()];
  //TODO: create map from textures to GLuint*
  glGenTextures(texture_paths.size(), gl_textures);
  for (int i = 0; i < texture_paths.size(); i++) {
    gl_texture_sizes[i] = load_texture(0, gl_textures[i], (texture_paths[i]).c_str());
    FileUtils::get_filename_from_path(texture_paths[i], texture_name);
    std::cout << texture_name << endl;
    tex_file_to_texture[texture_name] = gl_textures+i;
  }
  
  std::vector<std::string> cubemap_fnames = {
    m_project_root + "/textures/space/posx.jpg",
    m_project_root + "/textures/space/negx.jpg",
    m_project_root + "/textures/space/posy.jpg",
    m_project_root + "/textures/space/negy.jpg",
    m_project_root + "/textures/space/posz.jpg",
    m_project_root + "/textures/space/negz.jpg"
  };
  
  load_cubemap(1, m_gl_cubemap_tex, cubemap_fnames);
  std::cout << "Loaded cubemap texture" << std::endl;
}

void GalaxySimulator::load_shaders() {
  std::set<std::string> shader_folder_contents;
  bool success = FileUtils::list_files_in_directory(m_project_root + "/shaders", shader_folder_contents);
  if (!success) {
    std::cout << "Error: Could not find the shaders folder!" << std::endl;
  }
  
  std::string std_vert_shader = m_project_root + "/shaders/Default.vert";
  
  for (const std::string& shader_fname : shader_folder_contents) {
    std::string file_extension;
    std::string shader_name;
    
    FileUtils::split_filename(shader_fname, shader_name, file_extension);
    
    if (file_extension != "frag") {
      std::cout << "Skipping non-shader file: " << shader_fname << std::endl;
      continue;
    }
    
    std::cout << "Found shader file: " << shader_fname << std::endl;
    
    // Check if there is a proper .vert shader or not for it
    std::string vert_shader = std_vert_shader;
    std::string associated_vert_shader_path = m_project_root + "/shaders/" + shader_name + ".vert";
    if (FileUtils::file_exists(associated_vert_shader_path)) {
      vert_shader = associated_vert_shader_path;
    }
    
    GLShader nanogui_shader;
    nanogui_shader.initFromFiles(shader_name, vert_shader,
                                  m_project_root + "/shaders/" + shader_fname);
    
    // Special filenames are treated a bit differently
    ShaderTypeHint hint;
    if (shader_name == "Wireframe") {
      hint = ShaderTypeHint::WIREFRAME;
      std::cout << "Type: Wireframe" << std::endl;
    } else if (shader_name == "Normal") {
      hint = ShaderTypeHint::NORMALS;
      std::cout << "Type: Normal" << std::endl;
    } else {
      hint = ShaderTypeHint::PHONG;
      std::cout << "Type: Custom" << std::endl;
    }
    
    UserShader user_shader(shader_name, nanogui_shader, hint);
    
    shaders.push_back(user_shader);
    shaders_combobox_names.push_back(shader_name);
  }
  
  // TODO: Assuming that it's there, use "Normal" by default
  for (size_t i = 0; i < shaders_combobox_names.size(); ++i) {
    if (shaders_combobox_names[i] == "Normal") {
      active_shader_idx = i;
      break;
    }
  }
}

void GalaxySimulator::setSphereTextures() {
  galaxy->setTextures(tex_file_to_texture);
}

GalaxySimulator::GalaxySimulator(std::string project_root, Screen *screen)
: m_project_root(project_root) {
  this->screen = screen;
  
  this->load_shaders();
  this->load_textures();

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_DEPTH_TEST);
}

//TODO: fix destructor
GalaxySimulator::~GalaxySimulator() {
  for (auto shader : shaders) {
    shader.nanogui_shader.free();
  }
  glDeleteTextures(1, &m_gl_texture_1);
  glDeleteTextures(1, &m_gl_texture_2);
  glDeleteTextures(1, &m_gl_texture_3);
  glDeleteTextures(1, &m_gl_texture_4);
  glDeleteTextures(1, &m_gl_texture_5);
  glDeleteTextures(1, &m_gl_texture_6);
  glDeleteTextures(1, &m_gl_cubemap_tex);

  if (sp) delete sp;
}


void GalaxySimulator::loadSphereParameters(SphereParameters *sp) { this->sp = sp; }

void GalaxySimulator::loadGalaxy(Galaxy *galaxy) {
  this->galaxy = galaxy;
  this->setSphereTextures();
}

/**
 * Initializes the cloth simulation and spawns a new thread to separate
 * rendering from simulation.
 */
void GalaxySimulator::init() {

  // Initialize GUI
  screen->setSize(default_window_size);
  initGUI(screen);

  // Initialize camera

  CGL::Collada::CameraInfo camera_info;
  camera_info.hFov = 50000;
  camera_info.vFov = 35000;
  camera_info.nClip = 0.01;
  camera_info.fClip = 10000;

  // Try to intelligently figure out the camera target

  Vector3D avg_pm_position(0, 0, 0);

//  for (auto &pm : cloth->point_masses) {
//    avg_pm_position += pm.position / cloth->point_masses.size();
//  }

  CGL::Vector3D target(avg_pm_position.x, avg_pm_position.y / 2,
                       avg_pm_position.z);
  CGL::Vector3D c_dir(0., 0., 0.);
//  canonical_view_distance = max(cloth->width, cloth->height) * 0.9;
//  std::cout << "dist max: " << canonical_view_distance << "\n";
    // DEBUG: Adjust Camera View Distance Here
  canonical_view_distance = abs(galaxy->getLastPlanet()->getInitOrigin().norm()) / Sphere::sphere_factor;
//  std::cout << "dist lastPlanet: " << canonical_view_distance << "\n";
  scroll_rate = canonical_view_distance / 10;

  view_distance = canonical_view_distance;
  min_view_distance = canonical_view_distance / 100.0;
  max_view_distance = canonical_view_distance * 1000.0;

  // canonicalCamera is a copy used for view resets

  camera.place(target, acos(c_dir.y), atan2(c_dir.x, c_dir.z), view_distance,
               min_view_distance, max_view_distance);
  canonicalCamera.place(target, acos(c_dir.y), atan2(c_dir.x, c_dir.z),
                        view_distance, min_view_distance, max_view_distance);

  screen_w = default_window_size(0);
  screen_h = default_window_size(1);

  camera.configure(camera_info, screen_w, screen_h);
  canonicalCamera.configure(camera_info, screen_w, screen_h);
}

bool GalaxySimulator::isAlive() { return is_alive; }

void GalaxySimulator::drawContents() {
  glEnable(GL_DEPTH_TEST);

  if (!is_paused) {
    vector<Vector3D> external_accelerations = {gravity};

    for (int i = 0; i < simulation_steps; i++) {
//      cloth->simulate(frames_per_sec, simulation_steps, sp, external_accelerations, collision_objects);
        // FIXME:
        galaxy->simulate(frames_per_sec, simulation_steps);
    }
  }

  // Prepare the camera projection matrix
  Matrix4f model;
  model.setIdentity();

  Matrix4f view = getViewMatrix();
  Matrix4f projection = getProjectionMatrix();

  Matrix4f viewProjection = projection * view;
  Vector3D cam_pos = camera.position();

  glActiveTexture(GL_TEXTURE0);

  // Draw Trail with Shader2 so it can change colors
    for (size_t i = 0; i < shaders_combobox_names.size(); ++i) {
        if (shaders_combobox_names[i] == "Wireframe") {
            active_shader_idx = i;
            break;
        }
    }


    const UserShader& active_shader = shaders[active_shader_idx];
    GLShader shader2 = active_shader.nanogui_shader;
  shader2.bind();
  shader2.setUniform("u_model", model);
  shader2.setUniform("u_view_projection", viewProjection);
  shader2.setUniform("u_color", color, false);
  if (draw_track) { drawTrail(shader2); }

  // Draw Textures with Shader
    GLShader shader = GLShader();
    shader.initFromFiles("Texture", m_project_root + "/shaders/Default.vert", m_project_root + "/shaders/Texture.frag");
    shader.bind();
  shader.setUniform("u_model", model);
  shader.setUniform("u_view_projection", viewProjection);
  shader.setUniform("u_color", color, false);


  shader.setUniform("u_cam_pos", Vector3f(cam_pos.x, cam_pos.y, cam_pos.z), false);
  shader.setUniform("u_light_pos", Vector3f(0.5, 2, 2), false);
  shader.setUniform("u_light_intensity", Vector3f(3, 3, 3), false);
  shader.setUniform("u_texture", 0, false);
  shader.setUniform("u_normal_scaling", m_normal_scaling, false);
  shader.setUniform("u_height_scaling", m_height_scaling, false);

  shader.setUniform("u_texture_cubemap", 1, false);
  galaxy->render(shader, is_paused);
  //drawPhong(shader);
}

void GalaxySimulator::drawTrail(GLShader &shader) {
    std::vector<Sphere*> *planets = galaxy->planets;
    Sphere *center = (*planets)[0];
    for (Sphere *s : (*planets)) {
        if (center != s) {
            center->trail(shader, s->getTrack());
        }
    }
}

//void GalaxySimulator::drawNormals(GLShader &shader) {
//  int num_tris = cloth->clothMesh->triangles.size();
//
//  MatrixXf positions(4, num_tris * 3);
//  MatrixXf normals(4, num_tris * 3);
//
//  for (int i = 0; i < num_tris; i++) {
//    Triangle *tri = cloth->clothMesh->triangles[i];
//
//    Vector3D p1 = tri->pm1->position;
//    Vector3D p2 = tri->pm2->position;
//    Vector3D p3 = tri->pm3->position;
//
//    Vector3D n1 = tri->pm1->normal();
//    Vector3D n2 = tri->pm2->normal();
//    Vector3D n3 = tri->pm3->normal();
//
//    positions.col(i * 3) << p1.x, p1.y, p1.z, 1.0;
//    positions.col(i * 3 + 1) << p2.x, p2.y, p2.z, 1.0;
//    positions.col(i * 3 + 2) << p3.x, p3.y, p3.z, 1.0;
//
//    normals.col(i * 3) << n1.x, n1.y, n1.z, 0.0;
//    normals.col(i * 3 + 1) << n2.x, n2.y, n2.z, 0.0;
//    normals.col(i * 3 + 2) << n3.x, n3.y, n3.z, 0.0;
//  }
//
//  shader.uploadAttrib("in_position", positions, false);
//  shader.uploadAttrib("in_normal", normals, false);
//
//  shader.drawArray(GL_TRIANGLES, 0, num_tris * 3);
//#ifdef LEAK_PATCH_ON
//  shader.freeAttrib("in_position");
//  shader.freeAttrib("in_normal");
//#endif
//}

//void GalaxySimulator::drawPhong(GLShader &shader) {
//  int num_tris = cloth->clothMesh->triangles.size();
//
//  MatrixXf positions(4, num_tris * 3);
//  MatrixXf normals(4, num_tris * 3);
//  MatrixXf uvs(2, num_tris * 3);
//  MatrixXf tangents(4, num_tris * 3);
//
//  for (int i = 0; i < num_tris; i++) {
//    Triangle *tri = cloth->clothMesh->triangles[i];
//
//    Vector3D p1 = tri->pm1->position;
//    Vector3D p2 = tri->pm2->position;
//    Vector3D p3 = tri->pm3->position;
//
//    Vector3D n1 = tri->pm1->normal();
//    Vector3D n2 = tri->pm2->normal();
//    Vector3D n3 = tri->pm3->normal();
//
//    positions.col(i * 3    ) << p1.x, p1.y, p1.z, 1.0;
//    positions.col(i * 3 + 1) << p2.x, p2.y, p2.z, 1.0;
//    positions.col(i * 3 + 2) << p3.x, p3.y, p3.z, 1.0;
//
//    normals.col(i * 3    ) << n1.x, n1.y, n1.z, 0.0;
//    normals.col(i * 3 + 1) << n2.x, n2.y, n2.z, 0.0;
//    normals.col(i * 3 + 2) << n3.x, n3.y, n3.z, 0.0;
//
//    uvs.col(i * 3    ) << tri->uv1.x, tri->uv1.y;
//    uvs.col(i * 3 + 1) << tri->uv2.x, tri->uv2.y;
//    uvs.col(i * 3 + 2) << tri->uv3.x, tri->uv3.y;
//
//    tangents.col(i * 3    ) << 1.0, 0.0, 0.0, 1.0;
//    tangents.col(i * 3 + 1) << 1.0, 0.0, 0.0, 1.0;
//    tangents.col(i * 3 + 2) << 1.0, 0.0, 0.0, 1.0;
//  }
//
//
//  shader.uploadAttrib("in_position", positions, false);
//  shader.uploadAttrib("in_normal", normals, false);
//  shader.uploadAttrib("in_uv", uvs, false);
//  shader.uploadAttrib("in_tangent", tangents, false);
//
//  shader.drawArray(GL_TRIANGLES, 0, num_tris * 3);
//#ifdef LEAK_PATCH_ON
//  shader.freeAttrib("in_position");
//  shader.freeAttrib("in_normal");
//  shader.freeAttrib("in_uv");
//  shader.freeAttrib("in_tangent");
//#endif
//}

// ----------------------------------------------------------------------------
// CAMERA CALCULATIONS
//
// OpenGL 3.1 deprecated the fixed pipeline, so we lose a lot of useful OpenGL
// functions that have to be recreated here.
// ----------------------------------------------------------------------------

void GalaxySimulator::resetCamera() { camera.copy_placement(canonicalCamera); }

Matrix4f GalaxySimulator::getProjectionMatrix() {
  Matrix4f perspective;
  perspective.setZero();

  double cam_near = camera.near_clip();
  double cam_far = camera.far_clip();

  double theta = camera.v_fov() * M_PI / 360;
  double range = cam_far - cam_near;
  double invtan = 1. / tanf(theta);

  perspective(0, 0) = invtan / camera.aspect_ratio();
  perspective(1, 1) = invtan;
  perspective(2, 2) = -(cam_near + cam_far) / range;
  perspective(3, 2) = -1;
  perspective(2, 3) = -2 * cam_near * cam_far / range;
  perspective(3, 3) = 0;

  return perspective;
}

Matrix4f GalaxySimulator::getViewMatrix() {
  Matrix4f lookAt;
  Matrix3f R;

  lookAt.setZero();

  // Convert CGL vectors to Eigen vectors
  // TODO: Find a better way to do this!

  CGL::Vector3D c_pos = camera.position();
  CGL::Vector3D c_udir = camera.up_dir();
  CGL::Vector3D c_target = camera.view_point();

  Vector3f eye(c_pos.x, c_pos.y, c_pos.z);
  Vector3f up(c_udir.x, c_udir.y, c_udir.z);
  Vector3f target(c_target.x, c_target.y, c_target.z);

  R.col(2) = (eye - target).normalized();
  R.col(0) = up.cross(R.col(2)).normalized();
  R.col(1) = R.col(2).cross(R.col(0));

  lookAt.topLeftCorner<3, 3>() = R.transpose();
  lookAt.topRightCorner<3, 1>() = -R.transpose() * eye;
  lookAt(3, 3) = 1.0f;

  return lookAt;
}

// ----------------------------------------------------------------------------
// EVENT HANDLING
// ----------------------------------------------------------------------------

bool GalaxySimulator::cursorPosCallbackEvent(double x, double y) {
  if (left_down && !middle_down && !right_down) {
    if (ctrl_down) {
      mouseRightDragged(x, y);
    } else {
      mouseLeftDragged(x, y);
    }
  } else if (!left_down && !middle_down && right_down) {
    mouseRightDragged(x, y);
  } else if (!left_down && !middle_down && !right_down) {
    mouseMoved(x, y);
  }

  mouse_x = x;
  mouse_y = y;

  return true;
}

bool GalaxySimulator::mouseButtonCallbackEvent(int button, int action,
                                              int modifiers) {
  switch (action) {
  case GLFW_PRESS:
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      left_down = true;
      break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      middle_down = true;
      break;
    case GLFW_MOUSE_BUTTON_RIGHT:
      right_down = true;
      break;
    }
    return true;

  case GLFW_RELEASE:
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      left_down = false;
      break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      middle_down = false;
      break;
    case GLFW_MOUSE_BUTTON_RIGHT:
      right_down = false;
      break;
    }
    return true;
  }

  return false;
}

void GalaxySimulator::mouseMoved(double x, double y) { y = screen_h - y; }

void GalaxySimulator::mouseLeftDragged(double x, double y) {
  float dx = x - mouse_x;
  float dy = y - mouse_y;

  camera.rotate_by(-dy * (PI / screen_h), -dx * (PI / screen_w));
}

void GalaxySimulator::mouseRightDragged(double x, double y) {
  camera.move_by(mouse_x - x, y - mouse_y, canonical_view_distance);
}

bool GalaxySimulator::keyCallbackEvent(int key, int scancode, int action,
                                      int mods) {
  ctrl_down = (bool)(mods & GLFW_MOD_CONTROL);

  if (action == GLFW_PRESS) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
      is_alive = false;
      break;
    case 'r':
    case 'R':
      galaxy->reset();
      break;
    case ' ':
      resetCamera();
      break;
    case 'p':
    case 'P':
      is_paused = !is_paused;
      break;
    case 'n':
    case 'N':
      if (is_paused) {
        is_paused = false;
        drawContents();
        is_paused = true;
      }
      break;
      // TODO: Extra Keys
    case 'a':
    case 'A':
        galaxy->add_planet();
        drawContents();
      break;
    case 'd':
    case 'D':
        galaxy->remove_planet();
        drawContents();
        break;
    }
  }

  return true;
}

bool GalaxySimulator::dropCallbackEvent(int count, const char **filenames) {
  return true;
}

bool GalaxySimulator::scrollCallbackEvent(double x, double y) {
  camera.move_forward(y * scroll_rate);
//    std::cout << "camera pos: " << camera.position() << "\n";
//    std::cout << "target pos: " << camera.view_point() << "\n";
  return true;
}

bool GalaxySimulator::resizeCallbackEvent(int width, int height) {
  screen_w = width;
  screen_h = height;

  camera.set_screen_size(screen_w, screen_h);
  return true;
}

void GalaxySimulator::initGUI(Screen *screen) {
  Window *window;
  
  window = new Window(screen, "Simulation");
  window->setPosition(Vector2i(default_window_size(0) - 245, 15));
  window->setLayout(new GroupLayout(15, 6, 14, 5));

  // New Planet Parameters
    Sphere *last = galaxy->getLastPlanet();

    sp->newOrigin = last->getInitOrigin();
    sp->newVelocity = last->getInitVelocity();
    sp->newRadius = last->getRadius();
    sp->newMass = last->getMass();

  new Label(window, "New Planet Parameters", "sans-bold");

  {
    Widget *panel = new Widget(window);
    GridLayout *layout =
        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    panel->setLayout(layout);

    new Label(panel, "Origin :", "sans-bold");

    FloatBox<double> *fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(sp->newOrigin.norm() * sp->minMultiplier);
    fb->setMinMaxValues(sp->newOrigin.norm() * sp->minMultiplier, sp->newOrigin.norm() * sp->maxMultiplier);
    fb->setValueIncrement(sp->newOrigin.norm() * sp->minMultiplier / 10.f);
    fb->setUnits("A.U.");
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { sp->newOrigin.x = value; });

    new Label(panel, "Radius :", "sans-bold");

    fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
      fb->setValue(sp->newRadius * sp->minMultiplier);
      fb->setMinMaxValues(sp->newRadius * sp->minMultiplier, sp->newRadius * sp->maxMultiplier);
      fb->setValueIncrement(sp->newRadius * sp->minMultiplier / 10.f);
      fb->setUnits("A.U.");
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { sp->newRadius = value; });

      new Label(panel, "Init Vel :", "sans-bold");

      fb = new FloatBox<double>(panel);
      fb->setEditable(true);
      fb->setFixedSize(Vector2i(100, 20));
      fb->setFontSize(14);
      fb->setValue(sp->newVelocity.norm() * sp->minMultiplier);
      fb->setMinMaxValues(sp->newVelocity.norm() * sp->minMultiplier, sp->newVelocity.norm() * sp->maxMultiplier);
      fb->setValueIncrement(sp->newVelocity.norm() * sp->minMultiplier / 10.f);
    fb->setUnits("A.U.");
    fb->setSpinnable(true);
      fb->setMinValue(0);
      fb->setCallback([this](float value) { sp->newVelocity.y = value; });

      new Label(panel, "Mass :", "sans-bold");

      fb = new FloatBox<double>(panel);
      fb->setEditable(true);
      fb->setFixedSize(Vector2i(100, 20));
      fb->setFontSize(14);
      fb->setValue(sp->newMass * sp->minMultiplier);
      fb->setMinMaxValues(sp->newMass * sp->minMultiplier, sp->newMass * sp->maxMultiplier);
      fb->setValueIncrement(sp->newMass * sp->minMultiplier / 10.f);
      fb->setUnits("A.U.");
      fb->setSpinnable(true);
      fb->setCallback([this](float value) { sp->newMass = value; });

      new Label(panel, "Remove Index:", "sans-bold");

      fb = new FloatBox<double>(panel);
      fb->setEditable(true);
      fb->setFixedSize(Vector2i(100, 20));
      fb->setFontSize(14);
      fb->setValue(1);
      fb->setMinMaxValues(1, galaxy->size() - 1);
//      fb->setUnits("A.U.");
        fb->setValueIncrement(1);
      fb->setSpinnable(true);
      fb->setCallback(
              [this, fb](float value) {
                  sp->delIndex = value;
                  fb->setMinMaxValues(1, galaxy->size() - 1);
              });

      // Add Planet Button
      Button *b = new Button(window, "Add Planet");
      b->setFlags(Button::NormalButton);
      b->setPushed(sp->button_pushed);
      b->setFontSize(14);
      b->setChangeCallback(
              [this](bool state) {
//                  std::cout << "state: " << state << endl;
                  sp->button_pushed = state;
                  if (state) {
                      std::cout << "adding planet using button" << endl;
                      Sphere *newPlanet = new Sphere(sp->newOrigin, sp->newRadius, 1, sp->newVelocity, sp->newMass);
                      galaxy->add_planet(newPlanet);
                      drawContents();
                  }
              });

      // Remove Planet Button
      b = new Button(window, "Remove Planet");
      b->setFlags(Button::NormalButton);
      b->setPushed(sp->button_pushed);
      b->setFontSize(14);
      b->setChangeCallback(
              [this](bool state) {
//                  std::cout << "state: " << state << endl;
                  sp->button_pushed = state;
                  if (state) {
                      std::cout << "removing planet using button" << endl;
                      galaxy->remove_planet(sp->delIndex);
                      drawContents();
                  }
              });
  }

  // Simulation constants

  new Label(window, "Simulation", "sans-bold");

  {
    Widget *panel = new Widget(window);
    GridLayout *layout =
        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    panel->setLayout(layout);

    new Label(panel, "frames/s :", "sans-bold");

    IntBox<int> *fsec = new IntBox<int>(panel);
    fsec->setEditable(true);
    fsec->setFixedSize(Vector2i(100, 20));
    fsec->setFontSize(14);
    fsec->setValue(frames_per_sec);
    fsec->setSpinnable(true);
    fsec->setCallback([this](int value) { frames_per_sec = value; });

    new Label(panel, "steps/frame :", "sans-bold");

    IntBox<int> *num_steps = new IntBox<int>(panel);
    num_steps->setEditable(true);
    num_steps->setFixedSize(Vector2i(100, 20));
    num_steps->setFontSize(14);
    num_steps->setValue(simulation_steps);
    num_steps->setSpinnable(true);
    num_steps->setMinValue(0);
    num_steps->setCallback([this](int value) { simulation_steps = value; });

      // Time Lapse Buttons
      Button *b = new Button(window, "Seconds Per Step");
      b->setFlags(Button::NormalButton);
      b->setPushed(sp->button_pushed);
      b->setFontSize(14);
      b->setChangeCallback(
              [this, num_steps](bool state) {
                  sp->button_pushed = state;
                  if (state) {
                      simulation_steps = seconds;
                      num_steps->setValue(simulation_steps);
                  }
              });

      b = new Button(window, "Hours Per Step");
      b->setFlags(Button::NormalButton);
      b->setPushed(sp->button_pushed);
      b->setFontSize(14);
      b->setChangeCallback(
              [this, num_steps](bool state) {
                  sp->button_pushed = state;
                  if (state) {
                      simulation_steps = hours;
                      num_steps->setValue(simulation_steps);
                  }
              });

      b = new Button(window, "2 Hours Per Step");
      b->setFlags(Button::NormalButton);
      b->setPushed(sp->button_pushed);
      b->setFontSize(14);
      b->setChangeCallback(
              [this, num_steps](bool state) {
                  sp->button_pushed = state;
                  if (state) {
                      simulation_steps = 2 * hours;
                      num_steps->setValue(simulation_steps);
                  }
              });

      b = new Button(window, "3 Hours Per Step");
      b->setFlags(Button::NormalButton);
      b->setPushed(sp->button_pushed);
      b->setFontSize(14);
      b->setChangeCallback(
              [this, num_steps](bool state) {
                  sp->button_pushed = state;
                  if (state) {
                      simulation_steps = 3 * hours;
                      num_steps->setValue(simulation_steps);
                  }
              });

      b = new Button(window, "Toggle Trail");
      b->setFlags(Button::ToggleButton);
      b->setPushed(draw_track);
      b->setFontSize(14);
      b->setChangeCallback(
              [this](bool state) { draw_track = state; });
  }
  
  window = new Window(screen, "Appearance");
  window->setPosition(Vector2i(15, 15));
  window->setLayout(new GroupLayout(15, 6, 14, 5));

  // Appearance

  {
    
    
    ComboBox *cb = new ComboBox(window, shaders_combobox_names);
    cb->setFontSize(14);
    cb->setCallback(
        [this, screen](int idx) { active_shader_idx = idx; });
    cb->setSelectedIndex(active_shader_idx);
  }

  // Shader Parameters

  new Label(window, "Trail Color", "sans-bold");

  {
    ColorWheel *cw = new ColorWheel(window, color);
    cw->setColor(this->color);
    cw->setCallback(
        [this](const nanogui::Color &color) { this->color = color; });
  }

//  new Label(window, "Parameters", "sans-bold");
//
//  {
//    Widget *panel = new Widget(window);
//    GridLayout *layout =
//        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
//    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
//    layout->setSpacing(0, 10);
//    panel->setLayout(layout);
//
//    new Label(panel, "Normal :", "sans-bold");
//
//    FloatBox<double> *fb = new FloatBox<double>(panel);
//    fb->setEditable(true);
//    fb->setFixedSize(Vector2i(100, 20));
//    fb->setFontSize(14);
//    fb->setValue(this->m_normal_scaling);
//    fb->setSpinnable(true);
//    fb->setCallback([this](float value) { this->m_normal_scaling = value; });
//
//    new Label(panel, "Height :", "sans-bold");
//
//    fb = new FloatBox<double>(panel);
//    fb->setEditable(true);
//    fb->setFixedSize(Vector2i(100, 20));
//    fb->setFontSize(14);
//    fb->setValue(this->m_height_scaling);
//    fb->setSpinnable(true);
//    fb->setCallback([this](float value) { this->m_height_scaling = value; });
//  }
}

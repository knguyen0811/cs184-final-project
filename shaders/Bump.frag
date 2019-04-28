#version 330

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

uniform vec4 u_color;

uniform sampler2D u_texture_3;
uniform vec2 u_texture_3_size;

uniform float u_normal_scaling;
uniform float u_height_scaling;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_uv;

out vec4 out_color;

float h(vec2 uv) {
  // You may want to use this helper function...
  vec4 color = texture(u_texture_3, uv);
  return color.r;
}

void main() {
  // YOUR CODE HERE
  float width = u_texture_3_size.x;
  float height = u_texture_3_size.y;
  float huv = h(v_uv);
  float kh = 2.f * u_height_scaling;
  float kn = u_normal_scaling;
//  float du = (h(vec2((v_uv.x + 1.f)/width, v_uv.y)) - huv) * kh * kn;
//  float dv = (h(vec2(v_uv.x, (v_uv.y + 1.f)/height)) - huv) * kh * kn;
  float du = (h(vec2(v_uv.x + 1.f/width, v_uv.y)) - huv) * kh * kn;
  float dv = (h(vec2(v_uv.x, v_uv.y + 1.f/height)) - huv) * kh * kn;

  mat3 tbn = mat3(v_tangent.xyz, cross(v_normal.xyz, v_tangent.xyz), v_normal.xyz);
  vec3 no = vec3(-du, -dv, 1);
  vec3 nd = normalize(tbn * no);

  // NOTE: Phong code:
  vec3 l = normalize(u_light_pos - v_position.xyz);
  float maxd = max(0.f, dot(nd, l));
  vec4 intensity4 = vec4(u_light_intensity,0);

  vec3 v = normalize(u_cam_pos - v_position.xyz);
  vec3 h = ((v + l) / length(v + l));
  float maxs = pow(max(0.f, dot(nd, h)), 100.f);

//  vec3 rvec = u_light_pos - v_position.xyz;
  float dist = distance(u_light_pos, v_position.xyz);
  float dist2 = dist * dist;

  float kd = 1.f;
  float ks = 1.f;
  float ka = 0.0001f;
  vec4 ia = vec4(1,1,1,0);

  out_color = ka*ia + kd * intensity4 * maxd / dist2 + ks * intensity4 * maxs / dist2;
  out_color *= u_color;

  out_color.a = 1;
}


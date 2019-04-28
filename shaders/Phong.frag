#version 330

uniform vec4 u_color;
uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

void main() {
  // YOUR CODE HERE
  vec3 l = normalize(u_light_pos - v_position.xyz);
  float maxd = max(0.f, dot(v_normal.xyz, l));
  vec4 intensity4 = vec4(u_light_intensity,0);

  vec3 v = normalize(u_cam_pos - v_position.xyz);
  vec3 h = ((v + l) / length(v + l));
  float maxs = pow(max(0.f, dot(v_normal.xyz, h)), 100.f);

//  vec3 rvec = u_light_pos - v_position.xyz;
  float dist = distance(u_light_pos, v_position.xyz);
  float dist2 = dist * dist;

//  vec3 rvecs = u_cam_pos - v_position.xyz;
//  float dists = length(rvecs);
//  float dist2s = dists * dists;
  float kd = 1.f;
  float ks = 1.f;
  float ka = 0.0001f;
  vec4 ia = vec4(1,1,1,0);

  out_color = ka*ia + kd * intensity4 * maxd / dist2 + ks * intensity4 * maxs / dist2;
  out_color *= u_color;

  out_color.a = 1;
}


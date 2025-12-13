"  #version 330 core\n"
"  layout (location = 0) in vec3 vP;\n"
"  layout (location = 1) in vec4 vC;\n"
"  out vec4 fC;\n"
"  uniform mat4 vp;\n"
"  void main() {\n"
"      fC = vC;\n"
"      gl_Position = vp * vec4(vP, 1);\n"
"  }";

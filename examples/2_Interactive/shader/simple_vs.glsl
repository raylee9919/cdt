"  #version 330 core\n"
"  layout (location = 0) in vec2 vP;\n"
"  uniform mat4 vp;\n"
"  void main() {\n"
"      gl_Position = vp * vec4(vP, 0, 1);\n"
"  }";

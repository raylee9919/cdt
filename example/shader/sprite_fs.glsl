"  #version 330 core\n"
"  in vec2 f_uv;\n"
"  out vec4 f_color;\n"
"  uniform sampler2D tex;\n"
"  void main() {\n"
"      vec4 c = texture(tex, f_uv);\n"
"      if (c.a==0.f) { discard; }\n"
"      f_color = c;\n"
"  }";

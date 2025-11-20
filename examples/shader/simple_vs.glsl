R"(
#version 330 core

layout (location = 0) in vec2 vP;
uniform float scale;

void main()
{
    float x = scale*vP.x;
    float y = scale*vP.y;
    gl_Position = vec4(x, y, 0, 1);
}

)";

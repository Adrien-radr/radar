#version 400

in vec3 in_position;
in vec4 in_color;
in vec3 in_offset;

uniform mat4 ProjMatrix;

out vec4 v_color;

void main() {
    v_color = in_color;

    vec4 p = ProjMatrix * vec4(in_offset, 1.0) + vec4(in_position, 0.0);
    gl_Position = p; 
    // gl_Position = vec4(p.x, p.y, 0, p.w);
}
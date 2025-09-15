#version 330 core

in vec4 vClr;
in vec2 vTec;
out vec4 fragClr;

uniform vec4 base_color;

void main() {
	fragClr = base_color * vClr;
}

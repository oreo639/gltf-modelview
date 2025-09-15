#version 330 core

in vec4 vClr;
in vec2 vTec;
out vec4 fragClr;

uniform sampler2D texture1;

void main() {
	fragClr = texture(texture1, vTec) * vClr;
}

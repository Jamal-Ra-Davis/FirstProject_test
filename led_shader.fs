#version 330 core
out vec4 FragColor;

uniform vec3 ledColor;

void main()
{
	FragColor = vec4(ledColor, 1.0);
}
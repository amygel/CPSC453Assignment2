// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// location indices for these attributes correspond to those specified in the
// InitializeGeometry() function of the main program
layout(location = 0) in vec2 VertexPosition;
layout(location = 1) in vec3 VertexColour;
layout(location = 2) in vec2 VertexTexture;

// output to be interpolated between vertices and passed to the fragment stage
out vec3 Colour;
out vec2 textureCoords;

uniform float angle;
uniform vec2 offset;

void main()
{
	vec2 newCoords;

	// Rotate the coordinates
	newCoords.x = VertexPosition.x * cos(angle) - VertexPosition.y * sin(angle);
	newCoords.y = VertexPosition.y * cos(angle) + VertexPosition.x * sin(angle);

	// Translate the coordinates
	newCoords += offset;

    // assign modified vertex position
    gl_Position = vec4(newCoords, 0.0, 1.0);

    // assign output colour/ texture to be interpolated
	textureCoords = VertexTexture;
    Colour = VertexColour;
}

// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
in vec3 Colour;
in vec2 textureCoords;

mat3 hSobel = {
	{ -1.0, -2.0, -1.0},
	{ 0.0, 0.0, 0.0},
	{ 1.0, 2.0, 1.0 }};
mat3 vSobel = {
	{ 1.0, 0.0, -1.0},
	{ 2.0, 0.0, -2.0},
	{ 1.0, 0.0, -1.0 }};
mat3 unsharp = {
	{ 0.0, -1.0, 0.0},
	{ -1.0, 5.0, -1.0},
	{ 0.0, -1.0, 0.0 }};

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

// our texture to read from
uniform sampler2DRect tex;
uniform int filter;

void main(void)
{
	vec4 texColour = texture(tex, textureCoords);

	if(filter == 0)
	{
		FragmentColour = texColour;
		return;
	}

	mat3 I;
	mat3 F;

	if(filter == 1)
	{
		F = vSobel;
	}
	else if(filter == 2)
	{
		F = hSobel;
	}
	else if(filter == 3)
	{
		F = unsharp;
	}

	// Create matrix with surrounding textures
	for (int i=0; i<3; i++)
    {
        for (int j=0; j<3; j++) {
            vec4 smt = texture( tex, ivec2(textureCoords) + ivec2(i-1,j-1));
            I[i][j] = length(smt.rgb); 
        }
    }

	// Calculate the convolution values for the mask
    float dp3 = dot(F[0], I[0]) + dot(F[1], I[1]) + dot(F[2], I[2]);

	FragmentColour = vec4(0.5 * sqrt(dp3 * dp3 * dp3 * dp3));
}

// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
in vec2 textureCoords;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

// our texture to read from
uniform sampler2DRect tex;
uniform int filter;

void main(void)
{
	// Get texture
	vec4 texColour = texture(tex, textureCoords);

	// Don't bother doing the calculations if not filtering
	if(filter == 0)
	{
		FragmentColour = texColour;
		return;
	}

	mat3 I;
	mat3 F;

	// Fill matrix with selected filter
	if(filter == 1)
	{
		// vertical sobel
		F[0]=vec3(1.0, 0.0, -1.0);
		F[1]=vec3(2.0, 0.0, -2.0);
		F[2]=vec3(1.0, 0.0, -1.0);
	}
	else if(filter == 2)
	{
	    // horizontal sobel
		F[0]=vec3(-1.0, -2.0, -1.0);
		F[1]=vec3(0.0, 0.0, 0.0);
		F[2]=vec3(1.0, 2.0, 1.0);
	}
	else if(filter == 3)
	{
		// unsharp mask
		F[0]=vec3(0.0, -1.0, 0.0);
		F[1]=vec3(-1.0, 5.0, -1.0);
		F[2]=vec3(0.0, -1.0, 0.0 );
	}

	// Create matrix with surrounding textures
	for (int i=0, k=2; i<3; i++, k--)
    {
        for (int j=0; j<3; j++) {
            vec4 smt = texture( tex, ivec2(textureCoords) + ivec2(j-1,k-1));
            I[i][j] = length(smt.rgb); 
        }
    }

	// Calculate the convolution values for the mask
    float dotprod = dot(F[0], I[0]) + dot(F[1], I[1]) + dot(F[2], I[2]);

	FragmentColour = vec4(0.5 * abs(dotprod));
}

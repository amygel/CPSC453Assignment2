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

// various colour effects
const vec3 SEPIA = vec3(1.2, 1.0, 0.8); 
const vec3 GREYSCALE1 = vec3(0.333, 0.333, 0.333); 
const vec3 GREYSCALE2 = vec3(0.299, 0.587, 0.114); 
const vec3 GREYSCALE3 = vec3(0.213, 0.715, 0.072);

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

// our texture to read from
uniform sampler2DRect tex;

uniform int colourEffect;

uniform mat3 filters[3] = mat3[](
    mat3( 1.0, 2.0, 1.0, 0.0, 0.0, 0.0, -1.0, -2.0, -1.0 ),
    mat3( -1.0, 0.0, 1.0, -2.0, 0.0, 2.0, -1.0, 0.0, -1.0 ),
	mat3( 0.0, -1.0, 0.0, -1.0, 5.0, -1.0, 0.0, -1.0, 0.0 )
);

void main(void)
{
	vec4 texColour = texture(tex, textureCoords);

	vec4 newColour;

	// Part 1
	//float luminance;
		
	//if(colourEffect == 1)
	//{
	//	luminance = dot(texColour.rgb, GREYSCALE2);
	//	newColour = vec4(vec3(luminance, luminance, luminance) * SEPIA, 1.0);
	//}
	//else if(colourEffect == 2)
	//{
	//	luminance = dot(texColour.rgb, GREYSCALE1);
	//	newColour = vec4(luminance, luminance, luminance, 0.0);
	//}
	//else if(colourEffect == 3)
	//{
	//	luminance = dot(texColour.rgb, GREYSCALE2);
	//	newColour = vec4(luminance, luminance, luminance, 0.0);
	//}
	//else if(colourEffect == 4)
	//{
	//	luminance = dot(texColour.rgb, GREYSCALE3);
	//	newColour = vec4(luminance, luminance, luminance, 0.0);
	//}
	//else 
	//{
	//	newColour = texColour;
	//}

	// Part 2
	mat3 I;
	float cnv[2];

	for (int i=0; i<3; i++)
    {
        for (int j=0; j<3; j++) {
            vec3 smt = texture( tex, ivec2(textureCoords) + ivec2(i-1,j-1));
            I[i][j] = length(smt.rgb); 
        }
    }

	// Calculate the convolution values for all the masks
    for (int i=0; i<2; i++) {
        float dp3 = dot(filters[i][0], I[0]) + dot(filters[i][1], I[1]) + dot(filters[i][2], I[2]);
        cnv[i] = dp3 * dp3; 
    }
	newColour = vec4(0.5 * sqrt(cnv[0]*cnv[0]+cnv[1]*cnv[1]));

	FragmentColour = newColour;
}

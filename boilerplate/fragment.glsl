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

void main(void)
{
	vec4 texColour = texture(tex, textureCoords);

	float luminance;
	vec4 newColour;
	
	switch(colourEffect)
	{
	case 0:
	default:
		newColour = texColour;
		break;
	case 1:
		luminance = dot(texColour.rgb, GREYSCALE1);
		newColour = vec4(luminance, luminance, luminance, 0.0);
		break;
	case 2:
		luminance = dot(texColour.rgb, GREYSCALE2);
		newColour = vec4(luminance, luminance, luminance, 0.0);
		break;
	case 3:
		luminance = dot(texColour.rgb, GREYSCALE3);
		newColour = vec4(luminance, luminance, luminance, 0.0);
		break;
	case 4:
		luminance = dot(texColour.rgb, GREYSCALE2);
		newColour = vec4(vec3(luminance, luminance, luminance) * SEPIA, 1.0);
		break;
	};

	FragmentColour = newColour;
}

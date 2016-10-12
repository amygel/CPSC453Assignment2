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
uniform int blur;


// constant values
const float PI = 3.14159265;
const vec2 texOffset = vec2(1.0, 1.0);

void main(void)
{
	// Don't bother doing the calculations if not blurring
	if(blur == 0)
	{
		FragmentColour = texture(tex, textureCoords);;
		return;
	}

	float sigma;
	float numBlurPixelsPerSide;

	// Populate with selected blur values
	if(blur == 1)
	{
		sigma = 1.5;
		numBlurPixelsPerSide = 1.0;
	}
	else if(blur == 2)
	{
		sigma = 1.96;
		numBlurPixelsPerSide = 2.0;
	}
	else if(blur == 3)
	{
		sigma = 2.56;
		numBlurPixelsPerSide = 3.0;
	}

	vec4 avgValue = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	float coefficientSum = 0.0f;

	// Incremental Gaussian Coefficent Calculation
	vec3 incrementalGaussian;
	incrementalGaussian.x = 1.0f / (sqrt(2.0f * PI) * sigma);
	incrementalGaussian.y = exp(-0.5f / (sigma * sigma));
	incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;
	
	// First take the sample of the centre
	avgValue += texture(tex, textureCoords) * incrementalGaussian.x;
	coefficientSum += incrementalGaussian.x;
	incrementalGaussian.xy *= incrementalGaussian.yz;
	
	// Go through the remaining samples
	for (float i = 1.0f; i <= numBlurPixelsPerSide; i++) { 
		avgValue += texture(tex, textureCoords - i * texOffset) * incrementalGaussian.x;         
		avgValue += texture(tex, textureCoords + i * texOffset) * incrementalGaussian.x;         
		coefficientSum += 2 * incrementalGaussian.x;
		incrementalGaussian.xy *= incrementalGaussian.yz;
	}

	FragmentColour = avgValue / coefficientSum;
}

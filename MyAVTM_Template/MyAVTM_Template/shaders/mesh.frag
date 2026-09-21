#version 430

struct Materials {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float shininess;
	int texCount;
};

in Data {
	vec3 normal;
	vec3 eye;
	vec2 tex_coord;
} DataIn;

uniform Materials mat;

uniform sampler2D texmap;
uniform sampler2D texmap1;
uniform sampler2D texmap2;
uniform sampler2D texmap3;

uniform int texMode;

// Controls for the lights
uniform bool dayMode;
uniform bool candleMode;
uniform bool headlightMode;

uniform vec3 lightDir; // Directional light direction
uniform vec4 candlePos[6]; // Candle light positions
uniform vec3 headlightDir; // Headlight direction
uniform vec4 headlightPos[2]; // Headlight positions
uniform float spotCosCutOff;
uniform float spotExp;

out vec4 colorOut;

void main() {
	vec4 texel, texel1;

	vec4 spec = vec4(0.0);
	float intensity = 0.0f;
	float intSpec = 0.0f;

	float totalIntensity = 0.0; // Accumulate intensity from all light sources
	vec4 totalSpecular = vec4(0.0); // Accumulate specular contributions from all light sources

	float att = 0.0;

	vec3 n = normalize(DataIn.normal);
	vec3 e = normalize(-DataIn.eye);
	vec3 sd = normalize(headlightDir);

	vec3 totalLight = vec3(0.0); // Accumulate light contributions from all light sources
	vec3 diffuse = mat.diffuse.rgb; // Diffuse color of the material
	vec3 specular = mat.specular.rgb; // Specular color of the material

	if (dayMode) {
		vec3 l = normalize(-lightDir); // Directional light direction
		intensity = max(dot(n, l), 0.0);
		totalIntensity += intensity;

		if (intensity > 0.0) { // Only calculate specular if the light is hitting the surface
			vec3 h = normalize(l + e);
			intSpec = max(dot(h, n), 0.0);
			totalSpecular += mat.specular * pow(intSpec, mat.shininess);
		}
	}

	if (candleMode) {
		for (int i = 0; i < 6; ++i) {
			vec3 lightVector = candlePos[i].xyz - DataIn.eye;
			vec3 l = normalize(lightVector);
			float dist = length(lightVector);

			att = 1.0 / (1.0 + 0.05 * dist + 0.01 * dist * dist); // Inverse square so that light intensity decreases with distance
			intensity = max(dot(n, l), 0.0) * att;
			totalIntensity += intensity;

			if (intensity > 0.0) { // Only calculate specular if the light is hitting the surface
				vec3 h = normalize(l + e);
				intSpec = max(dot(h, n), 0.0);
				totalSpecular += mat.specular * pow(intSpec, mat.shininess) * att;
			}
		}
	}

	if (headlightMode) {
		for (int i = 0; i < 2; ++i) {
			vec3 lightVector = headlightPos[i].xyz - DataIn.eye;
			vec3 l = normalize(lightVector);
			float dist = length(lightVector);
			att = 1.0 / (1.0 + 0.05 * dist + 0.01 * dist * dist); // Inverse square so that light intensity decreases with distance

			float spotCos = dot(-l, sd);
			if (spotCos > spotCosCutOff) { // Check if the fragment is within the spotlight cone
				att = pow(spotCos, spotExp) / (1.0 + 0.05 * dist + 0.01 * dist * dist);
				
				intensity = max(dot(n, l), 0.0) * att;
				totalIntensity += intensity;

				if (intensity > 0.0) { // Only calculate specular if the light is hitting the surface
					vec3 h = normalize(l + e);
					intSpec = max(dot(h, n), 0.0);
					totalSpecular += mat.specular * pow(intSpec, mat.shininess) * att;
				}
			}
		}
	}

	vec4 emission = vec4(0.0);

	if (headlightMode)
		emission = mat.emissive;

	if (texMode == 0) // No texturing
		colorOut = vec4((max(totalIntensity * mat.diffuse + totalSpecular, mat.ambient) + emission).rgb, 1.0);

	else if (texMode == 1) // Modulate diffuse color with texel color
	{
		texel = texture(texmap2, DataIn.tex_coord);  // texel from lighwood.tga
		colorOut = vec4(max(totalIntensity * mat.diffuse * texel + totalSpecular,0.07 * texel).rgb, 1.0);
	}
	else if (texMode == 2) // Diffuse color is replaced by texel color
	{
		texel = texture(texmap, DataIn.tex_coord);  // texel from stone.tga
		colorOut = vec4(max(totalIntensity * texel + totalSpecular, 0.07 * texel).rgb, 1.0);
	}
	else if (texMode == 4) // Diffuse color is replaced by texel color
	{
		texel = texture(texmap3, DataIn.tex_coord);  // texel from checker.tga
		colorOut = vec4(max(totalIntensity * texel + totalSpecular, 0.07 * texel).rgb, 1.0);
	}
	else // Multitexturing
	{
		texel = texture(texmap2, DataIn.tex_coord);  // texel from lighwood.tga
		texel1 = texture(texmap1, DataIn.tex_coord);  // texel from checker.tga
		colorOut = vec4(max(totalIntensity * texel * texel1 + totalSpecular, 0.07 * texel * texel1).rgb, 1.0);
	}
}

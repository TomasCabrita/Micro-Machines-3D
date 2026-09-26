#version 430

out vec4 colorOut;

uniform	sampler2D texUnitDiff;
uniform	sampler2D texUnitDiff1;
uniform	sampler2D texUnitSpec;
uniform	sampler2D texUnitNormalMap;

in Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir;
	vec2 TexCoord;
} DataIn;

struct Materials {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float shininess;
	int texCount;
};
uniform Materials mat;

uniform bool normalMap;  //for normal mapping
uniform bool specularMap;
uniform uint diffMapCount;


vec4 diff, auxSpec;

void main() {

	vec4 spec = vec4(0.0);
	vec3 n;

	if(normalMap)
		n = normalize(2.0 * texture(texUnitNormalMap, DataIn.TexCoord).rgb - 1.0);  //normal in tangent space
	else
		n = normalize(DataIn.normal);

	//If bump mapping, normalMap == TRUE, then lightDir and eye vectores come from vertex shader in tangent space
	vec3 l = normalize(DataIn.lightDir);
	vec3 e = normalize(DataIn.eye);

	float intensity = max(dot(n,l), 0.0);

	if(mat.texCount == 0) {
		diff = mat.diffuse;
		auxSpec = mat.specular;
	}
	else {
		if(diffMapCount == 0)
			diff = mat.diffuse;
		else if(diffMapCount == 1)
			diff = mat.diffuse * texture(texUnitDiff, DataIn.TexCoord);
		else
			diff = mat.diffuse * texture(texUnitDiff, DataIn.TexCoord) * texture(texUnitDiff1, DataIn.TexCoord);

		if(specularMap) 
			auxSpec = mat.specular * texture(texUnitSpec, DataIn.TexCoord);
		else
			auxSpec = mat.specular;
	}

	if (intensity > 0.0) {
		vec3 h = normalize(l + e);
		float intSpec = max(dot(h,n), 0.0);
		spec = auxSpec * pow(intSpec, mat.shininess);
		
	}

	colorOut = vec4((max(intensity * diff, diff*0.15) + spec).rgb, 1.0);
}
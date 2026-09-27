#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;


uniform vec4 l_pos;

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria
in vec4 tangent;
in vec4 texCoord;

out Data {
	vec3 normal;
	vec3 eye;
	vec2 tex_coord;
	mat3 tbn; // Tangent, Bitangent, Normal matrix for normal mapping
	vec3 skyboxTexCoord;
} DataOut;

void main () {
	vec3 t = normalize(m_normal * tangent.xyz);
	vec3 n = normalize(m_normal * normal.xyz);
	vec3 b = cross(n, t) * tangent.w; // Calculate bitangent
	    
	vec3 dir = position.xyz - vec3(0.5);

	DataOut.normal = n;
	DataOut.eye = (m_viewModel * position).xyz;
	DataOut.tex_coord = texCoord.st;
	DataOut.tbn = mat3(t, b, n);
	DataOut.skyboxTexCoord = dir;

    gl_Position = m_pvm * position;
}

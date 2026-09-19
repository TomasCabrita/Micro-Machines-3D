#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;


uniform vec4 l_pos;

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria
in vec4 texCoord;

out Data {
	vec3 normal;
	vec3 eye;
	vec2 tex_coord;
} DataOut;

void main () {
	DataOut.normal = normalize(m_normal * normal.xyz);
	DataOut.eye = (m_viewModel * position).xyz;
	DataOut.tex_coord = texCoord.st;

	gl_Position = m_pvm * position;	
}

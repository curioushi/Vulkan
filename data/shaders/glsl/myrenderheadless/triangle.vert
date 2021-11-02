#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 outColor;

out gl_PerVertex {
	vec4 gl_Position;   
};

layout(push_constant) uniform PushConsts {
	mat4 mvp;
} pushConsts;

void main() 
{
	outColor = inColor;
	vec4 p = pushConsts.mvp * vec4(inPos.xyz, 1.0);
	gl_Position = vec4(p.x/p.z, p.y/p.z, p.z, 1.0);
}

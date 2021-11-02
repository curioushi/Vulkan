#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 outColor;

out gl_PerVertex {
	vec4 gl_Position;   
};

layout(push_constant) uniform PushConsts {
	mat4 model_view;
	mat4 proj;
	float far_z;
} constants;

void main() 
{
	outColor = inColor;
	vec4 pView = constants.model_view * vec4(inPos.xyz, 1.0);
	vec4 pImg = pView / pView.z;
	pImg = constants.proj * pImg;
	float ndc_depth = pView.z / constants.far_z;
	gl_Position = vec4(pImg.x, pImg.y, ndc_depth, 1.0);
}

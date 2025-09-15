#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aClr;
layout (location = 2) in vec3 aNrm;
layout (location = 3) in vec2 aTec;
layout (location = 4) in ivec4 aBoneIds;
layout (location = 5) in vec4 aWeights;

out vec4 vClr;
out vec2 vTec;
out vec3 vFragPos;
out vec3 vNrm;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];
uniform bool bUseBoneMatrices;

void main()
{
	vec4 totalPosition = vec4(0.0f);
	vec3 totalNormal = vec3(0.0f);
	if (bUseBoneMatrices) {
		for (int i = 0 ; i < MAX_BONE_INFLUENCE ; i++) {
			if (aBoneIds[i] == -1)
				continue;
			if (aBoneIds[i] >= MAX_BONES) {
				totalPosition = vec4(aPos,1.0f);
				break;
			}

			vec4 localPosition = finalBonesMatrices[aBoneIds[i]] * vec4(aPos,1.0f);
			totalPosition += localPosition * aWeights[i];
			vec3 localNormal = mat3(finalBonesMatrices[aBoneIds[i]]) * aNrm;
			totalNormal += localNormal * aWeights[i];
		}
	} else {
		totalPosition = vec4(aPos,1.0f);
		totalNormal = aNrm;
	}

	gl_Position = projection * view * model * totalPosition;
	vFragPos = vec3(model * totalPosition);
	vNrm = normalize(mat3(model) * totalNormal);
	vClr = aClr;
	vTec = aTec;
}

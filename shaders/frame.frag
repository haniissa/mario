#version 450

// Samples the game frame and writes it straight out. Nearest-neighbour is done
// by the sampler (VK_FILTER_NEAREST) rather than here, which is what keeps the
// pixel art crisp when the viewport scales it up.

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D frameTex;

void main(){
    outColor = texture(frameTex, vUV);
}
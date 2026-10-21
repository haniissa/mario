#version 450

//Presents the game frame as a full-screen triangle.
// There is no vertex buffer: the three corners are generated from
// gl_vertexIndex, which is the samllest possible way to cover the screen. The 
// letterboxing is not done here either -- host sets a viewport that is the 
// centred, integer-scaled 400x224 rectangle, so the GPU does the scaling and
// the cleared background shows through as the bars

layout(location = 0) out vec2 vUV;

void main(){
    // 0 -> (0, 0), 1 ->  (0, 2): atriangle twice the size of teh
    // screen, so the visible part is exactly the screen rectangle.
    vec2 corner = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    vUV = corner;
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
#version 330 core

layout(location = 0) in vec3 position;     // Vertex position
layout(location = 1) in vec3 normal;       // Vertex normal
layout(location = 2) in vec2 texCoord;     // Vertex texture coordinates

out vec3 fragNormal;           // Normal to pass to fragment shader
out vec2 fragTexCoord;        // Texture coordinates to pass to fragment shader
out vec4 fragPosition;        // Transformed vertex position to pass to fragment shader

uniform mat4 model;            // Model matrix
uniform mat4 view;             // View matrix
uniform mat4 projection;       // Projection matrix

void main() {
    fragPosition = model * vec4(position, 1.0);  // Apply model transformation
    fragNormal = mat3(transpose(inverse(model))) * normal;  // Apply model transformation to normals
    fragTexCoord = texCoord;  // Pass texture coordinates to fragment shader

    gl_Position = projection * view * fragPosition;  // Apply projection and view transformations
}

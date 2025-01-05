#version 330 core

in vec3 fragNormal;            // Normal from vertex shader
in vec2 fragTexCoord;         // Texture coordinates from vertex shader
in vec4 fragPosition;         // Transformed vertex position

out vec4 color;                // Final output color of the fragment

uniform vec3 lightPos;         // Position of the light source
uniform vec3 viewPos;          // Camera (viewer's) position
uniform sampler2D texture1;    // Texture sampler

void main() {
    // Normalizing the interpolated normal
    vec3 norm = normalize(fragNormal);
    
    // Light direction (from fragment to light)
    vec3 lightDir = normalize(lightPos - fragPosition.xyz);
    
    // Ambient lighting (constant)
    vec3 ambient = 0.1 * vec3(1.0, 1.0, 1.0);  // White ambient light

    // Diffuse lighting (depends on angle between light and surface normal)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);  // White diffuse light

    // Specular lighting (shininess factor)
    vec3 viewDir = normalize(viewPos - fragPosition.xyz);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);  // Specular exponent for shininess
    vec3 specular = 0.5 * spec * vec3(1.0, 1.0, 1.0);  // White specular light

    // Combine lighting components
    vec3 result = ambient + diffuse + specular;

    // Apply texture mapping
    vec4 texColor = texture(texture1, fragTexCoord);

    // Final color (multiply result of lighting with texture color)
    color = vec4(result, 1.0) * texColor;
}

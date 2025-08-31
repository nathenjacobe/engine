#version 460 core
in vec3 frag_position;
in vec3 normal;
in vec2 tex_coords;
out vec4 frag_color;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 
  
uniform Material material;
uniform vec3 light_position;
uniform vec3 light_color;
uniform vec3 view_position;
uniform sampler2D primary_texture;
uniform float constant_attenuation;
uniform float linear_attenuation;
uniform float quadratic_attenuation;

uniform bool render_wireframe;

void main()
{
    if (render_wireframe) {
        frag_color = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }
    
    vec3 ambient = light_color * material.ambient;
    
    vec3 norm = normalize(normal);
    vec3 light_direction = normalize(light_position - frag_position);
    
    float distance = length(light_position - frag_position);
    float attenuation = 1.0 / (
        constant_attenuation +
        linear_attenuation * distance + 
        quadratic_attenuation * (distance * distance)
    );
    
    float diff = max(dot(norm, light_direction), 0.0);
    vec3 diffuse = light_color * (diff * material.diffuse);
 
    vec3 view_direction = normalize(view_position - frag_position);
    vec3 reflect_direction = reflect(-light_direction, norm);  
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), material.shininess);
    vec3 specular = light_color * (spec * material.specular);  
    
    vec3 result = ambient + (diffuse + specular) * attenuation;
    frag_color = texture(primary_texture, tex_coords) * vec4(result, 1.0);
}

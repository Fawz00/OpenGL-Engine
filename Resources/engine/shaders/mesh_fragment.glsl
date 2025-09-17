#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;

uniform float time;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_height1;

uniform vec3 viewPos;

void main()
{
    vec3 lightPosition = vec3(10.0, 10.0, 10.0);
    vec3 lightColor = vec3(1.0, 0.9, 0.875);
    float lightIntensity = 1.0;

    vec4 color = texture(texture_diffuse1, TexCoords);
    if (color.a < 0.1)
        discard;

    color.a = 0.5;

	// Ambient
    vec3 ambient = vec3(0.5) * color.rgb;

	// diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPosition - Position);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * diff * color.rgb;

    // specular
    vec3 viewDir = normalize(viewPos - Position);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2.0);
    vec3 specular = lightIntensity * spec * texture(texture_specular1, TexCoords).rgb;  

    color = vec4(ambient + diffuse + specular, color.a);

    //color.rgb = mix( clamp((viewPos - Position)*20.0, 0.0, 1.0), color.rgb, 0.5);

	FragColor = color;
}
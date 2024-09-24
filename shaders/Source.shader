#shader vertex
#version 330 core

noperspective out vec2 TexCoord;

void main(void){

    TexCoord.x = (gl_VertexID == 2)? 2.0: 0.0;
    TexCoord.y = (gl_VertexID == 1)? 2.0: 0.0;

    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);
}

#shader fragment
#version 330 core

uniform sampler2D buffer;
uniform float brightness;
noperspective in vec2 TexCoord;

out vec4 FragColor;

void main(void){
    vec4 texColor = texture(buffer, TexCoord);
    texColor.rgb *= brightness;  // Multiply the color by brightness
    FragColor = texColor;
}
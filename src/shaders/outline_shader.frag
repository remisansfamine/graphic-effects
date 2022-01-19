#version 330 core

mat3 sx = mat3( 
    1.0, 2.0, 1.0, 
    0.0, 0.0, 0.0, 
   -1.0, -2.0, -1.0 
);
mat3 sy = mat3( 
    1.0, 0.0, -1.0, 
    2.0, 0.0, -2.0, 
    1.0, 0.0, -1.0 
);

in VS_OUT
{
    vec2 UVs;
}fs_in;

uniform sampler2D uScreenTexture;
uniform vec2 uSmooth;
uniform vec3 uEdgeColor;

// Shader outputs
out vec4 oColor;

void main()
{
    mat3 I;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            vec3 sample  = texelFetch(uScreenTexture, ivec2(gl_FragCoord) + ivec2(i-1,j-1), 0 ).rgb;
            I[i][j] = length(sample); 
        }
    }

    float gx = dot(sx[0], I[0]) + dot(sx[1], I[1]) + dot(sx[2], I[2]); 
    float gy = dot(sy[0], I[0]) + dot(sy[1], I[1]) + dot(sy[2], I[2]);
    
    float g = sqrt(pow(gx, 2.0)+pow(gy, 2.0));

    // Try different values and see what happens
    g = smoothstep(uSmooth.x, uSmooth.y, g);

    vec3 diffuse = texture(uScreenTexture, fs_in.UVs).rgb;
    oColor = vec4(mix(diffuse, uEdgeColor, g), 1.);
}
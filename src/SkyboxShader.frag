//#version 330 core
out vec4 oColor;

in vec3 vPos;
  
uniform samplerCube environmentMap;
  
void main()
{
    vec3 envColor = texture(environmentMap, vPos).rgb;
    
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));  //Gamma Correction
  
    oColor = vec4(envColor, 1.0);
}

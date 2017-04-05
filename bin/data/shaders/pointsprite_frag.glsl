#version 400
in vec3 v_eyeSpace_pos;
in vec3 v_eyeSpace_LightDir;

uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;

out vec4 frag_color;

float LinearizeDepth(float depth)
{
    float near = 1.0;
    float far = 200.0;
    float z = depth * 2.0 - 1.0;
    z = ((2.0 * far * near) / (far + near - z * (far - near))) / far;

    return z;
}

void main()
{
    vec3 N;
    N.xy = gl_PointCoord * 2.0 - 1.0;
    float R2 = dot(N.xy, N.xy);
    if(R2 > 1.0) discard;
    N.z = sqrt(1.0 - R2);

    // depth
    float SphereRadius = 1.0;
    vec4 eyeSpacePoint = vec4(v_eyeSpace_pos + N*SphereRadius, 1.0);
    vec4 clipSpacePoint= ProjMatrix * eyeSpacePoint;
    float depth = clipSpacePoint.z / clipSpacePoint.w;
    float linear_depth = LinearizeDepth(depth);


    float diffuse = max(0.0, dot(N, normalize(v_eyeSpace_LightDir)));

    //frag_color = vec4(0.8*diffuse, 0.2+0.8*diffuse, 0.5+0.5*diffuse, 1.0);    
    frag_color = vec4(linear_depth, linear_depth, linear_depth, 1.0);
    //frag_color = vec4(clipSpacePoint.x, clipSpacePoint.y, clipSpacePoint.z, 1.0);
}

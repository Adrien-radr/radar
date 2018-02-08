#version 400

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;


out int gl_PrimitiveID;

void main() {

    gl_Position = p0;
    gl_PrimitiveID = 0;
    EmitVertex();
    gl_Position = p1;
    gl_PrimitiveID = 0;
    EmitVertex();
    gl_Position = p2;
    gl_PrimitiveID = 0;
    EmitVertex();
    EndPrimitive();

    mat4 InvProjMatrix = inverse(ProjMatrix);
    mat4 InvViewMatrix = inverse(ViewMatrix);

    p0.x = max(-0.8,min(0.8, p0.x));
    p0.y = max(-0.8,min(0.8, p0.y));
    p0.z = 0.01;

    p0 = InvProjMatrix * p0;

    #if 0
    if(p0.y < 0.0)
        p0.z = 0.1;
    else
        p0.z = 1;

    p0 = InvViewMatrix * p0;
    #endif
        p0.z = 10;
    p0 = InvViewMatrix * p0;

    gl_Position = p0;
    gl_PrimitiveID = 1;
    EmitVertex();

    p1.x = max(-0.8,min(0.8, p1.x));
    p1.y = max(-0.8,min(0.8, p1.y));
    p1.z = 0.01;

    p1 = InvProjMatrix * p1;

    #if 0
    if(p1.y < 0.0)
        p1.z = 0.1;
    else
        p1.z = 1;

    #endif
        p1.z = 10;
    p1 = InvViewMatrix * p1;
    gl_Position = p1;
    gl_PrimitiveID = 1;
    EmitVertex();

    p2.x = max(-0.8,min(0.8, p2.x));
    p2.y = max(-0.8,min(0.8, p2.y));
    p2.z = 0.01;

    p2 = InvProjMatrix * p2;

    #if 0
    if(p2.y < 0.0)
        p2.z = 0.1;
    else
        p2.z = 1;
        #endif

        p2.z = 10;

    p2 = InvViewMatrix * p2;
    gl_Position = p2;
    gl_PrimitiveID = 1;
    EmitVertex();

    EndPrimitive();
}

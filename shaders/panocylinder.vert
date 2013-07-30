// panocylinder.vert

attribute vec3 vPosition;
attribute vec2 vTexCoord;

varying vec2 vfTexCoord;

uniform float vMove;
uniform mat4 mvmtx;
uniform mat4 prmtx;

void main()
{
    vfTexCoord = vTexCoord;
    vec3 outPos = vPosition;
    outPos.y += vMove;
    gl_Position = prmtx * mvmtx * vec4(outPos, 1.0);
}

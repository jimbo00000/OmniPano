// pano.vert

attribute vec4 vPosition;
attribute vec4 vTex;

varying vec2 vfTex;

uniform mat4 mvmtx;
uniform mat4 prmtx;
uniform int useSphereGeometry;

void main()
{
    vfTex = vTex.xy;

    if (useSphereGeometry == 1)
    {
        float x = vTex.y - 0.5;
        float sphereFactor = 2.0 * sqrt(0.25 - abs(x*x));
        vPosition.x *= sphereFactor;
        vPosition.z *= sphereFactor;
    }

    gl_Position = prmtx * mvmtx * vPosition;
}

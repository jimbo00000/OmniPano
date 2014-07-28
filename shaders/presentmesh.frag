// presentmesh.frag

uniform float fboScale;
uniform sampler2D fboTex;

varying vec2 vfTexR;
varying vec2 vfTexG;
varying vec2 vfTexB;
varying float vfColor;

void main()
{
    vec2 tc = vfTexG;
    tc.y = 1.0 - tc.y;
    tc *= fboScale;
    gl_FragColor = vfColor * texture2D(fboTex, tc);
}

// panocylinder.frag

varying vec2 vfTexCoord;

uniform sampler2D texImage;
uniform vec2 texOff;
uniform float vEyeYaw;

void main()
{
    float turn = vEyeYaw - floor(vEyeYaw);
    gl_FragColor = texture2D(texImage, vfTexCoord + texOff + vec2(turn,0));
}

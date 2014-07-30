// basictex.frag

varying vec2 vfTex;

uniform sampler2D texImage;

void main()
{
    gl_FragColor = texture2D(texImage, vfTex);
}

// basictex.frag

varying vec3 vfColor;

uniform sampler2D texImage;

void main()
{
    //gl_FragColor = vec4(vfColor, 1.0);
    gl_FragColor = texture2D(texImage, vfColor);
}

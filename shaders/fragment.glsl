#version 450

layout (location = 0) out vec4 FragColor;

// TODO:
// interpolation
// magnitude
// mandelbrot set checking

void main()
{
    // gl_FragCoord gives us pixel coordinates directly
    int x = int(gl_FragCoord.x);
    int y = int(gl_FragCoord.y);
    
    // XOR pattern: if (x + y) is even -> black, if odd -> white
    float checker = float((x + y) % 2);
    
    FragColor = vec4(checker, checker, checker, 1.0);
}

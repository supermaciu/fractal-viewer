#version 450

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) readonly buffer StorageBuffer {
    vec2 resolution;
};

void interpolate(in vec2 screen, out vec2 world) {
    world.x = 4.0 * screen.x / resolution.x - 2.0;
    world.y = 2.0 * screen.y / resolution.y - 1.0;
}

const int max_iter = 1000;
void mandelbrot(in vec2 c, out vec4 color) {
    double fb = 0;
    double fa = 0;
    double tempa;
    double tempb;

    int i;
    for (i = 1; i < max_iter; i++) {
        tempa = fa*fa - fb*fb + c.x;
        tempb = 2*fa*fb + c.y;
        fa = tempa;
        fb = tempb;

        if (distance(vec2(0, 0), vec2(fa, fb)) > 2) break;
    }

    float x = 5*log(i+1)/i;
    color = vec4(x, x, x, 1.0);
}

void main() {
    vec2 screen = gl_FragCoord.xy;
    vec2 world;
    interpolate(screen, world);
    
    vec4 color;
    mandelbrot(world, color);;

    FragColor = color;
}
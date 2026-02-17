#version 450

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) readonly buffer StorageBuffer {
    vec2 resolution;
};

void interpolate(in vec2 screen, out vec2 world) {
    world.x = 4.0 * screen.x / resolution.x - 2.0;
    world.y = 2.0 * screen.y / resolution.y - 1.0;
}

void mandelbrot(in vec2 c, out bool colored) {
    float fa = 0;
    float fb = 0;
    float tempa;
    float tempb;

    int i;
    for (i = 0; i < 1000; i++) {
        tempa = fa*fa - fb*fb + c.x;
        tempb = 2*fa*fb + c.y;
        fa = tempa;
        fb = tempb;

        if (distance(vec2(0, 0), vec2(fa, fb)) > 2) break;
    }

    colored = (i > 800);
}

void main() {
    vec2 screen = gl_FragCoord.xy;
    vec2 world;
    interpolate(screen, world);

    bool colored;
    mandelbrot(world, colored);
    
    FragColor = vec4(!colored, !colored, !colored, 1.0);
}
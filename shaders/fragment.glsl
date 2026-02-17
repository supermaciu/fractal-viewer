#version 450

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) readonly buffer StorageBuffer {
    dvec2 resolution;
    double zoomAmount;
    double screenMouseX;
    double screenMouseY;
};

void interpolate(in dvec2 screen, out dvec2 world) {
    double zoomX = 4*zoomAmount;
    double zoomY = 2*zoomAmount;

    world.x = zoomX * screen.x / resolution.x - zoomX/2.0;
    world.y = zoomY * screen.y / resolution.y - zoomY/2.0;
}

const int max_iter = 1000;
void mandelbrot(in dvec2 c, out vec4 color) {
    double fb = 0;
    double fa = 0;
    double tempa;
    double tempb;

    int i;
    for (i = 1; i < max_iter; i++) {
        if (distance(dvec2(0, 0), dvec2(fa, fb)) > 2) break;

        tempa = fa*fa - fb*fb + c.x;
        tempb = 2.0*fa*fb + c.y;
        fa = tempa;
        fb = tempb;
    }

    double x = 5.0*log(i+1)/i;
    color = vec4(x, x, x, 1.0);
}

void main() {
    dvec2 screen = gl_FragCoord.xy;
    dvec2 world;
    interpolate(screen, world);
    
    vec4 color;
    mandelbrot(world, color);

    FragColor = color;
}
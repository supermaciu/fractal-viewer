#version 450

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) readonly buffer StorageBuffer {
    dvec2 resolution;
    dvec2 offset;
    double zoomAmount;
};

void interpolate(in dvec2 pixel, out dvec2 point) {
    dvec2 zoom = dvec2(4*zoomAmount, 2*zoomAmount);
    point = pixel / resolution - 0.5;
    // point *= zoom;
    // point += offset;
    point = fma(point, zoom, offset);
}

vec3 pal(in float t) {
    vec3 a = vec3(0.5,0.5,0.5), b = vec3(0.5,0.5,0.5), c = vec3(1.0,1.0,1.0), d = vec3(0.0,0.10,0.20);
    return a + b*cos(6.28318*(c*t+d));
}

int max_iter = 1000;
void mandelbrot(in dvec2 c, out vec4 color) {
    double fb = 0;
    double fa = 0;
    double tempa;
    double tempb;

    int i;
    double d;
    for (i = 0; i < max_iter; i++) {
        d = distance(dvec2(0, 0), dvec2(fa, fb));
        if (d > 2) break;

        tempa = fa*fa - fb*fb + c.x;
        tempb = 2.0*fa*fb + c.y;
        fa = tempa;
        fb = tempb;
    }

    double x = 5.0*log(i+1)/i;
    color = vec4(vec3(x), 1.0);
}

void main() {
    dvec2 pixel = gl_FragCoord.xy;
    dvec2 point;
    interpolate(pixel, point);
    
    vec4 color;
    mandelbrot(point, color);

    FragColor = color;
}
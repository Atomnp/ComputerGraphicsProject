#pragma once
//#include"core.h"
#include "GL/glu.h"
#include <utility>
//#include "GL/glew.h"

// template <class T>
// static void swap(T &a, T &b) {
//    auto temp = a;
//    a = b;
//    b = temp;
//}

struct framebuffer {
    bool *grid;
    vec3 *color;
    uint32_t x_size, y_size;
    framebuffer(const uint32_t &x, const uint32_t &y) : x_size(x), y_size(y) {
        grid = new bool[x_size * y_size];
        color = new vec3[x_size * y_size];
        clear();
    }

    ~framebuffer() {
        delete[] grid;
        delete[] color;
    }

    inline void clear() {
        for (uint32_t x = 0; x < x_size * y_size; ++x) {
            grid[x] = false;
            color[x] = 0;
        }
    }
};

struct engine {

    framebuffer *fboCPU;

    typedef void (*TransFunc)();

    engine(const uint32_t &x, const uint32_t &y) {
        fboCPU = new framebuffer(x, y);
        glViewport(0, 0, fboCPU->x_size, fboCPU->y_size);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, fboCPU->x_size, 0.0, fboCPU->y_size);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    ~engine() { delete fboCPU; }

    void clear() {
        glClear(GL_COLOR_BUFFER_BIT);
        fboCPU->clear();
    }

    void draw() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_POINTS);
        for (GLint x = 0; x < fboCPU->x_size; ++x) {
            for (GLint y = 0; y < fboCPU->y_size; ++y) {
                if (fboCPU->grid[x + y * fboCPU->x_size]) {
                    vec3 &c = fboCPU->color[x + y * fboCPU->x_size];
                    glColor3f(c.x, c.y, c.z);
                    glVertex2i(x, y);
                }
            }
        }
        glEnd();
        glFlush();
    }

    void putpixel(int x, int y, const vec3 &col = 1) {
        if (x < fboCPU->x_size && x >= 0 && y < fboCPU->y_size && y >= 0) {
            fboCPU->color[x + y * fboCPU->x_size] = col;
            fboCPU->grid[x + y * fboCPU->x_size] = true;
        }
    }

    void putpixel_adjusted(int x, int y, const vec3_T<float> &col = 0) {
        putpixel(x + fboCPU->x_size / 2, y + fboCPU->y_size / 2, col);
    }

    void draw_bresenham_adjusted(int x1, int y1, int x2, int y2,
                                 const vec3 &color = vec3(1, 0, 0)) {
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);

        int lx = x2 > x1 ? 1 : -1;
        int ly = y2 > y1 ? 1 : -1;

        int x = x1, y = y1;
        bool changed = false;

        if (dx <= dy) {
            changed = true;
            std::swap(dx, dy);
            std::swap(lx, ly);
            std::swap(x, y);
        }
        int p = 2 * dy - dx;
        for (int k = 0; k <= dx; k++) {
            if (!changed)
                putpixel_adjusted(x, y, color);
            else
                putpixel_adjusted(y, x, color);

            if (p < 0) {
                x += lx;
                p += 2 * dy;
            } else {
                x += lx;
                y += ly;
                p += 2 * dy - 2 * dx;
            }
        }
    }

    void
    drawLines(const std::vector<vec2_T<int>> &points,
              const vec3_T<float> &color = 1,
              const std::vector<uint32_t> &indices = std::vector<uint32_t>()) {
        if (indices.empty()) {
            for (size_t i = 0; i < points.size(); i += 2) {
                draw_bresenham_adjusted(points[i].x, points[i].y,
                                        points[i + 1].x, points[i + 1].y,
                                        color);
            }
            return;
        }

        for (size_t i = 0; i < indices.size(); i += 2) {
            draw_bresenham_adjusted(points[indices[i]].x, points[indices[i]].y,
                                    points[indices[i + 1]].x,
                                    points[indices[i + 1]].y, color);
        }
    }

    void drawTraingles(const std::vector<vec4> cube,
                       const std::vector<uint32_t> &indices, vec3 dir) {

        for (int i = 0; i < indices.size(); i += 3) {
            /* subtracting 1 because indices are 1 indexd not zero indexed */
            vec4 vertex1 = cube[indices[i] - 1];
            vec4 vertex2 = cube[indices[i + 1] - 1];
            vec4 vertex3 = cube[indices[i + 2] - 1];

            /* this if condition is completely for testing purpose, it helps to
             * check how each traingle are drawn */
            int filter = 1;
            if (filter or (indices[i] == 3 and indices[i + 1] == 8 and
                           indices[i + 2] == 4)) {

                /* camera vanda paxadi paro vane aile lai puraai traingle nai
                 * display nagarne */
                if (vertex1.z / vertex1.w > 1 or vertex3.z / vertex3.w > 1 or
                    vertex2.z / vertex2.w > 1) {
                    continue;
                }

                vec3 normal = vec3::cross(vec3(vertex2 - vertex1),
                                          vec3(vertex3 - vertex1));

                /* calculate the normal here and if the normal and camera
                 * direction dot product gives positive the trangle should not
                 * be drawn */
                auto temp = vec3::dot(normal, dir);
                if (temp > 0) {
                    continue;
                }

                /*  for testing */
                if (!filter) {
                    std::cout << "normal=" << normal;
                    std::cout << "dot=" << temp << std::endl;
                    std::cout << "camera dir=" << dir;
                }

                assert(vertex1.w != 0 and vertex2.w != 0 and vertex3.w != 0);

                float window_width = 640;
                float window_height = 480;

                draw_bresenham_adjusted(
                    (int)round(((vertex1.x / vertex1.w) + 1) * window_width /
                                   2 -
                               window_width / 2),
                    (int)round(((vertex1.y / vertex1.w) + 1) * window_height /
                                   2 -
                               window_height / 2),
                    (int)round(((vertex2.x / vertex2.w) + 1) * window_width /
                                   2 -
                               window_width / 2),
                    (int)round(((vertex2.y / vertex2.w) + 1) * window_height /
                                   2 -
                               window_height / 2));

                draw_bresenham_adjusted(
                    (int)round(((vertex2.x / vertex2.w) + 1) * window_width /
                                   2 -
                               window_width / 2),
                    (int)round(((vertex2.y / vertex2.w) + 1) * window_height /
                                   2 -
                               window_height / 2),
                    (int)round(((vertex3.x / vertex3.w) + 1) * window_width /
                                   2 -
                               window_width / 2),
                    (int)round(((vertex3.y / vertex3.w) + 1) * window_height /
                                   2 -
                               window_height / 2));

                draw_bresenham_adjusted(
                    (int)round(((vertex1.x / vertex1.w) + 1) * window_width /
                                   2 -
                               window_width / 2),
                    (int)round(((vertex1.y / vertex1.w) + 1) * window_height /
                                   2 -
                               window_height / 2),
                    (int)round(((vertex3.x / vertex3.w) + 1) * window_width /
                                   2 -
                               window_width / 2),
                    (int)round(((vertex3.y / vertex3.w) + 1) * window_height /
                                   2 -
                               window_height / 2));
            }
        }
    }

    void drawLinestrip(
        const std::vector<vec2_T<int>> &points, const vec3_T<float> &color = 1,
        const std::vector<uint32_t> &indices = std::vector<uint32_t>()) {
        for (size_t i = 0; i < points.size(); i++) {
            draw_bresenham_adjusted(points[i].x, points[i].y, points[i + 1].x,
                                    points[i + 1].y, color);
        }
    }

    //     void drawTriangles3d(const std::vector<vec3> &points, const vec3
    //     &color = 1, const std::vector<uint32_t> &indices =
    //     std::vector<uint32_t>()) {
    //         if (indices.empty()) {
    //             for (size_t i = 0; i < points.size(); i += 3) {
    //                 indices.push_back(i);
    //                 indices.push_back(i + 1);
    //                 indices.push_back(i + 2);
    //             }
    //             return;
    //         }
    //         for (size_t i = 0; i < indices.size(); i += 2) {
    //             draw_bresenham_adjusted(points[indices[i]].x,
    //             points[indices[i]].y, points[indices[i + 1]].x,
    //             points[indices[i + 1]].y, color);
    //         }
    //     }
};
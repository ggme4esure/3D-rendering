
#include <windows.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

#define PI 3.14159265358979323846
#define THING1 800
#define THING2 600

// Rotation angles.
float gAngleX = 0.4f;
float gAngleY = 0.6f;
float gAngleZ = 0.2f;

// Window size and pixel buffer.
int w = THING1;
int h = THING2;
vector<uint32_t> pxls;

float fps_global = 0.0f;
int temp_counter_thing = 0;
float temp_timer_thing = 0.0f;

// Color structure.
struct col {
    uint8_t r, g, b;
};

// Pack color into a single 32-bit pixel value.
uint32_t p(col c) {
    return (c.r << 16) | (c.g << 8) | c.b;
}

// Blend between two colors using a factor from 0 to 1.
col mix(col c1, col c2, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    col result;
    result.r = (uint8_t)(c1.r + (c2.r - c1.r) * t);
    result.g = (uint8_t)(c1.g + (c2.g - c1.g) * t);
    result.b = (uint8_t)(c1.b + (c2.b - c1.b) * t);
    return result;
}

// Map depth to a color for a simple depth effect.
col depthCol(float z) {
    float t = (z - 2.0f) / (5.5f - 2.0f);
    col a; a.r = 255; a.g = 120; a.b = 180;   // pinkish near
    col b; b.r = 80;  b.g = 20;  b.b = 60;    // dark pink far
    return mix(a, b, t);
}

// Rotate around X axis.
void doRotation1(float* x, float* y, float* z, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float newY = (*y) * c - (*z) * s;
    float newZ = (*y) * s + (*z) * c;
    *y = newY;
    *z = newZ;
}

// Rotate around Y axis.
void doRotation2(float* x, float* y, float* z, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float newX = (*x) * c + (*z) * s;
    float newZ = -(*x) * s + (*z) * c;
    *x = newX;
    *z = newZ;
}

// Rotate around Z axis.
void doRotation3(float* x, float* y, float* z, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float newX = (*x) * c - (*y) * s;
    float newY = (*x) * s + (*y) * c;
    *x = newX;
    *y = newY;
}

// Fill the buffer with a simple gradient background.
void cls() {
    float cx = w / 2.0f;
    float cy = h / 2.0f;
    float md = sqrt(cx * cx + cy * cy);
    if (md < 1.0f) md = 1.0f;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            float dx = i - cx;
            float dy = j - cy;
            float d = sqrt(dx * dx + dy * dy);
            float t = d / md;

            int rr = (int)(20 + (6 - 20) * t);
            int gg = (int)(18 + (4 - 18) * t);
            int bb = (int)(30 + (8 - 30) * t);

            if (i % 50 == 0 || j % 50 == 0) {
                rr = rr + 8; if (rr > 255) rr = 255;
                gg = gg + 8; if (gg > 255) gg = 255;
                bb = bb + 14; if (bb > 255) bb = 255;
            }

            col tmp; tmp.r = rr; tmp.g = gg; tmp.b = bb;
            pxls[j * w + i] = p(tmp);
        }
    }
}

// Draw a line between two points with color blending.
void line(int x0, int y0, int x1, int y1, col color1, col color2) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx, sy;
    if (x0 < x1) sx = 1; else sx = -1;
    if (y0 < y1) sy = 1; else sy = -1;
    int err = dx - dy;

    int totalSteps = dx;
    if (dy > dx) totalSteps = dy;
    int currentStep = 0;

    loop_start:
    if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) {
        float tt;
        if (totalSteps == 0) tt = 0.0f;
        else tt = (float)currentStep / (float)totalSteps;
        col cc = mix(color1, color2, tt);
        pxls[y0 * w + x0] = p(cc);
    }

    if (x0 == x1 && y0 == y1) goto loop_end;

    {
        int e2 = 2 * err;
        if (e2 > -dy) {
            err = err - dy;
            x0 = x0 + sx;
        }
        if (e2 < dx) {
            err = err + dx;
            y0 = y0 + sy;
        }
    }
    currentStep++;
    goto loop_start;

    loop_end:
    return;
}

// Draw a small square dot at a point.
void dot(int cx, int cy, int s, col c) {
    int r = s / 2;
    uint32_t packed = p(c);
    for (int a = -r; a <= r; a++) {
        for (int b = -r; b <= r; b++) {
            int xx = cx + b;
            int yy = cy + a;
            if (xx >= 0 && xx < w && yy >= 0 && yy < h)
                pxls[yy * w + xx] = packed;
        }
    }
}

// ============================================================
// ROCKET / PHALLIC SHAPE GEOMETRY
// Built from: cylinder body + hemisphere tip + two spheres at base
// ============================================================

// Geometry parameters
#define SLICES 16        // circumference subdivisions
#define BODY_RINGS 6     // rings along the shaft
#define TIP_RINGS 5      // rings on the hemisphere tip
#define BALL_RINGS 4     // rings on each sphere
#define BALL_SLICES 10   // circumference subdivisions for spheres

// Shape dimensions
const float BODY_RADIUS = 0.45f;
const float BODY_LENGTH = 2.4f;    // shaft length along Y axis
const float TIP_RADIUS = 0.45f;    // same as body radius at junction
const float TIP_HEIGHT = 0.9f;     // hemisphere height
const float BALL_RADIUS = 0.5f;    // sphere radius
const float BALL_SPREAD = 0.55f;   // horizontal offset for spheres
const float BALL_Y_OFFSET = -0.15f; // vertical offset for spheres from base

// Storage for all vertices and edges
vector<float> allX, allY, allZ;
struct Edge { int a, b; };
vector<Edge> allEdges;

int totalVerts = 0;
int totalEdges = 0;

void addVert(float x, float y, float z) {
    allX.push_back(x);
    allY.push_back(y);
    allZ.push_back(z);
    totalVerts++;
}

void addEdge(int a, int b) {
    Edge e; e.a = a; e.b = b;
    allEdges.push_back(e);
    totalEdges++;
}

void buildGeometry() {
    allX.clear(); allY.clear(); allZ.clear();
    allEdges.clear();
    totalVerts = 0;
    totalEdges = 0;

    // ---- CYLINDER BODY ----
    // Body goes from y = -BODY_LENGTH/2 to y = +BODY_LENGTH/2
    // Tip is at the top (positive Y)
    float bodyBottom = -BODY_LENGTH / 2.0f;
    float bodyTop = BODY_LENGTH / 2.0f;

    int bodyStartIdx = totalVerts;

    for (int ring = 0; ring <= BODY_RINGS; ring++) {
        float t = (float)ring / (float)BODY_RINGS;
        float y = bodyBottom + t * BODY_LENGTH;

        for (int s = 0; s < SLICES; s++) {
            float angle = (float)s / (float)SLICES * 2.0f * PI;
            float x = BODY_RADIUS * cos(angle);
            float z = BODY_RADIUS * sin(angle);
            addVert(x, y, z);
        }
    }

    // Connect body edges — ring connections (horizontal)
    for (int ring = 0; ring <= BODY_RINGS; ring++) {
        int ringBase = bodyStartIdx + ring * SLICES;
        for (int s = 0; s < SLICES; s++) {
            int next = (s + 1) % SLICES;
            addEdge(ringBase + s, ringBase + next);
        }
    }

    // Connect body edges — vertical (along length)
    for (int ring = 0; ring < BODY_RINGS; ring++) {
        int ringBase = bodyStartIdx + ring * SLICES;
        int nextRingBase = bodyStartIdx + (ring + 1) * SLICES;
        for (int s = 0; s < SLICES; s++) {
            addEdge(ringBase + s, nextRingBase + s);
        }
    }

    // ---- HEMISPHERE TIP (on top of body) ----
    int tipStartIdx = totalVerts;

    // The first ring of the tip connects to the last ring of the body
    for (int ring = 1; ring <= TIP_RINGS; ring++) {
        float t = (float)ring / (float)TIP_RINGS;
        // t goes from 0 (base) to 1 (apex)
        // Hemisphere: radius shrinks, height increases
        float phi = t * PI / 2.0f; // 0 to 90 degrees
        float ringRadius = TIP_RADIUS * cos(phi);
        float y = bodyTop + TIP_HEIGHT * sin(phi);

        if (ring == TIP_RINGS) {
            // Apex: single point
            addVert(0.0f, y, 0.0f);
        } else {
            for (int s = 0; s < SLICES; s++) {
                float angle = (float)s / (float)SLICES * 2.0f * PI;
                float x = ringRadius * cos(angle);
                float z = ringRadius * sin(angle);
                addVert(x, y, z);
            }
        }
    }

    // Connect tip ring edges (horizontal circles)
    for (int ring = 1; ring < TIP_RINGS; ring++) {
        int ringBase = tipStartIdx + (ring - 1) * SLICES;
        for (int s = 0; s < SLICES; s++) {
            int next = (s + 1) % SLICES;
            addEdge(ringBase + s, ringBase + next);
        }
    }

    // Connect body top ring to first tip ring
    int bodyTopRingBase = bodyStartIdx + BODY_RINGS * SLICES;
    int firstTipRingBase = tipStartIdx;
    for (int s = 0; s < SLICES; s++) {
        addEdge(bodyTopRingBase + s, firstTipRingBase + s);
    }

    // Connect vertical edges between tip rings
    for (int ring = 1; ring < TIP_RINGS - 1; ring++) {
        int ringBase = tipStartIdx + (ring - 1) * SLICES;
        int nextRingBase = tipStartIdx + ring * SLICES;
        for (int s = 0; s < SLICES; s++) {
            addEdge(ringBase + s, nextRingBase + s);
        }
    }

    // Connect last tip ring to apex
    int apexIdx = tipStartIdx + (TIP_RINGS - 2) * SLICES + SLICES; // the single apex vertex
    // Wait, let's recalculate. For ring=1..(TIP_RINGS-1), we add SLICES verts each.
    // For ring=TIP_RINGS, we add 1 vert (apex).
    // So apex index = tipStartIdx + (TIP_RINGS - 1) * SLICES
    apexIdx = tipStartIdx + (TIP_RINGS - 1) * SLICES;
    int lastTipRingBase = tipStartIdx + (TIP_RINGS - 2) * SLICES;
    for (int s = 0; s < SLICES; s++) {
        addEdge(lastTipRingBase + s, apexIdx);
    }

    // ---- TWO SPHERES AT THE BASE (left and right) ----
    float ballCenterY = bodyBottom + BALL_Y_OFFSET;

    for (int side = 0; side < 2; side++) {
        float offsetX = (side == 0) ? -BALL_SPREAD : BALL_SPREAD;
        int sphereStartIdx = totalVerts;

        // Generate sphere vertices using latitude/longitude
        // Bottom pole
        addVert(offsetX, ballCenterY - BALL_RADIUS, 0.0f);
        int bottomPoleIdx = sphereStartIdx;

        // Middle rings
        for (int ring = 1; ring < BALL_RINGS; ring++) {
            float phi = PI * (float)ring / (float)BALL_RINGS; // 0 to PI
            float ringY = ballCenterY - BALL_RADIUS * cos(phi);
            float ringR = BALL_RADIUS * sin(phi);

            for (int s = 0; s < BALL_SLICES; s++) {
                float theta = (float)s / (float)BALL_SLICES * 2.0f * PI;
                float x = offsetX + ringR * cos(theta);
                float z = ringR * sin(theta);
                addVert(x, ringY, z);
            }
        }

        // Top pole
        addVert(offsetX, ballCenterY + BALL_RADIUS, 0.0f);
        int topPoleIdx = totalVerts - 1;

        // Connect sphere edges — ring connections
        for (int ring = 1; ring < BALL_RINGS; ring++) {
            int ringBase = sphereStartIdx + 1 + (ring - 1) * BALL_SLICES;
            for (int s = 0; s < BALL_SLICES; s++) {
                int next = (s + 1) % BALL_SLICES;
                addEdge(ringBase + s, ringBase + next);
            }
        }

        // Connect vertical edges between rings
        for (int ring = 1; ring < BALL_RINGS - 1; ring++) {
            int ringBase = sphereStartIdx + 1 + (ring - 1) * BALL_SLICES;
            int nextRingBase = sphereStartIdx + 1 + ring * BALL_SLICES;
            for (int s = 0; s < BALL_SLICES; s++) {
                addEdge(ringBase + s, nextRingBase + s);
            }
        }

        // Connect poles
        int firstRingBase = sphereStartIdx + 1;
        int lastRingBase = sphereStartIdx + 1 + (BALL_RINGS - 2) * BALL_SLICES;
        for (int s = 0; s < BALL_SLICES; s++) {
            addEdge(bottomPoleIdx, firstRingBase + s);
            addEdge(lastRingBase + s, topPoleIdx);
        }
    }
}

// Project and draw the shape on screen.
void drawTheShape() {
    vector<int> sx(totalVerts), sy(totalVerts);
    vector<col> vertexColors(totalVerts);
    vector<bool> ok(totalVerts, false);

    for (int i = 0; i < totalVerts; i++) {
        float vx = allX[i];
        float vy = allY[i];
        float vz = allZ[i];

        doRotation1(&vx, &vy, &vz, gAngleX);
        doRotation2(&vx, &vy, &vz, gAngleY);
        doRotation3(&vx, &vy, &vz, gAngleZ);

        vz = vz + 4.0f; // push further back for larger model

        if (vz > 0.1f) {
            float fov = min(w, h) * 0.65f;
            sx[i] = (int)(vx * fov / vz + w / 2.0f);
            sy[i] = (int)(-vy * fov / vz + h / 2.0f);
            vertexColors[i] = depthCol(vz);
            ok[i] = true;
        } else {
            ok[i] = false;
        }
    }

    // Draw all edges
    for (int i = 0; i < totalEdges; i++) {
        int a = allEdges[i].a;
        int b = allEdges[i].b;
        if (ok[a] && ok[b]) {
            line(sx[a], sy[a], sx[b], sy[b], vertexColors[a], vertexColors[b]);
        }
    }

    // Draw dots at vertices
    for (int i = 0; i < totalVerts; i++) {
        if (ok[i]) {
            col white; white.r = 255; white.g = 255; white.b = 255;
            dot(sx[i], sy[i], 3, white);
        }
    }
}

// Window callback.
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    } else if (msg == WM_SIZE) {
        int nw = LOWORD(lp);
        int nh = HIWORD(lp);
        if (nw > 0 && nh > 0) {
            w = nw;
            h = nh;
            pxls.assign(w * h, 0);
        }
        return 0;
    } else if (msg == WM_ERASEBKGND) {
        return 1;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// Main entry.
int main() {
    HINSTANCE hInst = GetModuleHandle(NULL);

    const char* cn = "rocketclass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = cn;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    RECT r = {0, 0, THING1, THING2};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindowEx(0, cn,
        "3D Rocket Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInst, NULL);

    if (hwnd == NULL) {
        cout << "CRITICAL ERROR: Failed to create window!" << endl;
        return -1;
    }

    ShowWindow(hwnd, SW_SHOW);
    pxls.resize(w * h);

    // Build the geometry once
    buildGeometry();

    auto t1 = chrono::high_resolution_clock::now();

    MSG msg = {};
    int running = 1;
    while (running == 1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = 0;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (running != 1) break;

        auto t2 = chrono::high_resolution_clock::now();
        chrono::duration<float> diff = t2 - t1;
        t1 = t2;
        float dt = diff.count();
        if (dt > 0.1f) dt = 0.1f;

        temp_counter_thing++;
        temp_timer_thing += dt;
        if (temp_timer_thing >= 0.5f) {
            fps_global = temp_counter_thing / temp_timer_thing;
            temp_counter_thing = 0;
            temp_timer_thing = 0.0f;
        }

        gAngleX += 0.5f * dt;
        gAngleY += 0.7f * dt;
        gAngleZ += 0.3f * dt;

        cls();
        drawTheShape();

        HDC hdc = GetDC(hwnd);

        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        StretchDIBits(hdc, 0, 0, w, h, 0, 0, w, h,
            pxls.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);

        SetBkMode(hdc, TRANSPARENT);

        HFONT font = CreateFont(16, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        SetTextColor(hdc, RGB(255, 120, 180));
        const char* title = "3D ENGINE // ROCKET SHAPE";
        TextOut(hdc, 20, 20, title, strlen(title));

        SetTextColor(hdc, RGB(150, 160, 180));
        const char* subtitle = "Language: C++ (Native Win32 API)";
        TextOut(hdc, 20, 42, subtitle, strlen(subtitle));

        char buf[128];
        SetTextColor(hdc, RGB(245, 245, 245));
        sprintf(buf, "FPS: %.1f", fps_global);
        TextOut(hdc, 20, 64, buf, strlen(buf));

        SetTextColor(hdc, RGB(150, 160, 180));
        sprintf(buf, "Angles: X:%.2f  Y:%.2f  Z:%.2f", gAngleX, gAngleY, gAngleZ);
        TextOut(hdc, 20, 86, buf, strlen(buf));

        sprintf(buf, "Geometry: %d Vertices | %d Edges", totalVerts, totalEdges);
        TextOut(hdc, 20, 108, buf, strlen(buf));

        SelectObject(hdc, oldFont);
        DeleteObject(font);
        ReleaseDC(hwnd, hdc);

        Sleep(8);
    }

    return 0;
}

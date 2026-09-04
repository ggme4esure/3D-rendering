
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

// Rotation .
float gAngleX = 0.4f; 
float gAngleY = 0.6f;   
float gAngleZ = 0.2f; 

//  window size pixel buffer.
int w = THING1;
int h = THING2;
vector<uint32_t> pxls;

float fps_global = 0.0f;
int temp_counter_thing = 0;
float temp_timer_thing = 0.0f;

//  color structure.
struct col {
    uint8_t r, g, b;
};

//  color into a single 32-bit pixel value.
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
    float t = (z - 2.5f) / (4.5f - 2.5f);
    col a; a.r = 0; a.g = 240; a.b = 255;
    col b; b.r = 50; b.g = 15; b.b = 95;
    return mix(a, b, t);
}

// Rotate around X 
void doRotation1(float* x, float* y, float* z, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float newY = (*y) * c - (*z) * s;
    float newZ = (*y) * s + (*z) * c;
    *y = newY;
    *z = newZ;
}

// Rotate around the Y 
void doRotation2(float* x, float* y, float* z, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float newX = (*x) * c + (*z) * s;
    float newZ = -(*x) * s + (*z) * c;
    *x = newX;
    *z = newZ;
}

// Rotate around the Z axis.
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
            int gg = (int)(28 + (8 - 28) * t);
            int bb = (int)(44 + (12 - 44) * t);
            
            if (i % 50 == 0 || j % 50 == 0) {
                rr = rr + 10; if (rr > 255) rr = 255;
                gg = gg + 14; if (gg > 255) gg = 255;
                bb = bb + 22; if (bb > 255) bb = 255;
            }
            
            col tmp; tmp.r = rr; tmp.g = gg; tmp.b = bb;
            pxls[j * w + i] = p(tmp);
        }
    }
}

// Draw a line between two points with color .
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

float cubeX[8] = {-1, 1, 1,-1,-1, 1, 1,-1};
float cubeY[8] = {-1,-1, 1, 1,-1,-1, 1, 1};
float cubeZ[8] = {-1,-1,-1,-1, 1, 1, 1, 1};

int edges[24] = {
    0,1, 1,2, 2,3, 3,0,
    4,5, 5,6, 6,7, 7,4,
    0,4, 1,5, 2,6, 3,7
};

// Project and draw the cube on screen.
void drawTheCube() {
    int sx[8], sy[8];
    col vertexColors[8];
    bool ok[8];
    
    for (int i = 0; i < 8; i++) {
        float vx = cubeX[i];
        float vy = cubeY[i];
        float vz = cubeZ[i];
        
        doRotation1(&vx, &vy, &vz, gAngleX);
        doRotation2(&vx, &vy, &vz, gAngleY);
        doRotation3(&vx, &vy, &vz, gAngleZ);
        
        vz = vz + 3.5f;
        
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
    
    for (int i = 0; i < 12; i++) {
        int a = edges[i * 2];
        int b = edges[i * 2 + 1];
        if (ok[a] && ok[b]) {
            line(sx[a], sy[a], sx[b], sy[b], vertexColors[a], vertexColors[b]);
        }
    }
    
    for (int i = 0; i < 8; i++) {
        if (ok[i]) {
            col white; white.r = 255; white.g = 255; white.b = 255;
            dot(sx[i], sy[i], 5, white);
        }
    }
}

// window thing.
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

// Start window with loop.
int main() {
    HINSTANCE hInst = GetModuleHandle(NULL);
    
    const char* cn = "myclass123";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = cn;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
    
    RECT r = {0, 0, THING1, THING2};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindowEx(0, cn,

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
        drawTheCube();
        
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
        
        SetTextColor(hdc, RGB(0, 240, 255));
        const char* title = "3D ENGINE // SOFT-RASTERIZER";
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
        
        sprintf(buf, "Geometry: 8 Vertices | 12 Edges");
        TextOut(hdc, 20, 108, buf, strlen(buf));
        
        SelectObject(hdc, oldFont);
        DeleteObject(font);
        ReleaseDC(hwnd, hdc);
        
        Sleep(8);
    }
    
    return 0;
}

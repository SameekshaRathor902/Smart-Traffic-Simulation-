#include "renderer.h"
#include "constants.h"
#include <cmath>

//  DRAW HELPERS
// 3-bulb traffic light pole
void DrawTrafficLight(int x, int y, Color active) {
    //pole
    DrawRectangle(x + 8, y + 70, 4, 40, DARKGRAY);
    //housing
    DrawRectangle(x, y, 20, 68, BLACK);
    DrawRectangleLines(x, y, 20, 68, DARKGRAY);
    //bulbs
    DrawCircle(x+10, y+12, 7, (active.r==255 && active.g<50) ? RED   : ColorAlpha(RED,   0.25f));
    DrawCircle(x+10, y+34, 7, (active.g>200 && active.r>200) ? YELLOW: ColorAlpha(YELLOW,0.25f));
    DrawCircle(x+10, y+56, 7, (active.r<50 && active.g>200)  ? GREEN : ColorAlpha(GREEN, 0.25f));
}

//Dashed lines
void DrawDashedLine(int x1, int y1, int x2, int y2, int dashLen, Color c) {
    float dx = (float)(x2 - x1), dy = (float)(y2 - y1);
    float len = sqrtf(dx*dx + dy*dy);
    int   steps = (int)(len / (dashLen * 2));
    float nx = dx/len, ny = dy/len;
    for (int i = 0; i < steps; i++) {
        float sx = x1 + nx * i * dashLen * 2;
        float sy = y1 + ny * i * dashLen * 2;
        DrawLineEx({sx, sy}, {sx + nx*dashLen, sy + ny*dashLen}, 3, c);
    }
}

void DrawVehicle(const Vehicle& v) {
    DrawRectangleRec(v.rect, v.color);
    // windshield (sky blue rectengle in car)
    Color glass = ColorAlpha(SKYBLUE, 0.7f);
    if (v.direction == 0 || v.direction == 1) { //vertical
        DrawRectangle((int)v.rect.x+4, (int)v.rect.y+8, (int)v.rect.width-8, 10, glass);
    } else { //horizontal
        DrawRectangle((int)v.rect.x+8, (int)v.rect.y+4, 10, (int)v.rect.height-8, glass);
    }
    
    // ambulance cross
    if (v.isAmbulance) {
        int cx = (int)(v.rect.x + v.rect.width/2);
        int cy = (int)(v.rect.y + v.rect.height/2);
        DrawRectangle(cx-6, cy-2, 12, 4, RED);
        DrawRectangle(cx-2, cy-6, 4, 12, RED);
    }
}

//// UI spawn pannel in the top right corner
void DrawUI(int W, int selectedType) {
    int x = W - 200, y = 20;
    DrawRectangleRounded({(float)x,(float)y,178,110}, 0.1f, 4, {30,30,30,220});
    DrawText("SPAWN PANEL", x+18, y+10, 18, WHITE);
    DrawText("Type:", x+10, y+44, 15, LIGHTGRAY);
    DrawRectangle(x+70, y+42, 18, 18, selectedType==0 ? GREEN : GRAY);
    DrawText("Car",  x+94,  y+42, 15, WHITE);
    DrawRectangle(x+70, y+66, 18, 18, selectedType==1 ? GREEN : GRAY);
    DrawText("Amb",  x+94,  y+66, 15, WHITE);
    DrawText("Click a lane to spawn", x+10, y+92, 11, DARKGRAY);
}

//dynamic timers and live vehicle metrics.
void DrawHUD(const std::vector<Vehicle>& vehicles, IntersectionState state, float timer, float nsGreen, float ewGreen) {
    int ns = CountLane(vehicles,0)+CountLane(vehicles,1);
    int ew = CountLane(vehicles,2)+CountLane(vehicles,3);

    // background
    DrawRectangleRounded({10,10,200,120},0.1f,4,{0,0,0,160});
    DrawText(TextFormat("N/S: %d cars", ns), 20, 20, 18, WHITE);
    DrawText(TextFormat("E/W: %d cars", ew), 20, 44, 18, WHITE);

    DrawRectangle(20, 70, 160, 12, DARKGRAY);
    DrawRectangle(20, 70, (int)fminf(ns*8,160), 12, GREEN);
    DrawRectangle(20, 88, 160, 12, DARKGRAY);
    DrawRectangle(20, 88, (int)fminf(ew*8,160), 12, ORANGE);

    DrawText("NS", 186, 68, 12, GREEN);
    DrawText("EW", 186, 86, 12, ORANGE);
    DrawText(TextFormat("Timer: %.1fs", timer), 20, 106, 14, LIGHTGRAY);
}

void DrawScene(int W, int H, const std::vector<Vehicle>& vehicles, IntersectionState state, float timer, float nsGreen, float ewGreen, int selectedType) {
    BeginDrawing();
    ClearBackground({34, 85, 34, 255});

     // roads 
    DrawRectangle(ROAD_LEFT,  0,         ROAD_W, H, {60,60,60,255});
    DrawRectangle(0, ROAD_TOP, W,         ROAD_W, {60,60,60,255});

    // intersection box (slightly lighter)
    DrawRectangle(ROAD_LEFT, ROAD_TOP, ROAD_W, ROAD_W, {70,70,70,255});

    // road markings 
    // dashed centre lines (yellow)
    DrawDashedLine(RCX, 0,      RCX, ROAD_TOP,  20, {200,200,0,180});
    DrawDashedLine(RCX, ROAD_BOT, RCX, H,        20, {200,200,0,180});
    DrawDashedLine(0, RCY,      ROAD_LEFT, RCY,  20, {200,200,0,180});
    DrawDashedLine(ROAD_RIGHT, RCY, W, RCY,      20, {200,200,0,180});

    // stop lines (L shaped)
    DrawLineEx({(float)ROAD_LEFT,(float)STOP_S},{(float)RCX,(float)STOP_S},3,WHITE);
    DrawLineEx({(float)RCX,(float)STOP_N},{(float)ROAD_RIGHT,(float)STOP_N},3,WHITE);
    DrawLineEx({(float)STOP_E,(float)ROAD_TOP},{(float)STOP_E,(float)RCY},3,WHITE);
    DrawLineEx({(float)STOP_W,(float)RCY},{(float)STOP_W,(float)ROAD_BOT},3,WHITE);

    // zebra crosswalks (4 stripes each, placed outside intersection box)
    for (int i = 0; i < 4; i++) {
        int oy = i * 9;
        DrawRectangle(ROAD_LEFT+8, ROAD_TOP-36+oy, ROAD_W-16, 5, {255,255,255,140});
        DrawRectangle(ROAD_LEFT+8, ROAD_BOT+8+oy,  ROAD_W-16, 5, {255,255,255,140});
        DrawRectangle(ROAD_LEFT-36+oy, ROAD_TOP+8, 5, ROAD_W-16, {255,255,255,140});
        DrawRectangle(ROAD_RIGHT+8+oy, ROAD_TOP+8, 5, ROAD_W-16, {255,255,255,140});
    }

    for (const auto& v : vehicles) DrawVehicle(v);

    Color nsCol = (state==NS_GREEN) ? GREEN : (state==NS_YELLOW ? YELLOW : RED);
    Color ewCol = (state==EW_GREEN) ? GREEN : (state==EW_YELLOW ? YELLOW : RED);

    int off = ROAD_W/2 + 18;
    DrawTrafficLight(RCX + off,     RCY + off,      nsCol);
    DrawTrafficLight(RCX - off - 20, RCY - off - 70, nsCol);
    DrawTrafficLight(RCX - off - 20, RCY + off,      ewCol);
    DrawTrafficLight(RCX + off,     RCY - off - 70,  ewCol);

    DrawHUD(vehicles, state, timer, nsGreen, ewGreen);
    DrawUI(W, selectedType);

    EndDrawing();
}

//  Smart Traffic Simulation — Single File
//  Improvements:
//   - Density-based adaptive green time
//   - Named geometry constants (no magic numbers)
//   - Smooth braking
//   - Ambulance cross drawn + push-ahead logic restored
//   - Road markings, dashed lanes, stop lines, crosswalk
//   - Proper 3-bulb traffic light poles
//   - Fixed IntersectionState type safety
//   - Vehicle windshield detail
//   - NS/EW density HUD bars

#include "raylib.h"
#include <vector>
#include <cmath>

//  (change W/H here and everything follows)
static const int   W          = 1200;
static const int   H          = 900;
static const int   ROAD_W     = 220;   // total road width
static const int   LANE_W     = ROAD_W / 2;

// Road centre lines
static const int   RCX        = W / 2; // road centre X (vertical road)
static const int   RCY        = H / 2; // road centre Y (horizontal road)

// Road edges
static const int   ROAD_LEFT  = RCX - ROAD_W / 2;
static const int   ROAD_RIGHT = RCX + ROAD_W / 2;
static const int   ROAD_TOP   = RCY - ROAD_W / 2;
static const int   ROAD_BOT   = RCY + ROAD_W / 2;

// Stop lines (vehicles halt just before the intersection box)
static const int   STOP_S     = ROAD_TOP  - 5;   // southbound stops here (y)
static const int   STOP_N     = ROAD_BOT  + 5;   // northbound stops here (y)
static const int   STOP_E     = ROAD_LEFT - 5;   // eastbound  stops here (x)
static const int   STOP_W     = ROAD_RIGHT+ 5;   // westbound  stops here (x)

// Traffic light timing
static const float MIN_GREEN  =  4.0f;
static const float MAX_GREEN  = 22.0f;
static const float BASE_GREEN = 10.0f;
static const float YELLOW_DUR =  2.0f;

//  ENUMS & STRUCTS
enum IntersectionState { NS_GREEN, NS_YELLOW, EW_GREEN, EW_YELLOW };

struct Vehicle {
    Rectangle rect;
    Color     color;
    int       direction;   // 0=S 1=N 2=E 3=W
    float     speed;       // current speed (px/s)
    float     maxSpeed;
    bool      isAmbulance;
};

// UI globals
static int selectedType = 0; // 0=car 1=ambulance

//  HELPERS
// counts how many are currently traveling in a specific direction
int CountLane(const std::vector<Vehicle>& v, int dir) {
    int c = 0;
    for (const auto& x : v) if (x.direction == dir) c++;
    return c;
}

// scans if an emergency ambulance vehicle is present.
bool AmbulanceInLane(const std::vector<Vehicle>& v, int dir) {
    for (const auto& x : v)
        if (x.direction == dir && x.isAmbulance) return true;
    return false;
}

// Count vehicles that are still approaching / waiting (haven't cleared the intersection)
int CountApproaching(const std::vector<Vehicle>& vehicles, int dir) {
    int c = 0;
    for (const auto& v : vehicles) {
        if (v.direction != dir) continue;
        switch (dir) {
            case 0: if (v.rect.y + v.rect.height < ROAD_BOT) c++; break; // southbound: front hasn't exited bottom
            case 1: if (v.rect.y > ROAD_TOP)                 c++; break; // northbound: rear hasn't exited top
            case 2: if (v.rect.x + v.rect.width < ROAD_RIGHT)c++; break; // eastbound:  front hasn't exited right
            case 3: if (v.rect.x > ROAD_LEFT)                c++; break; // westbound:  rear hasn't exited left
        }
    }
    return c;
}

//  SPAWN
void SpawnVehicle(std::vector<Vehicle>& vehicles, int dir, bool amb) {
    Vehicle v;
    v.direction   = dir;
    v.isAmbulance = amb;
    v.maxSpeed    = amb ? 300.0f : 180.0f;
    v.speed       = v.maxSpeed;
    v.color       = amb ? RAYWHITE : BLUE;

    // Each direction gets its own lane (left-hand traffic convention)
    switch (dir) {
        case 0: v.rect = { (float)(RCX - LANE_W + 10), (float)(-60),       30, 50 }; break; // S: left half, top entry
        case 1: v.rect = { (float)(RCX + 10),           (float)(H + 20),   30, 50 }; break; // N: right half, bottom entry
        case 2: v.rect = { (float)(-60),                (float)(RCY + 10), 50, 30 }; break; // E: bottom half, left entry
        case 3: v.rect = { (float)(W + 20),             (float)(RCY - LANE_W + 10), 50, 30 }; break; // W: top half, right entry
    }
    
    //if spawn boundry == full => discard spawn request
    //if spawn boundry == clear => vehicle added to vector
    bool clear = true;
    for (const auto& e : vehicles)
        if (e.direction == v.direction && CheckCollisionRecs(v.rect, e.rect))
            { clear = false; break; }

    if (clear) vehicles.push_back(v);
}

//  TRAFFIC LIGHT UPDATE  (density-adaptive)
void UpdateTraffic(IntersectionState& state, float& timer,
                   const std::vector<Vehicle>& vehicles, float dt) {
    timer += dt;

    int ns = CountLane(vehicles,0) + CountLane(vehicles,1);
    int ew = CountLane(vehicles,2) + CountLane(vehicles,3);

    bool nsAmb = AmbulanceInLane(vehicles,0) || AmbulanceInLane(vehicles,1);
    bool ewAmb = AmbulanceInLane(vehicles,2) || AmbulanceInLane(vehicles,3);

    // Ambulance override
    if (nsAmb && !ewAmb) { state = NS_GREEN; timer = 0; return; }
    if (ewAmb && !nsAmb) { state = EW_GREEN; timer = 0; return; }
    if (nsAmb && ewAmb)  { state = (ns >= ew) ? NS_GREEN : EW_GREEN; timer = 0; return; }

    // Density-based green time:
    //   heavier lane gets up to MAX_GREEN, lighter lane as low as MIN_GREEN
    int total = ns + ew;
    float nsGreen = BASE_GREEN, ewGreen = BASE_GREEN;
    if (total > 0) {
        float nsRatio = (float)ns / total;   // 0..1
        float ewRatio = (float)ew / total;
        nsGreen = MIN_GREEN + nsRatio * (MAX_GREEN - MIN_GREEN);
        ewGreen = MIN_GREEN + ewRatio * (MAX_GREEN - MIN_GREEN);
    }

    // Early-skip: if the currently-green direction has NO approaching vehicles
    // and the waiting direction does, cut straight to yellow after a short grace period.
    static const float GRACE = 1.5f; // seconds before skipping an empty green
    int nsApproach = CountApproaching(vehicles,0) + CountApproaching(vehicles,1);
    int ewApproach = CountApproaching(vehicles,2) + CountApproaching(vehicles,3);

    switch (state) {
        case NS_GREEN:
            if (timer > nsGreen || (timer > GRACE && nsApproach == 0 && ewApproach > 0))
                { state = NS_YELLOW; timer = 0; }
            break;
        case NS_YELLOW: if (timer > YELLOW_DUR){ state = EW_GREEN;  timer = 0; } break;
        case EW_GREEN:
            if (timer > ewGreen || (timer > GRACE && ewApproach == 0 && nsApproach > 0))
                { state = EW_YELLOW; timer = 0; }
            break;
        case EW_YELLOW: if (timer > YELLOW_DUR){ state = NS_GREEN;  timer = 0; } break;
    }
}

//  VEHICLE UPDATE  (smooth braking + ambulance push)
void UpdateVehicles(std::vector<Vehicle>& vehicles,
                    IntersectionState state, float dt) {

    bool nsRed = (state == EW_GREEN || state == EW_YELLOW || state == NS_YELLOW);
    bool ewRed = (state == NS_GREEN || state == NS_YELLOW || state == EW_YELLOW);

    for (size_t i = 0; i < vehicles.size(); i++) {
        auto& v  = vehicles[i];
        Rectangle& r = v.rect;

        // ---- desired speed ----
        float desired = v.maxSpeed;

        // red-light stop zone
        bool atRed = false;
        if (!v.isAmbulance) {
            float front;
            switch (v.direction) {
                case 0: front = r.y + r.height; if (front > STOP_S - 60 && front < STOP_S + 10 && nsRed) atRed = true; break;
                case 1: front = r.y;             if (front < STOP_N + 60 && front > STOP_N - 10 && nsRed) atRed = true; break;
                case 2: front = r.x + r.width;  if (front > STOP_E - 60 && front < STOP_E + 10 && ewRed) atRed = true; break;
                case 3: front = r.x;             if (front < STOP_W + 60 && front > STOP_W - 10 && ewRed) atRed = true; break;
            }
        }
        if (atRed) desired = 0;

        // gap to car ahead
        float minGap = 1e9f;
        bool  pushedByAmb = false;

        for (size_t j = 0; j < vehicles.size(); j++) {
            if (i == j) continue;
            auto& o = vehicles[j];
            if (v.direction != o.direction) continue;

            float gap = 1e9f;
            switch (v.direction) {
                case 0: if (o.rect.y > r.y) gap = o.rect.y - (r.y + r.height); break;
                case 1: if (o.rect.y < r.y) gap = r.y - (o.rect.y + o.rect.height); break;
                case 2: if (o.rect.x > r.x) gap = o.rect.x - (r.x + r.width); break;
                case 3: if (o.rect.x < r.x) gap = r.x - (o.rect.x + o.rect.width); break;
            }
            if (gap >= 0 && gap < minGap) minGap = gap;

            // ambulance pushing car ahead
            if (!v.isAmbulance && o.isAmbulance) {
                float behind = 1e9f;
                switch (v.direction) {
                    case 0: if (o.rect.y < r.y) behind = r.y - (o.rect.y + o.rect.height); break;
                    case 1: if (o.rect.y > r.y) behind = o.rect.y - (r.y + r.height); break;
                    case 2: if (o.rect.x < r.x) behind = r.x - (o.rect.x + o.rect.width); break;
                    case 3: if (o.rect.x > r.x) behind = o.rect.x - (r.x + r.width); break;
                }
                if (behind >= 0 && behind < 100) pushedByAmb = true;
            }
        }

        // smooth braking based on gap
        if (minGap < 80.0f)  desired = fminf(desired, v.maxSpeed * (minGap / 80.0f));
        if (minGap < 5.0f)   desired = 0;
        if (pushedByAmb)     desired = fmaxf(desired, 300.0f);

        // lerp speed toward desired
        float accel = (desired > v.speed) ? 200.0f : 400.0f;
        if (v.speed < desired) v.speed = fminf(v.speed + accel * dt, desired);
        else                   v.speed = fmaxf(v.speed - accel * dt, desired);

        if (v.speed > 0) {
            switch (v.direction) {
                case 0: r.y += v.speed * dt; break;
                case 1: r.y -= v.speed * dt; break;
                case 2: r.x += v.speed * dt; break;
                case 3: r.x -= v.speed * dt; break;
            }
        }
    }

    // cull off-screen
    for (auto it = vehicles.begin(); it != vehicles.end(); ) {
        if (it->rect.x < -200 || it->rect.x > W+200 ||
            it->rect.y < -200 || it->rect.y > H+200)
            it = vehicles.erase(it);
        else ++it;
    }
}

//  DRAW HELPERS
// 3-bulb traffic light pole
void DrawTrafficLight(int x, int y, Color active) {
    // pole
    DrawRectangle(x + 8, y + 70, 4, 40, DARKGRAY);
    // housing
    DrawRectangle(x, y, 20, 68, BLACK);
    DrawRectangleLines(x, y, 20, 68, DARKGRAY);
    // bulbs
    DrawCircle(x+10, y+12, 7, (active.r==255&&active.g<50) ? RED   : ColorAlpha(RED,   0.25f));
    DrawCircle(x+10, y+34, 7, (active.g>200&&active.r>200) ? YELLOW: ColorAlpha(YELLOW,0.25f));
    DrawCircle(x+10, y+56, 7, (active.r<50&&active.g>200)  ? GREEN : ColorAlpha(GREEN, 0.25f));
}

// Dashed line
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

    // windshield
    Color glass = ColorAlpha(SKYBLUE, 0.7f);
    if (v.direction == 0 || v.direction == 1) { // vertical
        DrawRectangle((int)v.rect.x+4, (int)v.rect.y+8, (int)v.rect.width-8, 10, glass);
    } else { // horizontal
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

void DrawUI(int W) {
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

void DrawHUD(const std::vector<Vehicle>& vehicles,
             IntersectionState state, float timer,
             float nsGreen, float ewGreen) {
    int ns = CountLane(vehicles,0)+CountLane(vehicles,1);
    int ew = CountLane(vehicles,2)+CountLane(vehicles,3);

    // background
    DrawRectangleRounded({10,10,200,120},0.1f,4,{0,0,0,160});

    DrawText(TextFormat("N/S: %d cars", ns), 20, 20, 18, WHITE);
    DrawText(TextFormat("E/W: %d cars", ew), 20, 44, 18, WHITE);

    // density bars
    int maxV = 20;
    DrawRectangle(20, 70, 160, 12, DARKGRAY);
    DrawRectangle(20, 70, (int)fminf(ns*8,160), 12, GREEN);
    DrawRectangle(20, 88, 160, 12, DARKGRAY);
    DrawRectangle(20, 88, (int)fminf(ew*8,160), 12, ORANGE);

    DrawText("NS", 186, 68, 12, GREEN);
    DrawText("EW", 186, 86, 12, ORANGE);

    DrawText(TextFormat("Timer: %.1fs", timer), 20, 106, 14, LIGHTGRAY);
}

void DrawScene(int W, int H, const std::vector<Vehicle>& vehicles,
               IntersectionState state, float timer,
               float nsGreen, float ewGreen) {
    BeginDrawing();
    ClearBackground({34, 85, 34, 255}); // grass green

    // ---- roads ----
    DrawRectangle(ROAD_LEFT,  0,         ROAD_W, H, {60,60,60,255});
    DrawRectangle(0, ROAD_TOP, W,         ROAD_W, {60,60,60,255});

    // intersection box (slightly lighter)
    DrawRectangle(ROAD_LEFT, ROAD_TOP, ROAD_W, ROAD_W, {70,70,70,255});

    // ---- road markings ----
    // dashed centre lines
    DrawDashedLine(RCX, 0,      RCX, ROAD_TOP,  20, {200,200,0,180});
    DrawDashedLine(RCX, ROAD_BOT, RCX, H,        20, {200,200,0,180});
    DrawDashedLine(0, RCY,      ROAD_LEFT, RCY,  20, {200,200,0,180});
    DrawDashedLine(ROAD_RIGHT, RCY, W, RCY,      20, {200,200,0,180});

    // stop lines
    DrawLineEx({(float)ROAD_LEFT,(float)STOP_S},{(float)RCX,(float)STOP_S},3,WHITE);
    DrawLineEx({(float)RCX,(float)STOP_N},{(float)ROAD_RIGHT,(float)STOP_N},3,WHITE);
    DrawLineEx({(float)STOP_E,(float)ROAD_TOP},{(float)STOP_E,(float)RCY},3,WHITE);
    DrawLineEx({(float)STOP_W,(float)RCY},{(float)STOP_W,(float)ROAD_BOT},3,WHITE);

    // zebra crosswalks (4 stripes each, placed outside intersection box)
    for (int i = 0; i < 4; i++) {
        int oy = i * 9;
        // top crosswalk
        DrawRectangle(ROAD_LEFT+8, ROAD_TOP-36+oy, ROAD_W-16, 5, {255,255,255,140});
        // bottom crosswalk
        DrawRectangle(ROAD_LEFT+8, ROAD_BOT+8+oy,  ROAD_W-16, 5, {255,255,255,140});
        // left crosswalk
        DrawRectangle(ROAD_LEFT-36+oy, ROAD_TOP+8, 5, ROAD_W-16, {255,255,255,140});
        // right crosswalk
        DrawRectangle(ROAD_RIGHT+8+oy, ROAD_TOP+8, 5, ROAD_W-16, {255,255,255,140});
    }

    // ---- vehicles ----
    for (const auto& v : vehicles) DrawVehicle(v);

    // ---- traffic lights ----
    Color nsCol = (state==NS_GREEN) ? GREEN : (state==NS_YELLOW ? YELLOW : RED);
    Color ewCol = (state==EW_GREEN) ? GREEN : (state==EW_YELLOW ? YELLOW : RED);

    int off = ROAD_W/2 + 18;
    DrawTrafficLight(RCX + off,     RCY + off,      nsCol);
    DrawTrafficLight(RCX - off - 20, RCY - off - 70, nsCol);
    DrawTrafficLight(RCX - off - 20, RCY + off,      ewCol);
    DrawTrafficLight(RCX + off,     RCY - off - 70,  ewCol);

    // ---- HUD ----
    DrawHUD(vehicles, state, timer, nsGreen, ewGreen);
    DrawUI(W);

    EndDrawing();
}

//  MAIN
int main() {
    InitWindow(W, H, "Smart Traffic Simulation");
    SetTargetFPS(60);

    std::vector<Vehicle> vehicles;
    IntersectionState state = NS_GREEN;
    float timer   = 0;
    float nsGreen = BASE_GREEN;
    float ewGreen = BASE_GREEN;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ---- INPUT ----
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            int px = W - 200, py = 20;

            // Vehicle type selection buttons
            if (CheckCollisionPointRec(m,{(float)px+70,(float)py+42,18,18})) selectedType=0;
            if (CheckCollisionPointRec(m,{(float)px+70,(float)py+66,18,18})) selectedType=1;

            // Click on road outside panel -> infer direction from lane clicked
            if (m.x < px) {
                bool onVertRoad = (m.x >= ROAD_LEFT && m.x <= ROAD_RIGHT);
                bool onHorzRoad = (m.y >= ROAD_TOP  && m.y <= ROAD_BOT);
                bool inBox      = onVertRoad && onHorzRoad;

                if (!inBox) {
                    int dir = -1;
                    if (onVertRoad && !onHorzRoad) {
                        // Vertical road: left lane travels South (0), right lane travels North (1)
                        dir = (m.x < RCX) ? 0 : 1;
                    } else if (onHorzRoad && !onVertRoad) {
                        // Horizontal road: bottom lane travels East (2), top lane travels West (3)
                        dir = (m.y > RCY) ? 2 : 3;
                    }
                    if (dir >= 0)
                        SpawnVehicle(vehicles, dir, selectedType==1);
                }
            }
        }

        // ---- UPDATE ----
        // recalculate green times for HUD display
        {
            int ns = CountLane(vehicles,0)+CountLane(vehicles,1);
            int ew = CountLane(vehicles,2)+CountLane(vehicles,3);
            int total = ns+ew;
            if (total > 0) {
                nsGreen = MIN_GREEN + ((float)ns/total)*(MAX_GREEN-MIN_GREEN);
                ewGreen = MIN_GREEN + ((float)ew/total)*(MAX_GREEN-MIN_GREEN);
            } else {
                nsGreen = ewGreen = BASE_GREEN;
            }
        }
        UpdateTraffic(state, timer, vehicles, dt);
        UpdateVehicles(vehicles, state, dt);

        // ---- DRAW ----
        DrawScene(W, H, vehicles, state, timer, nsGreen, ewGreen);
    }

    CloseWindow();
    return 0;
}

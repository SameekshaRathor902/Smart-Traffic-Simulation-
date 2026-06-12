#include "raylib.h"
#include "constants.h"
#include "traffic_light.h"
#include "vehicle.h"
#include "renderer.h"
#include <vector>

int main() {
    InitWindow(W, H, "Smart Traffic Simulation");
    SetTargetFPS(60);

    std::vector<Vehicle> vehicles;
    IntersectionState state = NS_GREEN;
    float timer   = 0;
    float nsGreen = BASE_GREEN;
    float ewGreen = BASE_GREEN;
    int selectedType = 0; // 0=car, 1=ambulance

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // INPUT HANDLING 
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            int px = W - 200, py = 20;

            if (CheckCollisionPointRec(m, {(float)px+70, (float)py+42, 18, 18})) selectedType = 0;
            if (CheckCollisionPointRec(m, {(float)px+70, (float)py+66, 18, 18})) selectedType = 1;

            if (m.x < px) {
                bool onVertRoad = (m.x >= ROAD_LEFT && m.x <= ROAD_RIGHT);
                bool onHorzRoad = (m.y >= ROAD_TOP  && m.y <= ROAD_BOT);
                bool inBox      = onVertRoad && onHorzRoad;

                if (!inBox) {
                    int dir = -1;
                    if (onVertRoad && !onHorzRoad) {
                        dir = (m.x < RCX) ? 0 : 1;
                    } else if (onHorzRoad && !onVertRoad) {
                        dir = (m.y > RCY) ? 2 : 3;
                    }
                    if (dir >= 0)
                        SpawnVehicle(vehicles, dir, selectedType == 1);
                }
            }
        }

        // SIMULATION UPDATES
        {
            int ns = CountLane(vehicles, 0) + CountLane(vehicles, 1);
            int ew = CountLane(vehicles, 2) + CountLane(vehicles, 3);
            int total = ns + ew;
            if (total > 0) {
                nsGreen = MIN_GREEN + ((float)ns / total) * (MAX_GREEN - MIN_GREEN);
                ewGreen = MIN_GREEN + ((float)ew / total) * (MAX_GREEN - MIN_GREEN);
            } else {
                nsGreen = ewGreen = BASE_GREEN;
            }
        }
        
        UpdateTraffic(state, timer, vehicles, dt);
        UpdateVehicles(vehicles, state, dt);

        // ---- RENDERING ----
        DrawScene(W, H, vehicles, state, timer, nsGreen, ewGreen, selectedType);
    }

    CloseWindow();
    return 0;
}

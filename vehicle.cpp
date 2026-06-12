#include "vehicle.h"
#include "constants.h"
#include <cmath>

int CountLane(const std::vector<Vehicle>& v, int dir) {
    int c = 0;
    for (const auto& x : v) if (x.direction == dir) c++;
    return c;
}

bool AmbulanceInLane(const std::vector<Vehicle>& v, int dir) {
    for (const auto& x : v)
        if (x.direction == dir && x.isAmbulance) return true;
    return false;
}

void SpawnVehicle(std::vector<Vehicle>& vehicles, int dir, bool amb) {
    Vehicle v;
    v.direction   = dir;
    v.isAmbulance = amb;
    v.maxSpeed    = amb ? 300.0f : 180.0f;
    v.speed       = v.maxSpeed;
    v.color       = amb ? RAYWHITE : BLUE;

    switch (dir) {
        case 0: v.rect = { (float)(RCX - LANE_W + 10), (float)(-60),       30, 50 }; break;
        case 1: v.rect = { (float)(RCX + 10),           (float)(H + 20),   30, 50 }; break;
        case 2: v.rect = { (float)(-60),                (float)(RCY + 10), 50, 30 }; break;
        case 3: v.rect = { (float)(W + 20),             (float)(RCY - LANE_W + 10), 50, 30 }; break;
    }

    bool clear = true;
    for (const auto& e : vehicles)
        if (e.direction == v.direction && CheckCollisionRecs(v.rect, e.rect))
            { clear = false; break; }

    if (clear) vehicles.push_back(v);
}

void UpdateVehicles(std::vector<Vehicle>& vehicles, IntersectionState state, float dt) {
    bool nsRed = (state == EW_GREEN || state == EW_YELLOW || state == NS_YELLOW);
    bool ewRed = (state == NS_GREEN || state == NS_YELLOW || state == EW_YELLOW);

    for (size_t i = 0; i < vehicles.size(); i++) {
        auto& v  = vehicles[i];
        Rectangle& r = v.rect;
        float desired = v.maxSpeed;

        // Red light collision handling
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

        // Safe spacing calculations
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

        // LERP movement updates
        if (minGap < 80.0f)  desired = fminf(desired, v.maxSpeed * (minGap / 80.0f));
        if (minGap < 5.0f)   desired = 0;
        if (pushedByAmb)     desired = fmaxf(desired, 300.0f);

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

    // Cleanup offscreen instances
    for (auto it = vehicles.begin(); it != vehicles.end(); ) {
        if (it->rect.x < -200 || it->rect.x > W+200 || it->rect.y < -200 || it->rect.y > H+200)
            it = vehicles.erase(it);
        else ++it;
    }
}
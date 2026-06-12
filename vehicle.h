#ifndef VEHICLE_H
#define VEHICLE_H

#include "raylib.h"
#include "traffic_light.h"
#include <vector>

struct Vehicle {
    Rectangle rect;
    Color     color;
    int       direction;   // 0=S, 1=N, 2=E, 3=W
    float     speed;       // current speed (px/s)
    float     maxSpeed;
    bool      isAmbulance;
};

// Lane counting & checking utilities
int CountLane(const std::vector<Vehicle>& v, int dir);
bool AmbulanceInLane(const std::vector<Vehicle>& v, int dir);

// Core simulation loops
void SpawnVehicle(std::vector<Vehicle>& vehicles, int dir, bool amb);
void UpdateVehicles(std::vector<Vehicle>& vehicles, IntersectionState state, float dt);

#endif
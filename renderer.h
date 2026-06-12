#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include "traffic_light.h"
#include "vehicle.h"
#include <vector>

void DrawTrafficLight(int x, int y, Color active);
void DrawDashedLine(int x1, int y1, int x2, int y2, int dashLen, Color c);
void DrawVehicle(const Vehicle& v);
void DrawUI(int W, int selectedType);
void DrawHUD(const std::vector<Vehicle>& vehicles, IntersectionState state, float timer, float nsGreen, float ewGreen);
void DrawScene(int W, int H, const std::vector<Vehicle>& vehicles, IntersectionState state, float timer, float nsGreen, float ewGreen, int selectedType);

#endif
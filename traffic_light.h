#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <vector>

// Forward declaration of Vehicle to avoid circular inclusion
struct Vehicle;

enum IntersectionState { NS_GREEN, NS_YELLOW, EW_GREEN, EW_YELLOW };

// Adaptive logic functions
void UpdateTraffic(IntersectionState& state, float& timer, const std::vector<Vehicle>& vehicles, float dt);
int CountApproaching(const std::vector<Vehicle>& vehicles, int dir);

#endif
#include "traffic_light.h"
#include "vehicle.h"
#include "constants.h"

// Count vehicles that are still approaching / waiting (haven't cleared the intersection)
int CountApproaching(const std::vector<Vehicle>& vehicles, int dir) {
    int c = 0;
    for (const auto& v : vehicles) {
        if (v.direction != dir) continue;
        switch (dir) {
            case 0: if (v.rect.y + v.rect.height < ROAD_BOT) c++; break; // southbound
            case 1: if (v.rect.y > ROAD_TOP)                 c++; break; // northbound
            case 2: if (v.rect.x + v.rect.width < ROAD_RIGHT)c++; break; // eastbound
            case 3: if (v.rect.x > ROAD_LEFT)                c++; break; // westbound
        }
    }
    return c;
}

//  TRAFFIC LIGHT UPDATE  (density-adaptive)
void UpdateTraffic(IntersectionState& state, float& timer, const std::vector<Vehicle>& vehicles, float dt) {
    timer += dt;

    int ns = CountLane(vehicles, 0) + CountLane(vehicles, 1);
    int ew = CountLane(vehicles, 2) + CountLane(vehicles, 3);

    bool nsAmb = AmbulanceInLane(vehicles, 0) || AmbulanceInLane(vehicles, 1);
    bool ewAmb = AmbulanceInLane(vehicles, 2) || AmbulanceInLane(vehicles, 3);

    // Ambulance overrides
    if (nsAmb && !ewAmb) { state = NS_GREEN; timer = 0; return; }
    if (ewAmb && !nsAmb) { state = EW_GREEN; timer = 0; return; }
    if (nsAmb && ewAmb)  { state = (ns >= ew) ? NS_GREEN : EW_GREEN; timer = 0; return; }

    // Density-based green time calculations
    //  heavier lane gets up to MAX_GREEN, lighter lane as low as MIN_GREEN
    int total = ns + ew;
    float nsGreen = BASE_GREEN, ewGreen = BASE_GREEN;
    if (total > 0) {
        float nsRatio = (float)ns / total;
        float ewRatio = (float)ew / total;
        nsGreen = MIN_GREEN + nsRatio * (MAX_GREEN - MIN_GREEN);
        ewGreen = MIN_GREEN + ewRatio * (MAX_GREEN - MIN_GREEN);
    }

    // Early-skip: if the currently-green direction has NO approaching vehicles
    // and the waiting direction does, cut straight to yellow after a short grace period.
    static const float GRACE = 1.5f; // seconds before skipping an empty green
    int nsApproach = CountApproaching(vehicles, 0) + CountApproaching(vehicles, 1);
    int ewApproach = CountApproaching(vehicles, 2) + CountApproaching(vehicles, 3);

    switch (state) {
        case NS_GREEN:
            if (timer > nsGreen || (timer > GRACE && nsApproach == 0 && ewApproach > 0))
                { state = NS_YELLOW; timer = 0; }
            break;
        case NS_YELLOW: 
            if (timer > YELLOW_DUR) { state = EW_GREEN;  timer = 0; } 
            break;
        case EW_GREEN:
            if (timer > ewGreen || (timer > GRACE && ewApproach == 0 && nsApproach > 0))
                { state = EW_YELLOW; timer = 0; }
            break;
        case EW_YELLOW: 
            if (timer > YELLOW_DUR) { state = NS_GREEN;  timer = 0; } 
            break;
    }
}

#pragma once
#include "hand_tracking_types.h"
#include <vector>

namespace livevel {

// Rule-based gesture classifier using spatial relationships between landmarks.
// This classifier is shared by both backends and provides consistent
// gesture recognition from 21-point hand landmarks.
class GestureClassifier {
public:
    GestureClassifier() = default;

    // Classify the gesture for each hand in the tracking result.
    std::vector<GestureEvent> classify(const HandTrackingResult& result) const;

    // Classify the gesture for a single hand.
    GestureEvent classifySingleHand(const HandData& hand, int handIndex) const;

private:
    // Divides the hand into 12 independent zones and determines their states.
    std::array<ZoneState, static_cast<std::size_t>(HandZoneId::Count)> analyzeZones(const HandData& hand) const;

    // Evaluates a gesture candidate and returns its score based on visible zones.
    // Returns a score > 0 if there's a match, <= 0 if contradiction or insufficient data.
    float scoreGesture(const std::array<ZoneState, static_cast<std::size_t>(HandZoneId::Count)>& zones,
                       const std::array<bool, 5>& expectedFingers) const;
};

} // namespace livevel

#include "gesture_classifier.h"
#include <cmath>
#include <algorithm>
#include <initializer_list>
#include <utility>

namespace livevel {

std::array<ZoneState, static_cast<std::size_t>(HandZoneId::Count)> GestureClassifier::analyzeZones(const HandData& hand) const {
    std::array<ZoneState, static_cast<std::size_t>(HandZoneId::Count)> zones;
    
    // Helper to determine if a specific segment of a finger is extended.
    auto checkExtended = [&](int startIdx, int endIdx, bool isThumb) -> std::pair<bool, float> {
        const auto& p1 = hand.landmarks[startIdx];
        const auto& p2 = hand.landmarks[endIdx];
        if (p1.visibility < 0.5f || p2.visibility < 0.5f) {
            return {false, 0.0f}; // Occluded
        }
        
        bool extended = false;
        float margin = 0.0f;
        
        if (isThumb) {
            const auto& mcp = hand.landmarks[HandLandmarkIndex::THUMB_MCP];
            float p1Dist = std::abs(p1.x - mcp.x);
            float p2Dist = std::abs(p2.x - mcp.x);
            extended = (p2Dist > p1Dist);
            margin = std::abs(p2Dist - p1Dist);
        } else {
            extended = (p2.y < p1.y); // p2 (closer to tip) is above p1
            margin = std::abs(p2.y - p1.y);
        }
        
        float conf = std::min(margin / 0.15f, 1.0f);
        return {extended, conf};
    };

    auto checkVisibility = [&](std::initializer_list<int> indices) {
        float minVis = 1.0f;
        for (int idx : indices) {
            minVis = std::min(minVis, hand.landmarks[idx].visibility);
        }
        return minVis > 0.5f;
    };

    // Palm Zones
    zones[static_cast<int>(HandZoneId::PalmBase)].isVisible = checkVisibility({HandLandmarkIndex::WRIST, HandLandmarkIndex::THUMB_CMC});
    zones[static_cast<int>(HandZoneId::PalmBase)].confidence = 1.0f; 
    
    zones[static_cast<int>(HandZoneId::PalmTop)].isVisible = checkVisibility({HandLandmarkIndex::INDEX_MCP, HandLandmarkIndex::MIDDLE_MCP, HandLandmarkIndex::RING_MCP, HandLandmarkIndex::PINKY_MCP});
    zones[static_cast<int>(HandZoneId::PalmTop)].confidence = 1.0f;

    // Fingers
    const struct { int mcp, pip, tip; bool isThumb; } kFingerData[] = {
        {HandLandmarkIndex::THUMB_MCP,  HandLandmarkIndex::THUMB_IP,  HandLandmarkIndex::THUMB_TIP,  true},
        {HandLandmarkIndex::INDEX_MCP,  HandLandmarkIndex::INDEX_PIP, HandLandmarkIndex::INDEX_TIP,  false},
        {HandLandmarkIndex::MIDDLE_MCP, HandLandmarkIndex::MIDDLE_PIP,HandLandmarkIndex::MIDDLE_TIP, false},
        {HandLandmarkIndex::RING_MCP,   HandLandmarkIndex::RING_PIP,  HandLandmarkIndex::RING_TIP,   false},
        {HandLandmarkIndex::PINKY_MCP,  HandLandmarkIndex::PINKY_PIP, HandLandmarkIndex::PINKY_TIP,  false}
    };

    for (int i = 0; i < 5; ++i) {
        int baseZoneIdx = static_cast<int>(HandZoneId::ThumbBase) + i * 2;
        int tipZoneIdx = baseZoneIdx + 1;
        
        auto baseExt = checkExtended(kFingerData[i].mcp, kFingerData[i].pip, kFingerData[i].isThumb);
        zones[baseZoneIdx].isVisible = checkVisibility({kFingerData[i].mcp, kFingerData[i].pip});
        zones[baseZoneIdx].isExtended = baseExt.first;
        zones[baseZoneIdx].confidence = baseExt.second;
        
        auto tipExt = checkExtended(kFingerData[i].pip, kFingerData[i].tip, kFingerData[i].isThumb);
        zones[tipZoneIdx].isVisible = checkVisibility({kFingerData[i].pip, kFingerData[i].tip});
        zones[tipZoneIdx].isExtended = tipExt.first;
        zones[tipZoneIdx].confidence = tipExt.second;
    }

    return zones;
}

float GestureClassifier::scoreGesture(const std::array<ZoneState, static_cast<std::size_t>(HandZoneId::Count)>& zones,
                                      const std::array<bool, 5>& expectedFingers) const {
    float score = 0.0f;
    float maxPossibleScore = 0.0f;
    int visibleZones = 0;

    for (int i = 0; i < 5; ++i) {
        int baseZoneIdx = static_cast<int>(HandZoneId::ThumbBase) + i * 2;
        int tipZoneIdx = baseZoneIdx + 1;
        
        if (zones[baseZoneIdx].isVisible) {
            visibleZones++;
            maxPossibleScore += 1.0f;
            if (zones[baseZoneIdx].isExtended == expectedFingers[i]) {
                score += zones[baseZoneIdx].confidence;
            } else {
                score -= zones[baseZoneIdx].confidence;
            }
        }
        
        if (zones[tipZoneIdx].isVisible) {
            visibleZones++;
            maxPossibleScore += 1.0f;
            if (zones[tipZoneIdx].isExtended == expectedFingers[i]) {
                score += zones[tipZoneIdx].confidence;
            } else {
                score -= zones[tipZoneIdx].confidence;
            }
        }
    }
    
    // Add palm zones to visible count to require some hand structure
    if (zones[static_cast<int>(HandZoneId::PalmBase)].isVisible) visibleZones++;
    if (zones[static_cast<int>(HandZoneId::PalmTop)].isVisible) visibleZones++;

    // If too few zones are visible, we can't make a reliable guess.
    if (visibleZones < 3 || maxPossibleScore == 0.0f) {
        return 0.0f;
    }

    float normalizedScore = score / maxPossibleScore;
    return std::max(0.0f, normalizedScore);
}

GestureEvent GestureClassifier::classifySingleHand(const HandData& hand, int handIndex) const {
    GestureEvent event;
    event.hand = hand.handedness;
    event.handIndex = handIndex;

    auto zones = analyzeZones(hand);

    struct GestureCandidate {
        HandGesture gesture;
        std::array<bool, 5> expected;
    };

    GestureCandidate candidates[] = {
        {HandGesture::ILoveYou,   {true,  true,  false, false, true}},
        {HandGesture::Victory,    {false, true,  true,  false, false}},
        {HandGesture::ThumbUp,    {true,  false, false, false, false}},
        {HandGesture::PointingUp, {false, true,  false, false, false}},
        {HandGesture::OpenPalm,   {true,  true,  true,  true,  true}},
        {HandGesture::Fist,       {false, false, false, false, false}}
    };

    HandGesture bestGesture = HandGesture::None;
    float bestScore = 0.0f;

    for (const auto& candidate : candidates) {
        float score = scoreGesture(zones, candidate.expected);
        
        if (candidate.gesture == HandGesture::ThumbUp && score > 0.0f) {
            // Need wrist and middle MCP to distinguish ThumbUp vs ThumbDown
            if (hand.landmarks[HandLandmarkIndex::WRIST].visibility > 0.5f &&
                hand.landmarks[HandLandmarkIndex::MIDDLE_MCP].visibility > 0.5f) {
                const auto& wrist = hand.landmarks[HandLandmarkIndex::WRIST];
                const auto& middleMcp = hand.landmarks[HandLandmarkIndex::MIDDLE_MCP];
                
                if (wrist.y > middleMcp.y) {
                    // It is ThumbUp, keep the score
                } else {
                    // It is ThumbDown
                    if (score > bestScore) {
                        bestScore = score;
                        bestGesture = HandGesture::ThumbDown;
                    }
                    continue; 
                }
            } else {
                // If occluded, default to ThumbUp as it's more common, but reduce score slightly
                score *= 0.9f;
            }
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestGesture = candidate.gesture;
        }
    }

    if (bestScore > 0.35f) {
        event.gesture = bestGesture;
        event.confidence = bestScore;
    } else {
        event.gesture = HandGesture::None;
        event.confidence = 0.0f;
    }

    return event;
}

std::vector<GestureEvent> GestureClassifier::classify(
    const HandTrackingResult& result) const {

    std::vector<GestureEvent> events;
    events.reserve(result.hands.size());

    for (int i = 0; i < static_cast<int>(result.hands.size()); ++i) {
        GestureEvent ev = classifySingleHand(result.hands[i], i);
        if (ev.gesture != HandGesture::None) {
            events.push_back(ev);
        }
    }

    return events;
}

} // namespace livevel

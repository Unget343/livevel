#include "hand_tracking_types.h"
#include "mediapipe_c_api.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace livevel;

    constexpr std::array<std::array<int, 2>, 21> expectedConnections = {{
        {0, 1}, {1, 5}, {5, 9}, {9, 13}, {13, 17}, {0, 17},
        {1, 2}, {2, 3}, {3, 4},
        {5, 6}, {6, 7}, {7, 8},
        {9, 10}, {10, 11}, {11, 12},
        {13, 14}, {14, 15}, {15, 16},
        {17, 18}, {18, 19}, {19, 20},
    }};

    static_assert(HandLandmarkIndex::COUNT == 21);
    assert(kHandConnections == expectedConnections);

    std::array<bool, HandLandmarkIndex::COUNT> used{};
    for (const auto& connection : kHandConnections) {
        assert(connection[0] >= 0 && connection[0] < HandLandmarkIndex::COUNT);
        assert(connection[1] >= 0 && connection[1] < HandLandmarkIndex::COUNT);
        assert(connection[0] != connection[1]);
        used[connection[0]] = true;
        used[connection[1]] = true;
    }
    for (const bool landmarkIsConnected : used) assert(landmarkIsConnected);

    HandData hand;
    for (int i = 0; i < HandLandmarkIndex::COUNT; ++i) {
        hand.landmarks[i] = {static_cast<float>(i) / HandLandmarkIndex::COUNT,
                             static_cast<float>(i) / HandLandmarkIndex::COUNT,
                             -static_cast<float>(i), 1.0f};
    }
    assert(hand.landmarks[HandLandmarkIndex::THUMB_TIP].x == 4.0f / 21.0f);
    assert(hand.landmarks[HandLandmarkIndex::PINKY_TIP].z == -20.0f);

    std::cout << "21-landmark hand tracking contract passed.\n";
    return 0;
}

// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// ZFinder - Zoom level calculator
// Port of Njord's ZFinder.kt

#ifndef S57_POSTGIS_ZFINDER_HPP
#define S57_POSTGIS_ZFINDER_HPP

namespace s57 {

// ZFinder calculates the appropriate zoom level from a scale value
// This is a direct port of Njord's ZFinder.kt logic
class ZFinder {
public:
    // One-to-one zoom level (highest detail)
    static constexpr int ONE_TO_ONE_ZOOM = 28;

    // Find the zoom level for a given scale
    // Matches Njord's ZFinder.findZoom() implementation
    static int findZoom(int scale) {
        int zoom = ONE_TO_ONE_ZOOM;
        double zScale = static_cast<double>(scale);
        while (zScale > 1.0) {
            zScale /= 2.0;
            zoom -= 1;
        }
        return zoom;
    }

    // Calculate min and max zoom from SCAMIN and SCAMAX attributes
    // Returns a pair of (minZ, maxZ)
    static std::pair<int, int> calculateZRange(int scamin, int scamax) {
        int minZ = 0;
        int maxZ = ONE_TO_ONE_ZOOM;

        if (scamin > 0) {
            minZ = findZoom(scamin);
        }
        if (scamax > 0) {
            maxZ = findZoom(scamax);
        }

        // Ensure minZ <= maxZ
        if (minZ > maxZ) {
            std::swap(minZ, maxZ);
        }

        return {minZ, maxZ};
    }
};

} // namespace s57

#endif // S57_POSTGIS_ZFINDER_HPP

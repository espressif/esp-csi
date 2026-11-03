/*
 * SPDX-FileCopyrightText: 2026 vb3227
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "original_decision.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>

// Host statistics adapters; firmware links these functions from Espressif.
extern "C" float median(const float *a, size_t count)
{
    std::vector<float> values(a, a + count); std::sort(values.begin(), values.end());
    return count % 2 ? values[count / 2] : (values[count / 2 - 1] + values[count / 2]) / 2;
}
extern "C" float trimmean(const float *a, size_t count, float percent)
{
    std::vector<float> values(a, a + count); std::sort(values.begin(), values.end());
    size_t trim = static_cast<size_t>(count * percent / 2);
    float total = 0;
    for (size_t i = trim; i < count - trim; ++i) {
        total += values[i];
    }
    return total / (count - 2 * trim);
}

int main()
{
    OriginalDecision d;
    d.movementSensitivity = 0.20f;
    d.baseline(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
    assert(!d.presenceKnown && d.movementThreshold == 0.0003f);
    for (int i = 0; i < 25; ++i) {
        d.push(0.2f, 0.00005f);
    }
    assert(!d.movement && !d.presence);
    d.push(0.2f, 0.001f); assert(!d.movement && d.movementHits == 1);
    d.push(0.2f, 0.001f); assert(d.movement && d.movementHits == 2);
    // Both spikes were below the fixed threshold: the adaptive branch detected them.
    for (int i = 0; i < 5; ++i) {
        d.push(0.2f, 0.00005f);
    }
    assert(!d.movement && d.movementHits == 0);
    assert(!d.push(0, -1));
    assert(!d.push(std::numeric_limits<float>::infinity(), 0));
    d.baseline(0.01f, 0.00001f);
    assert(d.presenceKnown && d.movementThreshold == 0.00001f); // no old 0.0003 clamp
    for (int i = 0; i < 25; ++i) {
        d.push(0.2f, 0.000001f);
    }
    assert(d.presence && !d.movement);
    OriginalDecision normal, high;
    normal.movementSensitivity = 0.20f;
    high.movementSensitivity = 0.35f;
    for (int i = 0; i < 25; ++i) {
        normal.push(0, 0.0009f);
        high.push(0, 0.0009f);
    }
    assert(!normal.movement && high.movement);
    high.clear();
    for (int i = 0; i < 4; ++i) {
        assert(!high.push(0, 0.0009f));
    }
    assert(high.push(0, 0.0009f) && high.movement);
    std::puts("PASS: original adaptive branch, 2/5 votes, wraparound, calibration, sensitivity and warmup");
}

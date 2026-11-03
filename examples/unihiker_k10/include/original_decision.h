/*
 * SPDX-FileCopyrightText: 2026 vb3227
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <stddef.h>
#include <cmath>

// Algorithms exported by the unmodified Espressif archive.
extern "C" float trimmean(const float *, size_t, float);
extern "C" float median(const float *, size_t);

class OriginalDecision {
    float wander_[25] = {}, jitter_[25] = {};
    size_t next_ = 0, count_ = 0;
public:
    float wanderThreshold = 0, movementThreshold = 0.0003f;
    float presenceSensitivity = 0.15f, movementSensitivity = 0.35f;
    float wanderAverage = 0, jitterMedian = 0, lastJitter = 0, activity = 0;
    unsigned movementHits = 0;
    bool movement = false, presence = false, presenceKnown = false;

    void clear()
    {
        next_ = count_ = 0; movementHits = 0;
        wanderAverage = jitterMedian = lastJitter = activity = 0;
        movement = presence = false;
        for (size_t i = 0; i < 25; ++i) {
            wander_[i] = jitter_[i] = 0;
        }
    }
    void baseline(float wander, float jitter)
    {
        presenceKnown = std::isfinite(wander) && wander > 0;
        wanderThreshold = presenceKnown ? wander : 0;
        movementThreshold = std::isfinite(jitter) && jitter > 0 ? jitter : 0.0003f;
        clear();
    }
    bool push(float wander, float jitter)
    {
        if (!std::isfinite(wander) || !std::isfinite(jitter) || wander < 0 || jitter < 0) {
            return false;
        }
        wander_[next_] = wander; jitter_[next_] = jitter;
        next_ = (next_ + 1) % 25;
        if (count_ < 25) {
            ++count_;
        }
        if (count_ < 5) {
            return false;
        }
        // Exactly the console_test formulas; don't include unfilled zero slots at startup.
        wanderAverage = trimmean(wander_, count_, 0.5f);
        jitterMedian = median(jitter_, count_);
        lastJitter = jitter; movementHits = 0;
        float peak = 0;
        for (size_t i = 0; i < 5; ++i) {
            size_t index = (next_ + 24 - i) % 25;
            float value = jitter_[index];
            if (value * movementSensitivity > movementThreshold ||
                    (value * movementSensitivity > jitterMedian && value > 0.0002f)) {
                ++movementHits;
            }
            if (value > peak) {
                peak = value;
            }
        }
        movement = movementHits >= 2;
        presence = presenceKnown && wanderAverage * presenceSensitivity > wanderThreshold;
        float ratio = peak * movementSensitivity / movementThreshold;
        activity = fminf(1, ratio / 3);
        return true;
    }
};

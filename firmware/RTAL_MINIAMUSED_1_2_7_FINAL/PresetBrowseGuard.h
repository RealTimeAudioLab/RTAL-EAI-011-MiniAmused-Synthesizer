#pragma once
#include <stdint.h>

// RTAL MiniAmused - PRESET UP/DOWN touch gesture conditioner.
//
// The GT911/LVGL input stream can deliver multiple PRESSED/RELEASED/CLICKED
// sequences for one physical tap. Filtering CLICKED alone is insufficient:
// spurious events can occur after the old 180 ms timestamp lockout.
//
// We count a SHORT TAP only after a completed LVGL click AND a continuously
// released 110 ms settling period. A brief re-press on the same arrow within
// this settling window is folded into the *same* gesture. After finalization,
// re-presses on that arrow within 300 ms are discarded (hold-repeat exempt).
// Deliberate rapid browsing is available via the established hold-repeat.
//
// This module is independent of Arduino/LVGL, so the actual state machine can
// be compiled and tested on a desktop without any board libraries.
class PresetBrowseGuard final {
public:
    static constexpr uint32_t RELEASE_SETTLE_MS = 110;
    static constexpr uint32_t SAME_ARROW_GUARD_MS = 300;
    static constexpr uint32_t HOLD_START_MS = 500;

    void pressed(int direction, uint32_t now) {
        if(direction != -1 && direction != 1) return;

        // A fake release followed immediately by a new press is still the
        // original physical gesture. Never restart the hold timer here.
        if(phase_ == Phase::Settling && direction_ == direction &&
           elapsed(now, releasedAt_) < RELEASE_SETTLE_MS) {
            phase_ = Phase::Down;
            return;
        }

        // If the scheduler did not get a chance to service a settled release,
        // finish that click before looking at the next physical press.
        if(phase_ == Phase::Settling &&
           elapsed(now, releasedAt_) >= RELEASE_SETTLE_MS) {
            finishRelease();
        }

        if(phase_ == Phase::Down) {
            // Duplicate PRESSED is not a new gesture. Switching targets while
            // still pressed is cancelled until the next physical release.
            if(direction != direction_) {
                phase_ = Phase::Suppressed;
                direction_ = direction;
            }
            return;
        }
        if(phase_ == Phase::Suppressed) return;
        if(phase_ == Phase::Settling) {
            // Another target was pressed too soon. Preserve the first tap,
            // suppress this incomplete second gesture.
            finishRelease();
            phase_ = Phase::Suppressed;
            direction_ = direction;
            return;
        }

        const unsigned idx = index(direction);
        if(guardSeen_[idx] && elapsed(now, lastFinishedAt_[idx]) < SAME_ARROW_GUARD_MS) {
            phase_ = Phase::Suppressed;
            direction_ = direction;
            return;
        }

        phase_ = Phase::Down;
        direction_ = direction;
        pressedAt_ = now;
        lastRepeatAt_ = now;
        clicked_ = false;
        repeated_ = false;
    }

    void released(int direction, uint32_t now) {
        if(direction != -1 && direction != 1) return;
        if(direction != direction_) return;
        if(phase_ == Phase::Down) {
            phase_ = Phase::Settling;
            releasedAt_ = now;
        } else if(phase_ == Phase::Suppressed) {
            phase_ = Phase::Idle;
            direction_ = 0;
        }
    }

    // LVGL 8 normally emits RELEASED before CLICKED. Do not consume a tap
    // at RELEASED, and never emit steps directly inside an LVGL callback.
    void clicked(int direction) {
        if(direction == direction_ &&
           (phase_ == Phase::Down || phase_ == Phase::Settling)) {
            clicked_ = true;
        }
    }

    void pressLost(int direction, uint32_t now) {
        if(direction != direction_) return;
        if(phase_ != Phase::Idle) {
            // A drag off the arrow must never generate a browse step.
            if(direction == -1 || direction == 1) {
                const unsigned idx = index(direction);
                guardSeen_[idx] = true;
                lastFinishedAt_[idx] = now;
            }
            phase_ = Phase::Idle;
            direction_ = 0;
            clicked_ = false;
            repeated_ = false;
        }
    }

    // Called from UI::service() after LVGL events have been dispatched.
    // Returns -1, +1 or 0; only this method causes a preset browse step.
    int service(uint32_t now, bool rawPressed) {
        if(queuedStep_ != 0) {
            const int step = queuedStep_;
            queuedStep_ = 0;
            return step;
        }

        if(phase_ == Phase::Settling) {
            if(!rawPressed && elapsed(now, releasedAt_) >= RELEASE_SETTLE_MS) {
                finishRelease();
                if(queuedStep_ != 0) {
                    const int step = queuedStep_;
                    queuedStep_ = 0;
                    return step;
                }
            }
            return 0;
        }

        if(phase_ != Phase::Down || !rawPressed) return 0;
        const uint32_t held = elapsed(now, pressedAt_);
        if(held < HOLD_START_MS) return 0;

        uint32_t interval = 180;
        if(held >= 1500) interval = 65;
        else if(held >= 900) interval = 105;

        if(!repeated_ || elapsed(now, lastRepeatAt_) >= interval) {
            lastRepeatAt_ = now;
            repeated_ = true;
            return direction_;
        }
        return 0;
    }

private:
    enum class Phase : uint8_t { Idle, Down, Settling, Suppressed };
    static uint32_t elapsed(uint32_t now, uint32_t then) { return now - then; }
    static unsigned index(int direction) { return direction < 0 ? 0U : 1U; }

    void finishRelease() {
        if(phase_ != Phase::Settling) return;
        const unsigned idx = index(direction_);
        guardSeen_[idx] = true;
        lastFinishedAt_[idx] = releasedAt_;
        if(clicked_ && !repeated_) queuedStep_ = direction_;
        phase_ = Phase::Idle;
        direction_ = 0;
        clicked_ = false;
        repeated_ = false;
    }

    Phase phase_ = Phase::Idle;
    int direction_ = 0;
    int queuedStep_ = 0;
    uint32_t pressedAt_ = 0;
    uint32_t releasedAt_ = 0;
    uint32_t lastRepeatAt_ = 0;
    uint32_t lastFinishedAt_[2] = {0, 0};
    bool guardSeen_[2] = {false, false};
    bool clicked_ = false;
    bool repeated_ = false;
};

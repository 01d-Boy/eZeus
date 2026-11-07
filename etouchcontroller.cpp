#include "etouchcontroller.h"

#include <cmath>
#include <algorithm>

bool eTouchController::handleEvent(const SDL_Event& e) {
    switch(e.type) {
    case SDL_FINGERDOWN:
        fingerDown(e.tfinger);
        return true;
    case SDL_FINGERMOTION:
        fingerMotion(e.tfinger);
        return true;
    case SDL_FINGERUP:
        fingerUp(e.tfinger);
        return true;
    default:
        return false;
    }
}

void eTouchController::sendMove(const int x, const int y) {
    mCursorX = x;
    mCursorY = y;
    if(mSink.fMove) mSink.fMove(eMouseEvent(x, y, false, false, mButtons));
}

void eTouchController::sendPress(const int x, const int y,
                                 const eMouseButton b) {
    mCursorX = x;
    mCursorY = y;
    mButtons = b | mButtons;
    if(mSink.fPress) mSink.fPress(eMouseEvent(x, y, false, false, mButtons, b));
}

void eTouchController::sendRelease(const int x, const int y,
                                   const eMouseButton b) {
    mCursorX = x;
    mCursorY = y;
    mButtons = mButtons & ~b;
    if(mSink.fRelease) mSink.fRelease(eMouseEvent(x, y, false, false, mButtons, b));
}

void eTouchController::sendWheel(const int x, const int y, const int dy) {
    if(mSink.fWheel) {
        mSink.fWheel(eMouseWheelEvent(x, y, false, false, mButtons, dy));
    }
}

void eTouchController::gestureCenter(int& cx, int& cy, double& dist) const {
    // use the first two fingers
    int i = 0;
    int x[2] = {0, 0};
    int y[2] = {0, 0};
    for(const auto& f : mFingers) {
        x[i] = f.second.fX;
        y[i] = f.second.fY;
        if(++i == 2) break;
    }
    cx = (x[0] + x[1])/2;
    cy = (y[0] + y[1])/2;
    const double dx = x[0] - x[1];
    const double dy = y[0] - y[1];
    dist = std::sqrt(dx*dx + dy*dy);
}

void eTouchController::beginGesture() {
    // cancel whatever the first finger was doing
    if(mMode == Mode::drag) {
        sendRelease(mCursorX, mCursorY, eMouseButton::left);
    }
    mMode = Mode::gesture;
    gestureCenter(mLastCX, mLastCY, mLastDist);
    mPinchAcc = 0;
    mScrollAcc = 0;
    mGesturePan = mSink.fPanMode ? mSink.fPanMode() : false;
    if(mGesturePan) {
        sendMove(mLastCX, mLastCY);
        sendPress(mLastCX, mLastCY, eMouseButton::middle);
    }
}

void eTouchController::endGesture() {
    if(mGesturePan) {
        sendRelease(mCursorX, mCursorY, eMouseButton::middle);
        mGesturePan = false;
    }
}

void eTouchController::fingerDown(const SDL_TouchFingerEvent& f) {
    const int x = static_cast<int>(f.x*mW);
    const int y = static_cast<int>(f.y*mH);
    mFingers[f.fingerId] = {x, y};
    if(mFingers.size() == 1) {
        mMode = Mode::pending;
        mMainFinger = f.fingerId;
        mStartX = x;
        mStartY = y;
        mStartTime = SDL_GetTicks();
        sendMove(x, y);
    } else if(mFingers.size() == 2) {
        beginGesture();
    }
}

void eTouchController::fingerMotion(const SDL_TouchFingerEvent& f) {
    const auto it = mFingers.find(f.fingerId);
    if(it == mFingers.end()) return;
    const int x = static_cast<int>(f.x*mW);
    const int y = static_cast<int>(f.y*mH);
    it->second = {x, y};

    if(mMode == Mode::gesture) {
        if(mFingers.size() < 2) return;
        int cx, cy;
        double dist;
        gestureCenter(cx, cy, dist);
        if(mGesturePan) {
            sendMove(cx, cy);
        } else {
            mScrollAcc += cy - mLastCY;
            while(std::abs(mScrollAcc) >= mScrollStep) {
                const int s = mScrollAcc > 0 ? 1 : -1;
                sendWheel(cx, cy, s);
                mScrollAcc -= s*mScrollStep;
            }
        }
        mPinchAcc += dist - mLastDist;
        while(std::abs(mPinchAcc) >= mPinchStep) {
            const int s = mPinchAcc > 0 ? 1 : -1;
            // the game accumulates the wheel and acts on the second event
            sendWheel(cx, cy, 4*s);
            sendWheel(cx, cy, 4*s);
            mPinchAcc -= s*mPinchStep;
        }
        mLastCX = cx;
        mLastCY = cy;
        mLastDist = dist;
        return;
    }

    if(f.fingerId != mMainFinger) return;

    if(mMode == Mode::pending) {
        const int dx = x - mStartX;
        const int dy = y - mStartY;
        if(dx*dx + dy*dy >= mDragThreshold*mDragThreshold) {
            mMode = Mode::drag;
            sendMove(mStartX, mStartY);
            sendPress(mStartX, mStartY, eMouseButton::left);
            sendMove(x, y);
        }
    } else if(mMode == Mode::drag) {
        sendMove(x, y);
    }
}

void eTouchController::fingerUp(const SDL_TouchFingerEvent& f) {
    const auto it = mFingers.find(f.fingerId);
    if(it == mFingers.end()) return;
    const int x = it->second.fX;
    const int y = it->second.fY;
    mFingers.erase(it);

    if(mMode == Mode::gesture) {
        if(mFingers.size() < 2) endGesture();
        if(mFingers.empty()) mMode = Mode::idle;
        else mMode = Mode::gesture; // wait until all fingers are lifted
        return;
    }

    if(mMode == Mode::pending) {
        sendMove(x, y);
        sendPress(x, y, eMouseButton::left);
        sendRelease(x, y, eMouseButton::left);
    } else if(mMode == Mode::drag) {
        sendMove(x, y);
        sendRelease(x, y, eMouseButton::left);
    }
    mMode = Mode::idle;
}

void eTouchController::update() {
    if(mMode != Mode::pending) return;
    const Uint32 now = SDL_GetTicks();
    if(now - mStartTime < 500) return;
    mMode = Mode::longPress;
    sendMove(mStartX, mStartY);
    sendPress(mStartX, mStartY, eMouseButton::right);
    sendRelease(mStartX, mStartY, eMouseButton::right);
}

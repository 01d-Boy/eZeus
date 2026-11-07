#ifndef ETOUCHCONTROLLER_H
#define ETOUCHCONTROLLER_H

#include <SDL2/SDL.h>

#include <functional>
#include <map>

#include "widgets/emouseevent.h"

// Translates SDL finger events into the mouse events eZeus understands.
//
//  * one finger tap          -> left click
//  * one finger drag         -> left button drag (roads, zones, selection...)
//  * one finger long press   -> right click (cancel / order)
//  * two finger drag         -> map panning (middle button drag),
//                               or wheel scrolling in menus
//  * two finger pinch        -> zoom (wheel)
class eTouchController {
public:
    struct Sink {
        std::function<void(const eMouseEvent&)> fMove;
        std::function<void(const eMouseEvent&)> fPress;
        std::function<void(const eMouseEvent&)> fRelease;
        std::function<void(const eMouseWheelEvent&)> fWheel;
        // true when two finger dragging should pan the map
        // (otherwise it scrolls with the wheel)
        std::function<bool()> fPanMode;
    };

    eTouchController(const Sink& sink) : mSink(sink) {}

    void setScreenSize(const int w, const int h) {
        mW = w;
        mH = h;
        mDragThreshold = std::max(10, h/80);
        mPinchStep = std::max(40, h/20);
        mScrollStep = std::max(30, h/40);
    }

    // returns true if the event was consumed
    bool handleEvent(const SDL_Event& e);
    // call once per frame (long press detection)
    void update();

    int cursorX() const { return mCursorX; }
    int cursorY() const { return mCursorY; }

private:
    enum class Mode { idle, pending, drag, longPress, gesture };

    struct Finger {
        int fX;
        int fY;
    };

    void fingerDown(const SDL_TouchFingerEvent& f);
    void fingerMotion(const SDL_TouchFingerEvent& f);
    void fingerUp(const SDL_TouchFingerEvent& f);

    void sendMove(const int x, const int y);
    void sendPress(const int x, const int y, const eMouseButton b);
    void sendRelease(const int x, const int y, const eMouseButton b);
    void sendWheel(const int x, const int y, const int dy);

    void gestureCenter(int& cx, int& cy, double& dist) const;
    void beginGesture();
    void endGesture();

    Sink mSink;
    int mW = 1;
    int mH = 1;
    int mDragThreshold = 12;
    int mPinchStep = 60;
    int mScrollStep = 40;

    Mode mMode = Mode::idle;
    std::map<SDL_FingerID, Finger> mFingers;

    SDL_FingerID mMainFinger = 0;
    int mStartX = 0;
    int mStartY = 0;
    Uint32 mStartTime = 0;

    int mCursorX = 0;
    int mCursorY = 0;
    eMouseButton mButtons{eMouseButton::none};

    int mLastCX = 0;
    int mLastCY = 0;
    double mLastDist = 0;
    double mPinchAcc = 0;
    int mScrollAcc = 0;
    bool mGesturePan = false;
};

#endif // ETOUCHCONTROLLER_H

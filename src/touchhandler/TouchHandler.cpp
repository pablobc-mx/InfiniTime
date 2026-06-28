#include "touchhandler/TouchHandler.h"

using namespace Pinetime::Controllers;
using namespace Pinetime::Applications;

namespace {
  TouchEvents ConvertGesture(Pinetime::Drivers::Cst816S::Gestures gesture) {
    switch (gesture) {
      case Pinetime::Drivers::Cst816S::Gestures::SingleTap:
        return TouchEvents::Tap;
      case Pinetime::Drivers::Cst816S::Gestures::LongPress:
        return TouchEvents::LongTap;
      case Pinetime::Drivers::Cst816S::Gestures::DoubleTap:
        return TouchEvents::DoubleTap;
      case Pinetime::Drivers::Cst816S::Gestures::SlideRight:
        return TouchEvents::SwipeRight;
      case Pinetime::Drivers::Cst816S::Gestures::SlideLeft:
        return TouchEvents::SwipeLeft;
      case Pinetime::Drivers::Cst816S::Gestures::SlideDown:
        return TouchEvents::SwipeDown;
      case Pinetime::Drivers::Cst816S::Gestures::SlideUp:
        return TouchEvents::SwipeUp;
      case Pinetime::Drivers::Cst816S::Gestures::None:
      default:
        return TouchEvents::None;
    }
  }
}

Pinetime::Applications::TouchEvents TouchHandler::GestureGet() {
  auto returnGesture = gesture;
  gesture = Pinetime::Applications::TouchEvents::None;
  return returnGesture;
}

bool TouchHandler::ProcessTouchInfo(Drivers::Cst816S::TouchInfos info) {
  if (!info.isValid) {
    return false;
  }

  // The P8b's CST716 in REPORT mode generates multiple "none" gesture events
  // with touching=true during the gesture, then one gesture event (e.g. "slide")
  // with touching=true at the end. Set touching=false after a gesture to signal release.
  if (info.gesture != Pinetime::Drivers::Cst816S::Gestures::None) {
    gesture = ConvertGesture(info.gesture);
    info.touching = false;
  }

  if (!info.touching) {
    gestureReleased = true;
  }

  currentTouchPoint = {info.x, info.y, info.touching};

  return true;
}

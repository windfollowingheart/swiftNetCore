#pragma once

class Timer;

///
/// An opaque identifier, for canceling Timer.
///
class TimerId
{
public:
  TimerId()
      : timer_(nullptr),
        sequence_(0)
  {
  }

  TimerId(Timer *timer, int seq)
      : timer_(timer),
        sequence_(seq)
  {
  }

  // default copy-ctor, dtor and assignment are okay

  friend class TimerQueue;

private:
  Timer *timer_;
  int sequence_;
};

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TimedCanvasDrawListener_h
#define TimedCanvasDrawListener_h

#include <memory>
#include "base/memory/weak_ptr.h"
#include "core/html/canvas/CanvasDrawListener.h"
#include "platform/Timer.h"
#include "platform/heap/Handle.h"

#include "public/platform/WebCanvasCaptureHandler.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace blink {
class ExecutionContext;

class TimedCanvasDrawListener final
    : public GarbageCollectedFinalized<TimedCanvasDrawListener>,
      public CanvasDrawListener {
  USING_GARBAGE_COLLECTED_MIXIN(TimedCanvasDrawListener);

 public:
  ~TimedCanvasDrawListener() override;
  static TimedCanvasDrawListener* Create(
      std::unique_ptr<WebCanvasCaptureHandler>,
      double frame_rate,
      ExecutionContext*);
  void SendNewFrame(
      sk_sp<SkImage>,
      base::WeakPtr<WebGraphicsContext3DProviderWrapper>) override;

  void Trace(blink::Visitor* visitor) override {}

 private:
  TimedCanvasDrawListener(std::unique_ptr<WebCanvasCaptureHandler>,
                          double frame_rate,
                          ExecutionContext*);
  // Implementation of TimerFiredFunction.
  void RequestFrameTimerFired(TimerBase*);

  TimeDelta frame_interval_;
  TaskRunnerTimer<TimedCanvasDrawListener> request_frame_timer_;
};

}  // namespace blink

#endif

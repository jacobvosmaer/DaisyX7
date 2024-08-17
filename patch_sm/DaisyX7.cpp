#include "../engine.h"
#include "daisy_patch_sm.h"

using namespace daisy;
using namespace patch_sm;

DaisyPatchSM hw;
Switch button, toggle;

#define nelem(x) (sizeof(x) / sizeof(*x))
#undef assert
#define assert(x)                                                              \
  if (!(x))                                                                    \
  __builtin_trap()

struct ops ops;
struct egs egs[NUM_OPS];

float cv(int n) {
  n--;
  assert(n >= CV_1 && n <= CV_8);
  return hw.GetAdcValue(n);
}

float multcoarse(float val) {
  int m = 31.f * val;
  return m ? (float)m : 0.5f;
}

float pinchzero(float x, float ignore) {
  x *= (1.0 + ignore);
  if (x < ignore)
    x = 0;
  else
    x -= ignore;
  return x;
}

float hztofreq(float hz) { return hz / hw.AudioSampleRate(); }

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  int i, j;
  float pitchhz, op2base;

  hw.ProcessAllControls();
  button.Debounce();
  toggle.Debounce();

  pitchhz = 20.0 * powf(2.0, 5.0 * cv(5) + 10.0 * cv(1));
  ops.feedback_level = cv(2) + cv(8);
  egs[OP1].amp = 1.0;
  egs[OP1].freq = hztofreq(pitchhz);
  egs[OP2].amp = pinchzero(cv(7), 0.001);

  op2base = toggle.Pressed() ? pitchhz : 4.0;
  egs[OP2].freq =
      hztofreq(op2base * (multcoarse(cv(3)) + 2.0 * pinchzero(cv(4), 0.004)));

  for (j = 0; j < (int)size; j += 2) {
    for (i = 0; i < NUM_OPS; i++) {
      ops_update(&ops, egs, i);
      out[j] = out[j + 1] = ops.mem;
    }
  }
}

int main(void) {
  hw.Init();
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
  button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
  toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());
  ops.algo = 1;
  hw.StartAudio(AudioCallback);

  while (1)
    ;
}

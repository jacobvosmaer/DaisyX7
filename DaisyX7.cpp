#include "algo.h"
#include "daisy_pod.h"
#include <math.h>
#include <string.h>

#define nelem(x) (int)(sizeof(x) / sizeof(*x))
float pi = 3.141592653;

using namespace daisy;

DaisyPod hw;

/* The OPS ASIC (YM2128) implements the digital oscillators of the DX7. */
struct {
  float phase[NUM_OPS];
  float mod;
  float feedback[2];
  float mem;
  int algo;
  int feedbacklevel;
} ops;

/* The EGS ASIC (YM2129) is responsible for providing frequency and amplitude
 * data to the OPS. TODO: modulate frequency and amplitude so we can produce
 * notes instead of drones. */
struct {
  float freq, amp;
} egs[NUM_OPS];

float hztofreq(float hz) { return hz / hw.AudioSampleRate(); }

float ops_feedback(void) { return hw.knob2.Process(); }

struct algorithm ops_algo(int i) {
  return algorithms[ops.algo][i % nelem(algorithms[0])];
}

void ops_update(int i) {
  if (i < 0 || i >= nelem(ops.phase) || ops.algo < 0 ||
      ops.algo >= nelem(algorithms))
    return;

  ops.phase[i] += egs[i].freq;
  if (ops.phase[i] > 1)
    ops.phase[i] -= 2;

  /* Calculate sample for current operator i based on ops.mod from operator i-1.
   * Because of the way the real OPS is implemented, the COM value we need
   * for this is stored on the algorithm of operator i-1. Reminder: the
   * operators are numbered in reverse from the DX7 UI. Index 0 is OP6, 1 is
   * OP5, ..., 5 is OP1. */
  float sample = sinf(pi * (ops.phase[i] + ops.mod)) * egs[i].amp /
                 (float)(1 + ops_algo(i - 1).com);

  /* Calculate ops.mod and ops.mem for use in next call to ops_update() */
  struct algorithm algo = ops_algo(i);
  if (algo.a) {
    ops.feedback[1] = ops.feedback[0];
    ops.feedback[0] = sample;
  }

  float newmem = (float)algo.c * ops.mem + (float)algo.d * sample;

  switch (algo.sel) {
  case 0:
    ops.mod = 0;
    break;
  case 1:
    ops.mod = sample;
    break;
  case 2:
    ops.mod = newmem;
    break;
  case 3:
    ops.mod = ops.mem;
    break;
  case 4:
    ops.mod = ops.feedback[0];
    break;
  case 5:
    ops.mod = ops_feedback() * (ops.feedback[0] + ops.feedback[1]) / 2.0;
    break;
  }

  ops.mem = newmem;
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  hw.ProcessAllControls();
  egs[4].amp = hw.knob1.Process();

  for (int j = 0; j < (int)size; j += 2) {
    for (int i = 0; i < nelem(ops.phase); i++)
      ops_update(i);
    out[j] = out[j + 1] = ops.mem;
  }
}

int main(void) {
  hw.Init();

  ops.algo = 1; /* Algorithm 1 */
  egs[5].freq = hztofreq(220);
  egs[5].amp = 1;
  egs[4].freq = hztofreq(440);
  egs[4].amp = 0.5;

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (1)
    ;
}

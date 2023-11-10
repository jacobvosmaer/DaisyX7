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
 * data to the OPS. TODO: modulate frequency and amplitude so we can produces
 * notes instead of drones. */
struct {
  float freqhz, amp;
} egs[NUM_OPS];

float egs_freq(int op) {
  if (op < 0 || op >= nelem(egs))
    return 0;

  return egs[op].freqhz / hw.AudioSampleRate();
}

float ops_feedback(void) {
  /* TODO return scale factor based on ops.feedback_level */
  return 0;
}

float ops_update(int i) {
  if (i < 0 || i >= nelem(algorithms[0]))
    return 0;

  struct algorithm algo = algorithms[ops.algo][i],
                   previous_algo =
                       algorithms[ops.algo][(i + 1) % nelem(algorithms[0])];

  /* Calculate sample for current operator i based on ops.mod from operator i+1.
   * For technical reasons the COM value we need for this is stored on the
   * algorithm of operator i+1.
   */
  float sample = sinf(pi * (ops.phase[i] + ops.mod)) * egs[i].amp /
                 (float)(1 + previous_algo.com);

  /* Calculate ops.mod and ops.mem for use in next call to ops_update() */

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
  return ops.mem;
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  for (int j = 0; j < (int)size; j += 2) {
    float sample;
    for (int i = nelem(egs); i--;) {
      ops.phase[i] += egs_freq(i);
      if (ops.phase[i] > 1)
        ops.phase[i] -= 2;
      sample = ops_update(i);
    }
    out[j] = out[j + 1] = sample;
  }
}

int main(void) {
  hw.Init();

  ops.algo = 0;        /* Algorithm 1 */
  egs[5].freqhz = 110; /* Operator 1: 220Hz, full volume */
  egs[5].amp = 1;

  hw.StartAudio(AudioCallback);

  while (1)
    ;
}

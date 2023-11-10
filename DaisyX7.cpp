#include "algo.h"
#include "daisy_pod.h"
#include <string.h>

#define nelem(x) (sizeof(x) / sizeof(*x))

using namespace daisy;

DaisyPod hw;

struct {
  float phase[NUM_OPS];
  float previous_sample;
  float feedback[2];
  float mem;
  int algo;
  int feedbacklevel;
} ops;

struct {
  float freqhz, amp;
} egs[NUM_OPS];

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  (void)ops;
  memset(out, 0, size * sizeof(*out));
}

int main(void) {
  hw.Init();

  ops.algo = 1;        /* Algorithm 2 */
  egs[5].freqhz = 220; /* Operator 1: 220Hz, full volume */
  egs[5].amp = 1;
  egs[4].freqhz = 220; /* Operator 2: 220Hz, half volume */
  egs[4].amp = 0.5;

  hw.StartAudio(AudioCallback);

  while (1)
    ;
}

#include "algo.h"
#include "daisy_pod.h"
#include <string.h>

using namespace daisy;

DaisyPod hw;

struct {
  struct {
    float phase;
  } osc[NUM_OPS];
  float previous_sample;
  float feedback[2];
  float mem;
  int algo;
} ops;

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  (void)ops;
  memset(out, 0, size * sizeof(*out));
}

int main(void) {
  hw.Init();
  hw.StartAudio(AudioCallback);

  while (1)
    ;
}

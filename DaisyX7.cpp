#include "daisy_pod.h"
#include <string.h>

using namespace daisy;

DaisyPod hw;

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  memset(out, 0, size * sizeof(*out));
}

int main(void) {
  hw.Init();
  hw.StartAudio(AudioCallback);

  while (1)
    ;
}

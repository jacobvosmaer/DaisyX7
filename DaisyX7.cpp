#include "algo.h"
#include "daisy_field.h"
#include <math.h>
#include <string.h>

#define nelem(x) (int)(sizeof(x) / sizeof(*x))
float pi = 3.141592653, semi_up = 1.05946309436, semi_down = 0.943874312682;

using namespace daisy;

DaisyField hw;

enum { OP6, OP5, OP4, OP3, OP2, OP1 };

/* The OPS ASIC (YM2128) implements the digital oscillators of the DX7. */
struct {
  float phase[NUM_OPS];
  float mod;
  float feedback[2];
  float mem;
  uint8_t algo;
  float feedback_level;
} ops;

/* The EGS ASIC (YM2129) is responsible for providing frequency and amplitude
 * data to the OPS. TODO: modulate frequency and amplitude so we can produce
 * notes instead of drones. */
struct {
  float freq, amp;
} egs[NUM_OPS];

float hztofreq(float hz) { return hz / hw.AudioSampleRate(); }

struct algorithm ops_algo(int i) {
  return algorithms[ops.algo][i % nelem(algorithms[0])];
}

void ops_update(int i) {
  ops.phase[i] += egs[i].freq;
  if (ops.phase[i] > 1)
    ops.phase[i] -= 2;

  /* Calculate sample for current operator i based on ops.mod from operator i-1.
   * Because of the way the real OPS is implemented, the COM value we need
   * for this is stored on the algorithm of operator i-1. Reminder: the
   * operators are numbered in reverse from the DX7 UI. Index 0 is OP6, 1 is
   * OP5, ..., 5 is OP1. */
  float maxmod = 3.85; /* Picked by ear comparing with TX7 */
  float sample = sinf(pi * (ops.phase[i] + maxmod * ops.mod)) * egs[i].amp /
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
    float fbscale = 6.0; /* Picked by ear */
    ops.mod =
        ops.feedback_level * (ops.feedback[0] + ops.feedback[1]) / fbscale;
    break;
  }

  ops.mem = newmem;
}

struct {
  float base;
  float mult[NUM_OPS];
} frequency;

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  hw.ProcessAllControls();

  egs[OP2].amp = hw.knob[0].Process();
  ops.feedback_level = hw.knob[1].Process();
  frequency.base = 20.0 * powf(2.0, 14.0 * hw.knob[7].Process());
  for (int i = 0; i < nelem(frequency.mult); i++)
    egs[i].freq = hztofreq(frequency.mult[i] * frequency.base);

  for (int j = 0; j < (int)size; j += 2) {
    for (int i = 0; i < nelem(ops.phase); i++)
      ops_update(i);
    out[j] = out[j + 1] = ops.mem;
  }
}

enum { columns = 18, rows = 5 };

int main(void) {
  hw.Init();
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  ops.algo = 1;
  egs[OP1].freq = hztofreq(220);
  egs[OP1].amp = 1;

  for (int i = 0; i < nelem(frequency.mult); i++)
    frequency.mult[i] = 1;

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (1) {
    char line[columns + 1];

    hw.display.SetCursor(0, 0);
    snprintf(line, sizeof(line), "algo=%d", ops.algo);
    hw.display.WriteString(line, Font_6x8, true);

    hw.display.Update();

    if (hw.sw[0].Pressed()) {
      hw.led_driver.SetAllTo(1.0f);
    } else {
      hw.led_driver.SetAllTo(0.0f);
    }

    hw.led_driver.SwapBuffersAndTransmit();
  }
}

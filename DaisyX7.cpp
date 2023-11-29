#include "algo.h"
#include "daisy_field.h"
#include <math.h>
#include <string.h>

#define nelem(x) (int)(sizeof(x) / sizeof(*x))
#undef assert
#define assert(x)                                                              \
  if (!(x))                                                                    \
  __builtin_trap()
float pi = 3.141592653;

using namespace daisy;

DaisyField hw;

int keytoggle[16];
int keytoled(int key) { return key >= 8 ? key - 8 : 15 - key; }

struct vknob {
  float init, last;
  int idx;
};

float vknob_raw(struct vknob *vk) { return hw.knob[vk->idx].Process(); }
void vknob_enable(struct vknob *vk) { vk->init = vknob_raw(vk); }
float vknob_value(struct vknob *vk) {
  float current = vknob_raw(vk);
  if (fabsf(vk->init - current) > 0.01)
    vk->last = current;
  return vk->last;
}

struct {
  struct vknob amp[NUM_OPS];
  struct vknob mult[NUM_OPS];
  int op;
} ui;

void ui_init(void) {
  for (int i = 0; i < nelem(ui.amp); i++)
    ui.amp[i].idx = DaisyField::KNOB_1;
  for (int i = 0; i < nelem(ui.mult); i++) {
    ui.mult[i].idx = DaisyField::KNOB_2;
    ui.mult[i].last = 1.f / 31.f;
  }
  ui.amp[0].last = 1;

  for (int i = 0; i < DaisyField::KNOB_LAST; i++)
    hw.knob[i].SetCoeff(0.5f);
}

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

struct algorithm ops_algo(uint8_t i) {
  return algorithms[ops.algo][i % nelem(algorithms[0])];
}

float comtab[] = {1.0 / 1.0, 1.0 / 2.0, 1.0 / 3.0,
                  1.0 / 4.0, 1.0 / 5.0, 1.0 / 6.0};

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
  float sample = sinf(pi * (ops.phase[i] + maxmod * ops.mod)) * egs[i].amp *
                 comtab[ops_algo(i - 1).com];

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
    float fbscale = 1.f / 6.0; /* Picked by ear */
    ops.mod =
        ops.feedback_level * (ops.feedback[0] + ops.feedback[1]) * fbscale;
    break;
  }

  ops.mem = newmem;
}

struct {
  float base;
  float mult[NUM_OPS];
} frequency;

float multcoarse(float val) {
  int m = 31.f * val;
  return m ? (float)m : 0.5f;
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  hw.ProcessAllControls();

  for (int i = 0; i < NUM_OPS; i++) {
    int key = 8 + i;
    int op = 5 - i;
    if (hw.KeyboardRisingEdge(key)) {
      vknob_enable(&ui.amp[i]);
      vknob_enable(&ui.mult[i]);
      keytoggle[key] = 1 - keytoggle[key];
    }
    if (keytoggle[key]) {
      egs[op].amp = vknob_value(&ui.amp[i]);
      frequency.mult[op] = multcoarse(vknob_value(&ui.mult[i]));
    }
  }
  if (hw.sw[0].RisingEdge())
    ops.algo--;
  else if (hw.sw[1].RisingEdge())
    ops.algo++;
  ops.algo %= nelem(algorithms);

  ops.feedback_level = hw.knob[6].Process();
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
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
  ui_init();

  hw.StartAdc();
  hw.StartAudio(AudioCallback);

  while (1) {
    char line[columns + 1];

    hw.display.SetCursor(0, 0);
    snprintf(line, sizeof(line), "algo=%02d", ops.algo + 1);
    hw.display.WriteString(line, Font_6x8, true);
    hw.display.Update();

    /* key 0: B1, key 8: A1 */
    for (int i = 0; i < nelem(keytoggle); i++)
      hw.led_driver.SetLed(keytoled(i), (float)keytoggle[i]);

    hw.led_driver.SwapBuffersAndTransmit();
  }
}

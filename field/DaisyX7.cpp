/* DX7 emulation drone synth on Daisy Field */

#include "../engine.h"
#include "daisy_field.h"
#include <math.h>
#include <string.h>

#define nelem(x) (int)(sizeof(x) / sizeof(*x))
#undef assert
#define assert(x)                                                              \
  if (!(x))                                                                    \
  __builtin_trap()

struct ops ops;
struct egs egs[NUM_OPS];

using namespace daisy;
DaisyField hw;

int keytoggle[16];
int keytoled(int key) { return key >= 8 ? key - 8 : 15 - key; }

/* vknob is a "virtual knob". Multiple virtual knobs can point to the same
 * physical knob. */
struct vknob {
  float init, last;
  int idx;
};

float vknob_raw(struct vknob *vk) { return hw.knob[vk->idx].Process(); }
void vknob_enable(struct vknob *vk) { vk->init = vknob_raw(vk); }

float vknob_value(struct vknob *vk) {
  float current = vknob_raw(vk);
  if (fabsf(vk->init - current) > 0.01) {
    vk->last = current;
    vk->init = -1.0;
  }
  return vk->last;
}

struct {
  struct vknob amp[NUM_OPS];
  struct vknob multcoarse[NUM_OPS];
  struct vknob multfine[NUM_OPS];
  int op;
} ui;

struct {
  float base;
  float mult[NUM_OPS];
  int fixed[NUM_OPS];
} frequency;

void ui_init(void) {
  for (int i = 0; i < nelem(ui.amp); i++)
    ui.amp[i].idx = DaisyField::KNOB_1;
  ui.amp[OP1].last = 1.f;
  for (int i = 0; i < nelem(ui.multcoarse); i++) {
    ui.multcoarse[i].idx = DaisyField::KNOB_2;
    ui.multcoarse[i].last = 1.f / 31.f;
    frequency.mult[i] = 1.f;
  }
  for (int i = 0; i < nelem(ui.multfine); i++)
    ui.multfine[i].idx = DaisyField::KNOB_3;

  for (int i = 0; i < DaisyField::KNOB_LAST; i++)
    hw.knob[i].SetCoeff(0.5f);
}

float multcoarse(float val) {
  int m = 31.f * val;
  return m ? (float)m : 0.5f;
}

int optokey(int op) { return 13 - op; }
float hztofreq(float hz) { return hz / hw.AudioSampleRate(); }

int boot = 1;
void ui_update(void) {
  hw.ProcessAllControls();

  for (int i = 0; i < NUM_OPS; i++) {
    if (hw.KeyboardRisingEdge(optokey(i)) || (boot && i == OP1)) {
      boot = 0;
      ui.op = i;
      vknob_enable(&ui.amp[i]);
      vknob_enable(&ui.multcoarse[i]);
      vknob_enable(&ui.multfine[i]);
      for (int j = 0; j < NUM_OPS; j++)
        keytoggle[optokey(j)] = j == i;
    }
  }

  enum { keyfixed = 7 };
  if (hw.KeyboardRisingEdge(keyfixed))
    frequency.fixed[ui.op] ^= 1;
  keytoggle[keyfixed] = frequency.fixed[ui.op];

  egs[ui.op].amp = vknob_value(&ui.amp[ui.op]);
  frequency.mult[ui.op] = multcoarse(vknob_value(&ui.multcoarse[ui.op])) +
                          2.0 * vknob_value(&ui.multfine[ui.op]);

  if (hw.sw[0].RisingEdge())
    ops.algo--;
  else if (hw.sw[1].RisingEdge())
    ops.algo++;
  ops.algo %= NUM_ALGOS;

  ops.feedback_level = hw.knob[6].Process();
  frequency.base = 20.0 * powf(2.0, 14.0 * hw.knob[7].Process());

  for (int i = 0; i < nelem(frequency.mult); i++) {
    float base = (frequency.fixed[i]) ? 4.f : frequency.base;
    egs[i].freq = hztofreq(frequency.mult[i] * base);
  }
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size) {
  ui_update();

  for (int j = 0; j < (int)size; j += 2) {
    for (int i = 0; i < nelem(ops.phase); i++)
      ops_update(&ops, egs, i);
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
    snprintf(line, sizeof(line), "algo=%02d fb=%02d", ops.algo + 1,
             (int)(ops.feedback_level * 100.f));
    hw.display.WriteString(line, Font_6x8, true);
    for (int i = 0; i < NUM_OPS; i++) {
      int x = 9 * (i + 1), op = 5 - i;
      hw.display.SetCursor(0, x);
      int freq = frequency.mult[op] * 100.f;
      int amp = egs[op].amp * 100.f;
      if (amp == 100)
        amp = 99;
      snprintf(line, sizeof(line), "OP%d F=%2d.%02d%c L=%02d", i + 1,
               freq / 100, freq % 100, frequency.fixed[op] ? 'f' : 'r', amp);
      hw.display.WriteString(line, Font_6x8, true);
    }
    hw.display.Update();

    /* key 0: B1, key 8: A1 */
    for (int i = 0; i < nelem(keytoggle); i++)
      hw.led_driver.SetLed(keytoled(i), (float)keytoggle[i]);

    hw.led_driver.SwapBuffersAndTransmit();
  }
}

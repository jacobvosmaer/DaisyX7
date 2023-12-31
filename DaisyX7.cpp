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

/* The OPS ASIC (YM2128) implements the digital oscillators of the DX7. The ops
 * struct contains the internal state of our simulation of the OPS, restricted
 * to just one voice because this program is monophonic. */
struct {
  float phase[NUM_OPS];
  float mod;
  float feedback[2];
  float mem;
  uint8_t algo;
  float feedback_level;
} ops;

/* The EGS ASIC (YM2129) is responsible for providing frequency and amplitude
 * data to the OPS. This program does not simulate the EGS. From the point of
 * view of the OPS, the EGS is a data bus that delivers pitch and amplitude data
 * for each oscillator. That is also all what we use the egs struct below for.
 */
struct {
  float freq, amp;
} egs[NUM_OPS];

struct algorithm ops_algo(uint8_t i) {
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
  float comtab[] = {1.0 / 1.0, 1.0 / 2.0, 1.0 / 3.0,
                    1.0 / 4.0, 1.0 / 5.0, 1.0 / 6.0};
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
  ops.algo %= nelem(algorithms);

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

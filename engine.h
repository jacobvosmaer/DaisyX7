#ifndef ENGINE_H
#define ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

enum { OP6, OP5, OP4, OP3, OP2, OP1, NUM_OPS };
enum { NUM_ALGOS = 32 };

/* The OPS ASIC (YM2128) implements the digital oscillators of the DX7. The ops
 * struct contains the internal state of our simulation of the OPS, restricted
 * to just one voice because this program is monophonic. */

struct ops {
  float phase[NUM_OPS];
  float mod;
  float feedback[2];
  float mem;
  uint8_t algo;
  float feedback_level;
};

/* The EGS ASIC (YM2129) is responsible for providing frequency and amplitude
 * data to the OPS. This program does not simulate the EGS. From the point of
 * view of the OPS, the EGS is a data bus that delivers pitch and amplitude data
 * for each oscillator. That is also all what we use the egs struct below for.
 */

struct egs {
  float freq, amp;
};

void ops_update(struct ops *ops, struct egs *egs, int i);

#ifdef __cplusplus
}
#endif

#endif

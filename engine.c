#include "engine.h"
#include "algo.h"

#define nelem(x) (int)(sizeof(x) / sizeof(*x))
float pi = 3.141592653;

struct algorithm ops_algo(struct ops *ops, uint8_t i) {
  return algorithms[ops->algo][i % nelem(algorithms[0])];
}

void ops_update(struct ops *ops, struct egs *egs, int i) {
  ops->phase[i] += egs[i].freq;
  if (ops->phase[i] > 1)
    ops->phase[i] -= 2;

  /* Calculate sample for current operator i based on ops->mod from operator
   * i-1. Because of the way the real OPS is implemented, the COM value we need
   * for this is stored on the algorithm of operator i-1. Reminder: the
   * operators are numbered in reverse from the DX7 UI. Index 0 is OP6, 1 is
   * OP5, ..., 5 is OP1. */
  float maxmod = 3.85; /* Picked by ear comparing with TX7 */
  float comtab[] = {1.0 / 1.0, 1.0 / 2.0, 1.0 / 3.0,
                    1.0 / 4.0, 1.0 / 5.0, 1.0 / 6.0};
  float sample = sinf(pi * (ops->phase[i] + maxmod * ops->mod)) * egs[i].amp *
                 comtab[ops_algo(ops, i - 1).com];

  /* Calculate ops->mod and ops->mem for use in next call to ops_update() */
  struct algorithm algo = ops_algo(ops, i);
  if (algo.a) {
    ops->feedback[1] = ops->feedback[0];
    ops->feedback[0] = sample;
  }

  float newmem = (float)algo.c * ops->mem + (float)algo.d * sample;

  switch (algo.sel) {
  case 0:
    ops->mod = 0;
    break;
  case 1:
    ops->mod = sample;
    break;
  case 2:
    ops->mod = newmem;
    break;
  case 3:
    ops->mod = ops->mem;
    break;
  case 4:
    ops->mod = ops->feedback[0];
    break;
  case 5:
    float fbscale = 1.f / 6.0; /* Picked by ear */
    ops->mod =
        ops->feedback_level * (ops->feedback[0] + ops->feedback[1]) * fbscale;
    break;
  }

  ops->mem = newmem;
}

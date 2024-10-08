#ifndef ALGO_H
#define ALGO_H

#include "engine.h"

/* Algorithm data comes from
 * https://www.righto.com/2021/12/yamaha-dx7-chip-reverse-engineering.html#fn:alg-table.
 * It's ordered from OP6 down to OP1. */
static struct algorithm {
  int sel, a, c, d, com;
} algorithms[][NUM_OPS] = {
    {{1, 1, 0, 0, 0},
     {1, 0, 0, 0, 0},
     {1, 0, 0, 0, 1},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 1},
     {5, 0, 1, 1, 0}},
    {{1, 0, 0, 0, 0},
     {1, 0, 0, 0, 0},
     {1, 0, 0, 0, 1},
     {5, 0, 0, 1, 0},
     {1, 1, 1, 0, 1},
     {0, 0, 1, 1, 0}},
    {{1, 1, 0, 0, 0},
     {1, 0, 0, 0, 1},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 0},
     {1, 0, 1, 0, 1},
     {5, 0, 1, 1, 0}},
    {{1, 0, 0, 0, 0},
     {1, 0, 0, 0, 1},
     {0, 1, 0, 1, 0},
     {1, 0, 1, 0, 0},
     {1, 0, 1, 0, 1},
     {5, 0, 1, 1, 0}},
    {{1, 1, 0, 0, 2},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 2},
     {0, 0, 1, 1, 0},
     {1, 0, 1, 0, 2},
     {5, 0, 1, 1, 0}},
    {{1, 0, 0, 0, 2},
     {0, 1, 0, 1, 0},
     {1, 0, 1, 0, 2},
     {0, 0, 1, 1, 0},
     {1, 0, 1, 0, 2},
     {5, 0, 1, 1, 0}},
    {{1, 1, 0, 0, 0},
     {0, 0, 0, 1, 0},
     {2, 0, 1, 1, 1},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 1},
     {5, 0, 1, 1, 0}},
    {{1, 0, 0, 0, 0},
     {5, 0, 0, 1, 0},
     {2, 1, 1, 1, 1},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 1},
     {0, 0, 1, 1, 0}},
    {{1, 0, 0, 0, 0},
     {0, 0, 0, 1, 0},
     {2, 0, 1, 1, 1},
     {5, 0, 0, 1, 0},
     {1, 1, 1, 0, 1},
     {0, 0, 1, 1, 0}},
    {{0, 0, 0, 1, 0},
     {2, 0, 1, 1, 1},
     {5, 0, 0, 1, 0},
     {1, 1, 1, 0, 0},
     {1, 0, 1, 0, 1},
     {0, 0, 1, 1, 0}},
    {{0, 1, 0, 1, 0},
     {2, 0, 1, 1, 1},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 0},
     {1, 0, 1, 0, 1},
     {5, 0, 1, 1, 0}},
    {{0, 0, 0, 1, 0},
     {0, 0, 1, 1, 0},
     {2, 0, 1, 1, 1},
     {5, 0, 0, 1, 0},
     {1, 1, 1, 0, 1},
     {0, 0, 1, 1, 0}},
    {{0, 1, 0, 1, 0},
     {0, 0, 1, 1, 0},
     {2, 0, 1, 1, 1},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 1},
     {5, 0, 1, 1, 0}},
    {{0, 1, 0, 1, 0},
     {2, 0, 1, 1, 0},
     {1, 0, 0, 0, 1},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 1},
     {5, 0, 1, 1, 0}},
    {{0, 0, 0, 1, 0},
     {2, 0, 1, 1, 0},
     {1, 0, 0, 0, 1},
     {5, 0, 0, 1, 0},
     {1, 1, 1, 0, 1},
     {0, 0, 1, 1, 0}},
    {{1, 1, 0, 0, 0},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 0},
     {0, 0, 1, 1, 0},
     {2, 0, 1, 1, 0},
     {5, 0, 0, 1, 0}},
    {{1, 0, 0, 0, 0},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 0},
     {5, 0, 1, 1, 0},
     {2, 1, 1, 1, 0},
     {0, 0, 0, 1, 0}},
    {{1, 0, 0, 0, 0},
     {1, 0, 0, 0, 0},
     {5, 0, 0, 1, 0},
     {0, 1, 1, 1, 0},
     {2, 0, 1, 1, 0},
     {0, 0, 0, 1, 0}},
    {{1, 1, 0, 0, 2},
     {4, 0, 0, 1, 2},
     {0, 0, 1, 1, 0},
     {1, 0, 1, 0, 0},
     {1, 0, 1, 0, 2},
     {5, 0, 1, 1, 0}},
    {{0, 0, 0, 1, 0},
     {2, 0, 1, 1, 2},
     {5, 0, 0, 1, 0},
     {1, 1, 1, 0, 2},
     {4, 0, 1, 1, 2},
     {0, 0, 1, 1, 0}},
    {{1, 0, 0, 1, 3},
     {3, 0, 0, 1, 3},
     {5, 0, 1, 1, 0},
     {1, 1, 1, 0, 3},
     {4, 0, 1, 1, 3},
     {0, 0, 1, 1, 0}},
    {{1, 1, 0, 0, 3},
     {4, 0, 0, 1, 3},
     {4, 0, 1, 1, 3},
     {0, 0, 1, 1, 0},
     {1, 0, 1, 0, 3},
     {5, 0, 1, 1, 0}},
    {{1, 1, 0, 0, 3},
     {4, 0, 0, 1, 3},
     {0, 0, 1, 1, 0},
     {1, 0, 1, 0, 3},
     {0, 0, 1, 1, 3},
     {5, 0, 1, 1, 0}},
    {{1, 1, 0, 0, 4},
     {4, 0, 0, 1, 4},
     {4, 0, 1, 1, 4},
     {0, 0, 1, 1, 4},
     {0, 0, 1, 1, 4},
     {5, 0, 1, 1, 0}},
    {{1, 1, 0, 0, 4},
     {4, 0, 0, 1, 4},
     {0, 0, 1, 1, 4},
     {0, 0, 1, 1, 4},
     {0, 0, 1, 1, 4},
     {5, 0, 1, 1, 0}},
    {{0, 1, 0, 1, 0},
     {2, 0, 1, 1, 2},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 2},
     {0, 0, 1, 1, 2},
     {5, 0, 1, 1, 0}},
    {{0, 0, 0, 1, 0},
     {2, 0, 1, 1, 2},
     {5, 0, 0, 1, 0},
     {1, 1, 1, 0, 2},
     {0, 0, 1, 1, 2},
     {0, 0, 1, 1, 0}},
    {{5, 0, 0, 1, 0},
     {1, 1, 1, 0, 0},
     {1, 0, 1, 0, 2},
     {0, 0, 1, 1, 0},
     {1, 0, 1, 0, 2},
     {0, 0, 1, 1, 2}},
    {{1, 1, 0, 0, 3},
     {0, 0, 0, 1, 0},
     {1, 0, 1, 0, 3},
     {0, 0, 1, 1, 3},
     {0, 0, 1, 1, 3},
     {5, 0, 1, 1, 0}},
    {{5, 0, 0, 1, 0},
     {1, 1, 1, 0, 0},
     {1, 0, 1, 0, 3},
     {0, 0, 1, 1, 3},
     {0, 0, 1, 1, 3},
     {0, 0, 1, 1, 3}},
    {{1, 1, 0, 0, 4},
     {0, 0, 0, 1, 4},
     {0, 0, 1, 1, 4},
     {0, 0, 1, 1, 4},
     {0, 0, 1, 1, 4},
     {5, 0, 1, 1, 0}},
    {{0, 1, 0, 1, 5},
     {0, 0, 1, 1, 5},
     {0, 0, 1, 1, 5},
     {0, 0, 1, 1, 5},
     {0, 0, 1, 1, 5},
     {5, 0, 1, 1, 5}},
};
#endif

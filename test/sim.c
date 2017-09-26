
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Options: backend -t c-sim -X -T -X template.c -o sim.c test.hdli

// S = A & B
// Q = S | C
// R = S | D

struct sim_state {
    bool in_A, in_B, in_C, in_D;
    bool out_Q, out_R;
    bool i[1];
};

struct sim_state *sim_create() {
    return calloc(1, sizeof(struct sim_state));
}

void sim_destroy(struct sim_state *state) {
    free(state);
}

void sim_cycle(struct sim_state *state) {
    // In: A, B, C, D
    // Out: Q, R
    // Stmts:
    //   6 = 0 & 1
    //   4 = 6 | 2
    //   5 = 6 | 3

    bool *i = state->i;

    bool o0;

    //o6 = i0 && i1;
    o0 = state->in_A && state->in_B;
    //o4 = i6 || i2;
    state->out_Q = i[0] || state->in_C;
    //o5 = i6 || i3;
    state->out_R = i[0] || state->in_D;

    i[0] = o0;
}

void sim_dump(struct sim_state *state) {
    printf("In: A=%i, B=%i, C=%i, D=%i\n", state->in_A, state->in_B, state->in_C, state->in_D);
    printf("Out: Q=%i, R=%i\n", state->out_Q, state->out_R);
    printf("Internal: i0=%i\n", state->i[0]);
}

int main() {
    struct sim_state *state = sim_create();

    sim_dump(state);

    state->in_A = true;
    state->in_B = true;
    state->in_C = false;
    state->in_D = true;
    sim_dump(state);

    for (int i = 0; i < 3; i++) {
        sim_cycle(state);
        sim_dump(state);
    }

    sim_destroy(state);

    return 0;
}
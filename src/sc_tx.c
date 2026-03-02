/*---------------------------------------------------------------------------*\

  sc_tx.c

  Single-carrier BBFM modem transmitter command-line tool.
  Reads 80 float symbols from stdin, writes 384 int16 audio samples to stdout.

  Usage:
    cat latents.f32 | sc_tx > audio.raw
    bbfm_enc | sc_tx > audio.raw

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "sc_modem.h"

int main(void) {
    sc_modem_tx *tx = sc_tx_create();
    if (!tx) {
        fprintf(stderr, "sc_tx: failed to create TX modem\n");
        return 1;
    }

    float symbols_in[SC_NPAYLOAD];
    int16_t samples_out[SC_NSAMPLES];
    int frames = 0;

    while (fread(symbols_in, sizeof(float), SC_NPAYLOAD, stdin) == SC_NPAYLOAD) {
        sc_tx_process(tx, samples_out, symbols_in);
        fwrite(samples_out, sizeof(int16_t), SC_NSAMPLES, stdout);
        frames++;
    }

    fprintf(stderr, "sc_tx: %d frames\n", frames);
    sc_tx_destroy(tx);
    return 0;
}

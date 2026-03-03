/*---------------------------------------------------------------------------*\

  sc_tx.c

  Single-carrier BBFM modem transmitter command-line tool.
  Reads 80 float symbols from stdin, writes 384 int16 audio samples to stdout.

  Usage:
    cat latents.f32 | sc_tx > audio.raw
    bbfm_enc | sc_tx > audio.raw
    bbfm_enc | sc_tx --preemph > audio.raw
    bbfm_enc | sc_tx --fcentre 1200 > audio.raw

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sc_modem.h"

int main(int argc, char *argv[]) {
    int preemph = 0;
    float fcentre = SC_FCENTRE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--preemph") == 0) {
            preemph = 1;
        } else if (strcmp(argv[i], "--fcentre") == 0 && i + 1 < argc) {
            fcentre = atof(argv[++i]);
        } else {
            fprintf(stderr, "usage: sc_tx [--preemph] [--fcentre Hz]\n");
            return 1;
        }
    }

    sc_modem_tx *tx = sc_tx_create_fcentre(fcentre);
    if (!tx) {
        fprintf(stderr, "sc_tx: failed to create TX modem\n");
        return 1;
    }

    /* De-emphasis filter state (75us time constant single-pole IIR low-pass).
       Applied to TX output to pre-compensate for Baofeng TX pre-emphasis. */
    float tau = 75e-6f;
    float alpha = 1.0f / (1.0f + SC_FS * tau);
    float deemph_y = 0.0f;

    float symbols_in[SC_NPAYLOAD];
    int16_t samples_out[SC_NSAMPLES];
    int frames = 0;

    while (fread(symbols_in, sizeof(float), SC_NPAYLOAD, stdin) == SC_NPAYLOAD) {
        sc_tx_process(tx, samples_out, symbols_in);

        if (preemph) {
            for (int i = 0; i < SC_NSAMPLES; i++) {
                float x = (float)samples_out[i];
                deemph_y = alpha * x + (1.0f - alpha) * deemph_y;
                float y = deemph_y;
                if (y > 32767.0f) y = 32767.0f;
                if (y < -32768.0f) y = -32768.0f;
                samples_out[i] = (int16_t)y;
            }
        }

        fwrite(samples_out, sizeof(int16_t), SC_NSAMPLES, stdout);
        frames++;
    }

    fprintf(stderr, "sc_tx: %d frames, fcentre=%.0f preemph=%d\n", frames, fcentre, preemph);
    sc_tx_destroy(tx);
    return 0;
}

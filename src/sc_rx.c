/*---------------------------------------------------------------------------*\

  sc_rx.c

  Single-carrier BBFM modem receiver command-line tool.
  Reads int16 audio samples from stdin, writes 80 float symbols to stdout
  when synchronized.

  Usage:
    cat audio.raw | sc_rx > latents.f32
    sc_tx | sc_rx > latents_out.f32
    sc_tx --preemph | sc_rx --preemph > latents_out.f32

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
            fprintf(stderr, "usage: sc_rx [--preemph] [--fcentre Hz]\n");
            return 1;
        }
    }

    sc_modem_rx *rx = sc_rx_create_fcentre(fcentre);
    if (!rx) {
        fprintf(stderr, "sc_rx: failed to create RX modem\n");
        return 1;
    }

    /* Pre-emphasis filter state (75us time constant single-pole IIR high-pass).
       Applied to RX input to compensate for Baofeng RX de-emphasis. */
    float tau = 75e-6f;
    float alpha = 1.0f / (1.0f + SC_FS * tau);
    float preemph_x_prev = 0.0f;
    float preemph_y = 0.0f;

    int16_t samples_in[SC_NIN_MAX];
    float symbols_out[SC_NPAYLOAD];
    int frames = 0;
    int synced_frames = 0;

    while (1) {
        int nin = sc_rx_nin(rx);
        size_t n = fread(samples_in, sizeof(int16_t), nin, stdin);
        if ((int)n != nin) break;

        if (preemph) {
            for (int i = 0; i < nin; i++) {
                float x = (float)samples_in[i];
                preemph_y = x - preemph_x_prev + (1.0f - alpha) * preemph_y;
                preemph_x_prev = x;
                float y = preemph_y;
                if (y > 32767.0f) y = 32767.0f;
                if (y < -32768.0f) y = -32768.0f;
                samples_in[i] = (int16_t)y;
            }
        }

        int synced = sc_rx_process(rx, symbols_out, samples_in);
        if (synced) {
            fwrite(symbols_out, sizeof(float), SC_NPAYLOAD, stdout);
            synced_frames++;
        }

        frames++;
    }

    fprintf(stderr, "sc_rx: %d frames, %d synced, fcentre=%.0f preemph=%d\n",
            frames, synced_frames, fcentre, preemph);
    sc_rx_destroy(rx);
    return 0;
}

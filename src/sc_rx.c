/*---------------------------------------------------------------------------*\

  sc_rx.c

  Single-carrier BBFM modem receiver command-line tool.
  Reads int16 audio samples from stdin, writes 80 float symbols to stdout
  when synchronized.

  Usage:
    cat audio.raw | sc_rx > latents.f32
    sc_tx | sc_rx > latents_out.f32

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "sc_modem.h"

int main(void) {
    sc_modem_rx *rx = sc_rx_create();
    if (!rx) {
        fprintf(stderr, "sc_rx: failed to create RX modem\n");
        return 1;
    }

    int16_t samples_in[SC_NIN_MAX];
    float symbols_out[SC_NPAYLOAD];
    int frames = 0;
    int synced_frames = 0;

    while (1) {
        int nin = sc_rx_nin(rx);
        size_t n = fread(samples_in, sizeof(int16_t), nin, stdin);
        if ((int)n != nin) break;

        int synced = sc_rx_process(rx, symbols_out, samples_in);
        if (synced) {
            fwrite(symbols_out, sizeof(float), SC_NPAYLOAD, stdout);
            synced_frames++;
        }

        frames++;
    }

    fprintf(stderr, "sc_rx: %d frames, %d synced\n", frames, synced_frames);
    sc_rx_destroy(rx);
    return 0;
}

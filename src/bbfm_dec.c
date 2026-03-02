/*
  bbfm_dec.c

  BBFM decoder: reads latent symbols from stdin (from sc_rx),
  decodes, writes features to stdout (for lpcnet_demo -fargan-synthesis).

  Reads 80 floats (RADE_LATENT_DIM), writes 4 frames × 36 floats.

  Usage:
    sc_rx | bbfm_dec | lpcnet_demo -fargan-synthesis - - > voice_out.raw
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_support.h"
#include "rade_core.h"
#include "rade_dec.h"
#include "rade_dec_data.h"
#include "rade_constants.h"
#include "arch.h"

/* LPCNet feature dimensions */
#define NB_TOTAL_FEATURES 36
#define NB_USED_FEATURES  20

int main(void) {
    RADEDecState *dec_state = calloc(1, sizeof(RADEDecState));
    RADEDec *dec_model = calloc(1, sizeof(RADEDec));

    if (!dec_state || !dec_model) {
        fprintf(stderr, "bbfm_dec: alloc failed\n");
        return 1;
    }

    rade_init_decoder(dec_state);

    int ret = init_radedec(dec_model, radedec_arrays);
    if (ret) {
        fprintf(stderr, "bbfm_dec: init_radedec failed (%d)\n", ret);
        return 1;
    }

    int arch = opus_select_arch();
    int dec_stride = DEC_OUTPUT_OUT_SIZE / RADE_FRAMES_PER_STEP;

    float *latents = calloc(RADE_LATENT_DIM, sizeof(float));
    float *dec_output = calloc(DEC_OUTPUT_OUT_SIZE, sizeof(float));
    float *features_out = calloc(RADE_FRAMES_PER_STEP * NB_TOTAL_FEATURES, sizeof(float));

    int frames = 0;

    while (1) {
        size_t n = fread(latents, sizeof(float), RADE_LATENT_DIM, stdin);
        if (n != RADE_LATENT_DIM) break;

        /* Decode: latents -> features */
        rade_core_decoder(dec_state, dec_model, dec_output,
                          latents, arch);

        /* Unpack decoder output (stride 20) into 36-wide feature
           frames for FARGAN, zeroing unused features 20-35 */
        memset(features_out, 0, RADE_FRAMES_PER_STEP * NB_TOTAL_FEATURES * sizeof(float));
        int i;
        for (i = 0; i < RADE_FRAMES_PER_STEP; i++) {
            memcpy(&features_out[i * NB_TOTAL_FEATURES],
                   &dec_output[i * dec_stride],
                   NB_USED_FEATURES * sizeof(float));
        }

        fwrite(features_out, sizeof(float),
               RADE_FRAMES_PER_STEP * NB_TOTAL_FEATURES, stdout);

        frames += RADE_FRAMES_PER_STEP;
    }

    fprintf(stderr, "bbfm_dec: processed %d frames\n", frames);
    free(dec_state); free(dec_model);
    free(latents); free(dec_output); free(features_out);
    return 0;
}

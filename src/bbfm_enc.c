/*
  bbfm_enc.c

  BBFM encoder: reads features from stdin (from lpcnet_demo -features),
  encodes with bottleneck=1, writes latent symbols to stdout (for sc_tx).

  Reads 4 frames × 36 floats per step, writes 80 floats (RADE_LATENT_DIM).

  Usage:
    cat voice_16k.raw | lpcnet_demo -features - - | bbfm_enc | sc_tx > audio.raw
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_support.h"
#include "rade_core.h"
#include "rade_enc.h"
#include "rade_enc_data.h"
#include "rade_constants.h"
#include "arch.h"

/* LPCNet feature dimensions */
#define NB_TOTAL_FEATURES 36
#define NB_USED_FEATURES  20

/* BBFM encoder uses 20 features per frame (no auxdata) */
#define ENC_INPUT_DIM     (RADE_FRAMES_PER_STEP * NB_USED_FEATURES)  /* 80 */

int main(void) {
    RADEEncState *enc_state = calloc(1, sizeof(RADEEncState));
    RADEEnc *enc_model = calloc(1, sizeof(RADEEnc));

    if (!enc_state || !enc_model) {
        fprintf(stderr, "bbfm_enc: alloc failed\n");
        return 1;
    }

    rade_init_encoder(enc_state);

    int ret = init_radeenc(enc_model, radeenc_arrays);
    if (ret) {
        fprintf(stderr, "bbfm_enc: init_radeenc failed (%d)\n", ret);
        return 1;
    }

    int arch = opus_select_arch();

    float *features_in = calloc(RADE_FRAMES_PER_STEP * NB_TOTAL_FEATURES, sizeof(float));
    float *enc_input = calloc(ENC_INPUT_DIM, sizeof(float));
    float *latents = calloc(RADE_LATENT_DIM, sizeof(float));

    int frames = 0;

    while (1) {
        int i;
        for (i = 0; i < RADE_FRAMES_PER_STEP; i++) {
            size_t n = fread(&features_in[i * NB_TOTAL_FEATURES],
                             sizeof(float), NB_TOTAL_FEATURES, stdin);
            if (n != NB_TOTAL_FEATURES) goto done;
        }

        /* Pack 20 used features per frame into encoder input */
        for (i = 0; i < RADE_FRAMES_PER_STEP; i++) {
            memcpy(&enc_input[i * NB_USED_FEATURES],
                   &features_in[i * NB_TOTAL_FEATURES],
                   NB_USED_FEATURES * sizeof(float));
        }

        /* Encode: features -> latents (bottleneck=1 for BBFM) */
        rade_core_encoder(enc_state, enc_model, latents,
                          enc_input, arch, 1);

        fwrite(latents, sizeof(float), RADE_LATENT_DIM, stdout);

        frames += RADE_FRAMES_PER_STEP;
    }

done:
    fprintf(stderr, "bbfm_enc: processed %d frames\n", frames);
    free(enc_state); free(enc_model);
    free(features_in); free(enc_input); free(latents);
    return 0;
}

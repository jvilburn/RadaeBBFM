/*
  bbfm_loopback.c

  BBFM encoder/decoder loopback test.
  Reads features from stdin (from lpcnet_demo -features),
  encodes with bottleneck=1, decodes, writes features to stdout
  (for lpcnet_demo -fargan-synthesis).

  Bypasses the OFDM modem entirely - tests only the neural
  encoder and decoder with BBFM weights.

  Usage:
    cat voice_16k.raw | lpcnet_demo -features - - | bbfm_loopback | lpcnet_demo -fargan-synthesis - - > out.raw
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_support.h"
#include "rade_core.h"
#include "rade_enc.h"
#include "rade_dec.h"
#include "rade_enc_data.h"
#include "rade_dec_data.h"
#include "rade_constants.h"
#include "arch.h"

/* LPCNet feature dimensions */
#define NB_TOTAL_FEATURES 36
#define NB_USED_FEATURES  20

/* BBFM encoder uses 20 features per frame (no auxdata) */
#define ENC_INPUT_DIM     (RADE_FRAMES_PER_STEP * NB_USED_FEATURES)  /* 80 */

int main(void) {
    RADEEncState *enc_state = calloc(1, sizeof(RADEEncState));
    RADEDecState *dec_state = calloc(1, sizeof(RADEDecState));
    RADEEnc *enc_model = calloc(1, sizeof(RADEEnc));
    RADEDec *dec_model = calloc(1, sizeof(RADEDec));

    if (!enc_state || !dec_state || !enc_model || !dec_model) {
        fprintf(stderr, "bbfm_loopback: alloc failed\n");
        return 1;
    }

    rade_init_encoder(enc_state);
    rade_init_decoder(dec_state);

    int ret_enc = init_radeenc(enc_model, radeenc_arrays);
    if (ret_enc) {
        fprintf(stderr, "bbfm_loopback: init_radeenc failed (%d)\n", ret_enc);
        return 1;
    }

    int ret_dec = init_radedec(dec_model, radedec_arrays);
    if (ret_dec) {
        fprintf(stderr, "bbfm_loopback: init_radedec failed (%d)\n", ret_dec);
        return 1;
    }

    int arch = opus_select_arch();
    int dec_stride = DEC_OUTPUT_OUT_SIZE / RADE_FRAMES_PER_STEP;

    fprintf(stderr, "bbfm_loopback: enc_input_dim=%d dec_output=%d dec_stride=%d latent=%d\n",
            ENC_INPUT_DIM, DEC_OUTPUT_OUT_SIZE, dec_stride, RADE_LATENT_DIM);

    float *features_in = calloc(RADE_FRAMES_PER_STEP * NB_TOTAL_FEATURES, sizeof(float));
    float *enc_input = calloc(ENC_INPUT_DIM, sizeof(float));
    float *latents = calloc(RADE_LATENT_DIM, sizeof(float));
    float *dec_output = calloc(DEC_OUTPUT_OUT_SIZE, sizeof(float));
    float *features_out = calloc(RADE_FRAMES_PER_STEP * NB_TOTAL_FEATURES, sizeof(float));

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

        /* Decode: latents -> features */
        rade_core_decoder(dec_state, dec_model, dec_output,
                          latents, arch);

        /* Unpack decoder output (stride 20) into 36-wide feature
           frames for FARGAN, zeroing unused features 20-35 */
        memset(features_out, 0, RADE_FRAMES_PER_STEP * NB_TOTAL_FEATURES * sizeof(float));
        for (i = 0; i < RADE_FRAMES_PER_STEP; i++) {
            memcpy(&features_out[i * NB_TOTAL_FEATURES],
                   &dec_output[i * dec_stride],
                   NB_USED_FEATURES * sizeof(float));
        }

        fwrite(features_out, sizeof(float),
               RADE_FRAMES_PER_STEP * NB_TOTAL_FEATURES, stdout);

        frames += RADE_FRAMES_PER_STEP;
    }

done:
    fprintf(stderr, "bbfm_loopback: processed %d frames\n", frames);
    free(enc_state); free(dec_state); free(enc_model); free(dec_model);
    free(features_in); free(enc_input); free(latents); free(dec_output); free(features_out);
    return 0;
}


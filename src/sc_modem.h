/*---------------------------------------------------------------------------*\

  sc_modem.h

  Single-carrier BBFM modem API.

  Sits between the RADE encoder/decoder and an FM radio audio interface.
  BPSK-like symbols at 2400 sym/s, 9600 Hz sample rate, 1500 Hz centre.

  Reference: radae/dsp.py class single_carrier

\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2024 David Rowe

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  - Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SC_MODEM_H
#define SC_MODEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Modem parameters */
#define SC_RS           2400    /* Symbol rate (Hz) */
#define SC_FS           9600    /* Sample rate (Hz) */
#define SC_M            4       /* Oversampling factor Fs/Rs */
#define SC_FCENTRE      1500    /* Centre frequency (Hz) */
#define SC_ALPHA        0.25f   /* RRC roll-off factor */
#define SC_NFILT_SYM    6       /* RRC filter span (symbols) */
#define SC_NTAP         24      /* RRC filter taps (Nfilt_sym * M) */

/* Frame structure */
#define SC_NSYNC        16      /* Sync word symbols */
#define SC_NPAYLOAD     80      /* Payload symbols per frame */
#define SC_NFRAME       96      /* Total symbols per frame (sync + payload) */
#define SC_NSAMPLES     (SC_NFRAME * SC_M)  /* Nominal samples per frame (384) */

/* Phase estimator */
#define SC_NPHASE       21      /* Phase estimation window (must be odd) */

/* Sync thresholds */
#define SC_SYNC_THRESH      0.5f
#define SC_UNSYNC_THRESH1   2       /* Error count to flag bad frame */
#define SC_UNSYNC_THRESH2   3       /* Consecutive bad frames to lose sync */

/* TX scaling */
#define SC_TX_SCALE     16384.0f

/* Maximum nin (nominal + M/4) */
#define SC_NIN_MAX      (SC_NSAMPLES + SC_M/4)

/* Opaque state structures */
typedef struct sc_modem_tx sc_modem_tx;
typedef struct sc_modem_rx sc_modem_rx;

/* --- TX API --- */

/* Create TX modem instance. Returns NULL on allocation failure. */
sc_modem_tx *sc_tx_create(void);

/* Destroy TX modem instance. */
void sc_tx_destroy(sc_modem_tx *tx);

/* Process one frame: 80 float symbols in, 384 int16 audio samples out.
   Returns number of output samples (always SC_NSAMPLES = 384). */
int sc_tx_process(sc_modem_tx *tx, int16_t out[], const float symbols_in[]);

/* --- RX API --- */

/* Create RX modem instance. Returns NULL on allocation failure. */
sc_modem_rx *sc_rx_create(void);

/* Destroy RX modem instance. */
void sc_rx_destroy(sc_modem_rx *rx);

/* Returns number of input samples needed for next sc_rx_process call.
   Nominally 384, can be 383 or 385. */
int sc_rx_nin(sc_modem_rx *rx);

/* Process one frame of audio samples.
   samples_in: sc_rx_nin() int16 audio samples
   symbols_out: buffer for SC_NPAYLOAD (80) float symbols (written only if synced)
   Returns 1 if synced and symbols_out is valid, 0 if not synced. */
int sc_rx_process(sc_modem_rx *rx, float symbols_out[], const int16_t samples_in[]);

/* Returns 1 if RX is currently in sync state. */
int sc_rx_sync(sc_modem_rx *rx);

#ifdef __cplusplus
}
#endif

#endif /* SC_MODEM_H */

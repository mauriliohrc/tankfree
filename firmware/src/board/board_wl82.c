/* board_wl82.c — SDK glue for the Tank-G (JieLi WL82 / AC791N).
 *
 * Connects our DSP pipeline (app_alpha.c) to the JieLi AC79 SDK's USB Audio Class
 * + codec, so the pedal is a USB soundcard like the factory firmware. This file
 * targets the SDK build (it #includes SDK headers and calls real SDK APIs found in
 * include_lib/driver/device/usb/device/uac_audio.h); it does NOT build standalone —
 * it compiles inside the SDK app tree. See firmware/SDK_INTEGRATION.md.
 *
 * Pipeline per audio block:  guitar ADC (+ optional DAW playback) → tf_alpha_process
 *                            → DAC (phones/XLR) AND USB capture (to the DAW).
 */
#if defined(TANKFREE_SDK)      /* only compiled in the SDK app tree */
#include "system/includes.h"
#include "usb/device/uac_audio.h"
#include "usb/device/uac_stream.h"

extern void tf_alpha_init(void);
extern void tf_alpha_process(float *buf, int n);
extern void tf_alpha_set_master(float v);

#define USB_ID         0
#define SR             48000
#define BLK            64              /* samples per block */

static volatile float g_playback[BLK];  /* latest PC→pedal audio */
static volatile int   g_have_playback;

/* UAC speaker (PC playback) arrives here. */
static void spk_rx(void *priv, void *buf, int len) {
    const int16_t *s = buf; int n = len / 2; if (n > BLK) n = BLK;
    for (int i = 0; i < n; i++) g_playback[i] = s[i] / 32768.0f;
    g_have_playback = 1;
}

/* Read a block: guitar ADC (+ mix PC playback for reamp/monitor). */
int tf_board_audio_in(float *buf, int n) {
    extern int tf_codec_adc_read(float *buf, int n);   /* SDK ADC (guitar) */
    tf_codec_adc_read(buf, n);
    if (g_have_playback) { for (int i = 0; i < n; i++) buf[i] += g_playback[i]; g_have_playback = 0; }
    return n;
}

/* Write a block: DAC (phones/XLR) AND USB capture (to the DAW). */
void tf_board_audio_out(const float *buf, int n) {
    extern void tf_codec_dac_write(const float *buf, int n);   /* SDK DAC */
    tf_codec_dac_write(buf, n);
    int16_t pcm[BLK]; if (n > BLK) n = BLK;
    for (int i = 0; i < n; i++) { float v = buf[i]; pcm[i] = (int16_t)(v > 1 ? 32767 : v < -1 ? -32768 : v * 32767); }
    /* duplicate mono → stereo capture handled by the mic stream config */
    extern void tf_uac_mic_write(const int16_t *pcm, int n);
    tf_uac_mic_write(pcm, n);
}

int tf_board_usb_playback(float *buf, int n) { for (int i = 0; i < n; i++) buf[i] = g_playback[i]; return n; }

/* Reboot into the JieLi loader so M-UPGRADE / mac_flash can write (recovery). */
void tf_enter_loader(void) {
    extern void jl_enter_uboot(void);   /* SDK/mask-ROM entry (wire to the update API) */
    jl_enter_uboot();
}

/* Bring up USB audio + start the Alpha engine. Called from the SDK app init. */
void tf_board_start(void) {
    tf_alpha_init();
    uac_register(USB_ID);
    uac_speaker_stream_open(USB_ID, SR, 2);
    uac_mic_stream_open(USB_ID, SR, BLK * 2, 2);
    set_uac_speaker_rx_handler(USB_ID, 0, spk_rx);
    /* master knob from the host volume (or a physical ADC knob) */
    tf_alpha_set_master(uac_get_spk_vol() / 100.0f);
    /* the SDK audio task then calls tf_alpha_tick()/our process per block */
}
#endif /* TANKFREE_SDK */

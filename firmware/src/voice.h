#pragma once

#include <cstddef>
#include <cstdint>

enum class VoiceCue : uint8_t {
  None = 0,
  BlockedApi = 1,
  SpokenBlocked = 2,
  MicMissing = 3,
  NeedKey = 4,
  TooShort = 5
};

void voice_begin();
void voice_poll();
bool voice_ptt_start();
bool voice_ptt_stop();
void voice_ptt_cancel();
bool voice_is_recording();
const int16_t* voice_pcm(size_t* samples_out);
uint32_t voice_sample_rate();
size_t voice_sample_count();
const uint8_t* voice_wav_blob(size_t* bytes_out);
const char* voice_sd_path();
void voice_play_pcm(const int16_t* pcm, size_t samples, uint32_t rate);
void voice_play_wav(const uint8_t* wav, size_t bytes);
void voice_speaker_resume();
void voice_cue(VoiceCue cue);
void voice_silence();

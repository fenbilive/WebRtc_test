//
//  main.cpp
//  webrtc_aec3
//
//  Created by huoyajun on 2019/9/30.
//  Copyright © 2019 huoyajun. All rights reserved.
//

#include "modules/audio_processing/aec3/echo_canceller3.h"

#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <ctime>

#include "modules/audio_processing/aec3/aec3_common.h"
#include "modules/audio_processing/aec3/block_processor.h"
#include "modules/audio_processing/aec3/frame_blocker.h"
#include "modules/audio_processing/audio_buffer.h"
#include "modules/audio_processing/high_pass_filter.h"
#include "modules/audio_processing/utility/cascaded_biquad_filter.h"
#include "modules/audio_processing/audio_buffer.h"
#include "rtc_base/strings/string_builder.h"
#include "api/audio/audio_frame.h"

using namespace webrtc;

static size_t buffer_sample_rate = 16000;
static size_t buffer_num_channels = 1;
static size_t output_sample_rate = 16000;
static size_t output_num_channel = 1;

static const std::string wav_read_path_cap = "/Users/huoyajun/Downloads/capture_d200.wav";
static const std::string wav_read_path_ren = "/Users/huoyajun/Downloads/render.wav";

static const std::string wav_write_path = "/Users/huoyajun/Downloads/audio_output_d200.wav";
constexpr size_t frameLenInMS = 10;
//constexpr size_t max_sample_rate = 48000;

void OptionalBandSplit(uint16_t sample_rate_hz_, AudioBuffer * capture_buffer_, AudioBuffer * render_buffer_) {
  if (sample_rate_hz_ > 16000) {
      capture_buffer_->SplitIntoFrequencyBands();
      render_buffer_->SplitIntoFrequencyBands();
  }
}

constexpr size_t NumBandsForRate(int sample_rate_hz) {
    return static_cast<size_t>(sample_rate_hz / 16000);
}

int main(int argc, const char * argv[]) {

  WavReader wav_read_cap(wav_read_path_cap);
  WavReader wav_read_ren(wav_read_path_ren);
  WavWriter wav_write(wav_write_path, (int)output_sample_rate, output_num_channel);

  size_t max_channel_num = std::max<size_t>(wav_read_cap.num_channels(), wav_read_ren.num_channels());
  size_t max_sample_rate = std::max<size_t>(wav_read_cap.sample_rate(), wav_read_ren.sample_rate());
  size_t buf_size = sizeof(float) * max_sample_rate * max_channel_num * frameLenInMS / 1000;
  void *pReadBuf = (void *)malloc(buf_size);
  assert(NULL !=  pReadBuf);
  memset(pReadBuf, 0, buf_size);
  int16_t *pBuf = (int16_t *)pReadBuf;

  StreamConfig cap_config(wav_read_cap.sample_rate(), wav_read_cap.num_channels());
  StreamConfig ren_config(wav_read_ren.sample_rate(), wav_read_ren.num_channels());
  AudioBuffer capture_buffer(wav_read_cap.sample_rate(), wav_read_cap.num_channels(), buffer_sample_rate, buffer_num_channels, output_sample_rate, output_num_channel);
  AudioBuffer render_buffer(wav_read_ren.sample_rate(), wav_read_ren.num_channels(), buffer_sample_rate, buffer_num_channels, output_sample_rate, output_num_channel);
  EchoCanceller3 aec3_test(EchoCanceller3Config(), static_cast<int>(buffer_sample_rate), wav_read_cap.num_channels(), wav_read_ren.num_channels());

  size_t frame_len_cap = wav_read_cap.sample_rate() * frameLenInMS / 1000;
  size_t frame_len_ren = wav_read_ren.sample_rate() * frameLenInMS / 1000;
  size_t frame_num_cap = wav_read_cap.num_samples() / (frame_len_cap * wav_read_cap.num_channels());
  size_t frame_num_ren = wav_read_ren.num_samples() / (frame_len_ren * wav_read_ren.num_channels());
  size_t frame_num = std::min(frame_num_cap, frame_num_ren);

  for(int i = 0; i < frame_num; i++)
  {
    size_t sample_num = frame_len_cap * wav_read_cap.num_channels();
    if (sample_num == wav_read_cap.ReadSamples(sample_num, pBuf))
    {
      capture_buffer.CopyFrom(pBuf, cap_config);
    }

    sample_num = frame_len_ren * wav_read_ren.num_channels();
    if (sample_num == wav_read_ren.ReadSamples(sample_num, pBuf))
    {
      render_buffer.CopyFrom(pBuf, ren_config);
    }

    aec3_test.AnalyzeCapture(&capture_buffer);
    OptionalBandSplit(wav_read_cap.sample_rate(), &capture_buffer, &render_buffer);
    aec3_test.AnalyzeRender(&render_buffer);

    aec3_test.ProcessCapture(&capture_buffer, false);
    capture_buffer.CopyTo(cap_config, pBuf);

    wav_write.WriteSamples(pBuf, frame_len_cap);
  }

    std::cout << "process succeed\n";
    return 0;
}

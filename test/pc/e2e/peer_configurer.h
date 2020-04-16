/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TEST_PC_E2E_PEER_CONFIGURER_H_
#define TEST_PC_E2E_PEER_CONFIGURER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "api/async_resolver_factory.h"
#include "api/call/call_factory_interface.h"
#include "api/fec_controller.h"
#include "api/rtc_event_log/rtc_event_log_factory_interface.h"
#include "api/task_queue/task_queue_factory.h"
#include "api/test/peerconnection_quality_test_fixture.h"
#include "api/transport/media/media_transport_interface.h"
#include "api/transport/network_control.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "rtc_base/network.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/ssl_certificate.h"
#include "rtc_base/thread.h"
#include "test/pc/e2e/peer_connection_quality_test_params.h"

namespace webrtc {
namespace webrtc_pc_e2e {

class PeerConfigurerImpl final
    : public PeerConnectionE2EQualityTestFixture::PeerConfigurer {
 public:
  PeerConfigurerImpl(rtc::Thread* network_thread,
                     rtc::NetworkManager* network_manager)
      : components_(std::make_unique<InjectableComponents>(network_thread,
                                                           network_manager)),
        params_(std::make_unique<Params>()) {}

  // Implementation of PeerConnectionE2EQualityTestFixture::PeerConfigurer.
  PeerConfigurer* SetTaskQueueFactory(
      std::unique_ptr<TaskQueueFactory> task_queue_factory) override {
    components_->pcf_dependencies->task_queue_factory =
        std::move(task_queue_factory);
    return this;
  }
  PeerConfigurer* SetCallFactory(
      std::unique_ptr<CallFactoryInterface> call_factory) override {
    components_->pcf_dependencies->call_factory = std::move(call_factory);
    return this;
  }
  PeerConfigurer* SetEventLogFactory(
      std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory) override {
    components_->pcf_dependencies->event_log_factory =
        std::move(event_log_factory);
    return this;
  }
  PeerConfigurer* SetFecControllerFactory(
      std::unique_ptr<FecControllerFactoryInterface> fec_controller_factory)
      override {
    components_->pcf_dependencies->fec_controller_factory =
        std::move(fec_controller_factory);
    return this;
  }
  PeerConfigurer* SetNetworkControllerFactory(
      std::unique_ptr<NetworkControllerFactoryInterface>
          network_controller_factory) override {
    components_->pcf_dependencies->network_controller_factory =
        std::move(network_controller_factory);
    return this;
  }
  PeerConfigurer* SetMediaTransportFactory(
      std::unique_ptr<MediaTransportFactory> media_transport_factory) override {
    components_->pcf_dependencies->media_transport_factory =
        std::move(media_transport_factory);
    return this;
  }
  PeerConfigurer* SetVideoEncoderFactory(
      std::unique_ptr<VideoEncoderFactory> video_encoder_factory) override {
    components_->pcf_dependencies->video_encoder_factory =
        std::move(video_encoder_factory);
    return this;
  }
  PeerConfigurer* SetVideoDecoderFactory(
      std::unique_ptr<VideoDecoderFactory> video_decoder_factory) override {
    components_->pcf_dependencies->video_decoder_factory =
        std::move(video_decoder_factory);
    return this;
  }

  PeerConfigurer* SetAsyncResolverFactory(
      std::unique_ptr<webrtc::AsyncResolverFactory> async_resolver_factory)
      override {
    components_->pc_dependencies->async_resolver_factory =
        std::move(async_resolver_factory);
    return this;
  }
  PeerConfigurer* SetRTCCertificateGenerator(
      std::unique_ptr<rtc::RTCCertificateGeneratorInterface> cert_generator)
      override {
    components_->pc_dependencies->cert_generator = std::move(cert_generator);
    return this;
  }
  PeerConfigurer* SetSSLCertificateVerifier(
      std::unique_ptr<rtc::SSLCertificateVerifier> tls_cert_verifier) override {
    components_->pc_dependencies->tls_cert_verifier =
        std::move(tls_cert_verifier);
    return this;
  }

  PeerConfigurer* AddVideoConfig(
      PeerConnectionE2EQualityTestFixture::VideoConfig config) override {
    params_->video_configs.push_back(std::move(config));
    video_generators_.push_back(nullptr);
    return this;
  }
  PeerConfigurer* AddVideoConfig(
      PeerConnectionE2EQualityTestFixture::VideoConfig config,
      std::unique_ptr<test::FrameGeneratorInterface> generator) override {
    params_->video_configs.push_back(std::move(config));
    video_generators_.push_back(std::move(generator));
    return this;
  }
  PeerConfigurer* SetAudioConfig(
      PeerConnectionE2EQualityTestFixture::AudioConfig config) override {
    params_->audio_config = std::move(config);
    return this;
  }
  PeerConfigurer* SetNetEqFactory(
      std::unique_ptr<NetEqFactory> neteq_factory) override {
    components_->pcf_dependencies->neteq_factory = std::move(neteq_factory);
    return this;
  }
  PeerConfigurer* SetRtcEventLogPath(std::string path) override {
    params_->rtc_event_log_path = std::move(path);
    return this;
  }
  PeerConfigurer* SetAecDumpPath(std::string path) override {
    params_->aec_dump_path = std::move(path);
    return this;
  }
  PeerConfigurer* SetRTCConfiguration(
      PeerConnectionInterface::RTCConfiguration configuration) override {
    params_->rtc_configuration = std::move(configuration);
    return this;
  }
  PeerConfigurer* SetBitrateParameters(
      PeerConnectionInterface::BitrateParameters bitrate_params) override {
    params_->bitrate_params = bitrate_params;
    return this;
  }

  PeerConfigurer* SetIceTransportFactory(
      std::unique_ptr<IceTransportFactory> factory) override {
    components_->pc_dependencies->ice_transport_factory = std::move(factory);
    return this;
  }
  // Implementation of PeerConnectionE2EQualityTestFixture::PeerConfigurer end.

  InjectableComponents* components() { return components_.get(); }
  Params* params() { return params_.get(); }
  std::vector<std::unique_ptr<test::FrameGeneratorInterface>>*
  video_generators() {
    return &video_generators_;
  }

  // Returns InjectableComponents and transfer ownership to the caller.
  // Can be called once.
  std::unique_ptr<InjectableComponents> ReleaseComponents() {
    RTC_CHECK(components_);
    auto components = std::move(components_);
    components_ = nullptr;
    return components;
  }
  // Returns Params and transfer ownership to the caller.
  // Can be called once.
  std::unique_ptr<Params> ReleaseParams() {
    RTC_CHECK(params_);
    auto params = std::move(params_);
    params_ = nullptr;
    return params;
  }
  // Returns frame generators and transfer ownership to the caller.
  // Can be called once.
  std::vector<std::unique_ptr<test::FrameGeneratorInterface>>
  ReleaseVideoGenerators() {
    auto video_generators = std::move(video_generators_);
    video_generators_.clear();
    return video_generators;
  }

 private:
  std::unique_ptr<InjectableComponents> components_;
  std::unique_ptr<Params> params_;
  std::vector<std::unique_ptr<test::FrameGeneratorInterface>> video_generators_;
};

// Set missing params to default values if it is required:
//  * Generate video stream labels if some of them are missing
//  * Generate audio stream labels if some of them are missing
//  * Set video source generation mode if it is not specified
//  * Video codecs under test
void SetDefaultValuesForMissingParams(
    PeerConnectionE2EQualityTestFixture::RunParams* run_params,
    std::vector<std::unique_ptr<PeerConfigurerImpl>>* peers);
// Validate peer's parameters, also ensure uniqueness of all video stream
// labels.
void ValidateParams(
    const PeerConnectionE2EQualityTestFixture::RunParams& run_params,
    const std::vector<std::unique_ptr<PeerConfigurerImpl>>& peers);

}  // namespace webrtc_pc_e2e
}  // namespace webrtc

#endif  // TEST_PC_E2E_PEER_CONFIGURER_H_

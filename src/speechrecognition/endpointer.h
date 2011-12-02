// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SPEECH_ENDPOINTER_ENDPOINTER_H_
#define CONTENT_BROWSER_SPEECH_ENDPOINTER_ENDPOINTER_H_

#include "energy_endpointer.h"

#include <QtGlobal>

class EpStatus;

namespace speech_input {

// A simple interface to the underlying energy-endpointer implementation, this
// class lets callers provide audio as being recorded and let them poll to find
// when the user has stopped speaking.
//
// There are two events that may trigger the end of speech:
//
// speechInputPossiblyComplete event:
//
// Signals that silence/noise has  been detected for a *short* amount of
// time after some speech has been detected. It can be used for low latency
// UI feedback. To disable it, set it to a large amount.
//
// speechInputComplete event:
//
// This event is intended to signal end of input and to stop recording.
// The amount of time to wait after speech is set by
// speech_input_complete_silence_length_ and optionally two other
// parameters (see below).
// This time can be held constant, or can change as more speech is detected.
// In the latter case, the time changes after a set amount of time from the
// *beginning* of speech.  This is motivated by the expectation that there
// will be two distinct types of inputs: short search queries and longer
// dictation style input.
//
// Three parameters are used to define the piecewise constant timeout function.
// The timeout length is speech_input_complete_silence_length until
// long_speech_length, when it changes to
// long_speech_input_complete_silence_length.
class Endpointer {
 public:
  explicit Endpointer(int sample_rate);

  // Start the endpointer. This should be called at the beginning of a session.
  void StartSession();

  // Stop the endpointer.
  void EndSession();

  // Start environment estimation. Audio will be used for environment estimation
  // i.e. noise level estimation.
  void SetEnvironmentEstimationMode();

  // Start user input. This should be called when the user indicates start of
  // input, e.g. by pressing a button.
  void SetUserInputMode();

  // Process a segment of audio, which may be more than one frame.
  // The status of the last frame will be returned.
  EpStatus ProcessAudio(const qint16* audio_data, int num_samples,
                        float* rms_out);

  // Get the status of the endpointer.
  EpStatus Status(qint64 *time_us);

  // Returns true if the endpointer detected reasonable audio levels above
  // background noise which could be user speech, false if not.
  bool DidStartReceivingSpeech() const {
    return speech_previously_detected_;
  }

  bool IsEstimatingEnvironment() const {
    return energy_endpointer_.estimating_environment();
  }

  void set_speech_input_complete_silence_length(qint64 time_us) {
    speech_input_complete_silence_length_us_ = time_us;
  }

  void set_long_speech_input_complete_silence_length(qint64 time_us) {
    long_speech_input_complete_silence_length_us_ = time_us;
  }

  void set_speech_input_possibly_complete_silence_length(qint64 time_us) {
    speech_input_possibly_complete_silence_length_us_ = time_us;
  }

  void set_long_speech_length(qint64 time_us) {
    long_speech_length_us_ = time_us;
  }

  bool speech_input_complete() const {
    return speech_input_complete_;
  }

  // RMS background noise level in dB.
  float NoiseLevelDb() const { return energy_endpointer_.GetNoiseLevelDb(); }

 private:
  // Reset internal states. Helper method common to initial input utterance
  // and following input utternaces.
  void Reset();

  // Minimum allowable length of speech input.
  qint64 speech_input_minimum_length_us_;

  // The speechInputPossiblyComplete event signals that silence/noise has been
  // detected for a *short* amount of time after some speech has been detected.
  // This proporty specifies the time period.
  qint64 speech_input_possibly_complete_silence_length_us_;

  // The speechInputComplete event signals that silence/noise has been
  // detected for a *long* amount of time after some speech has been detected.
  // This property specifies the time period.
  qint64 speech_input_complete_silence_length_us_;

  // Same as above, this specifies the required silence period after speech
  // detection. This period is used instead of
  // speech_input_complete_silence_length_ when the utterance is longer than
  // long_speech_length_. This parameter is optional.
  qint64 long_speech_input_complete_silence_length_us_;

  // The period of time after which the endpointer should consider
  // long_speech_input_complete_silence_length_ as a valid silence period
  // instead of speech_input_complete_silence_length_. This parameter is
  // optional.
  qint64 long_speech_length_us_;

  // First speech onset time, used in determination of speech complete timeout.
  qint64 speech_start_time_us_;

  // Most recent end time, used in determination of speech complete timeout.
  qint64 speech_end_time_us_;

  qint64 audio_frame_time_us_;
  EpStatus old_ep_status_;
  bool waiting_for_speech_possibly_complete_timeout_;
  bool waiting_for_speech_complete_timeout_;
  bool speech_previously_detected_;
  bool speech_input_complete_;
  EnergyEndpointer energy_endpointer_;
  int sample_rate_;
  qint32 frame_size_;
};

}  // namespace speech_input

#endif  // CONTENT_BROWSER_SPEECH_ENDPOINTER_ENDPOINTER_H_

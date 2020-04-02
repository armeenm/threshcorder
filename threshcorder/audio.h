#pragma once

#include <alsa/asoundlib.h>
#include <array>
#include <cmath>
#include <fmt/core.h>
#include <memory>
#include <numeric>
#include <optional>

using sample_t = std::int16_t;
using fat_t = std::int64_t;

auto get_handle(std::string_view device, snd_pcm_format_t format, unsigned int rate) -> snd_pcm_t* {
  int err;
  snd_pcm_t* handle;
  snd_pcm_hw_params_t* hw_params;

  if ((err = snd_pcm_open(&handle, device.data(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fmt::print(stderr, "Cannot open audio device {} ({})\n", device.data(), snd_strerror(err));
    return nullptr;
  }

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    fmt::print(stderr, "Cannot allocate hardware parameter structure ({})\n", snd_strerror(err));
    return nullptr;
  }

  if ((err = snd_pcm_hw_params_any(handle, hw_params)) < 0) {
    fmt::print(stderr, "Cannot initialize hardware parameter structure ({})\n", snd_strerror(err));
    return nullptr;
  }

  if ((err = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fmt::print(stderr, "Cannot set access type ({})\n", snd_strerror(err));
    return nullptr;
  }

  if ((err = snd_pcm_hw_params_set_format(handle, hw_params, format)) < 0) {
    fmt::print(stderr, "Cannot set sample format ({})\n", snd_strerror(err));
    return nullptr;
  }

  if ((err = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, 0)) < 0) {
    fmt::print(stderr, "Cannot set sample rate ({})\n", snd_strerror(err));
    return nullptr;
  }

  if ((err = snd_pcm_hw_params_set_channels(handle, hw_params, 1)) < 0) {
    fmt::print(stderr, "Cannot set channel count ({})\n", snd_strerror(err));
    return nullptr;
  }

  if ((err = snd_pcm_hw_params(handle, hw_params)) < 0) {
    fmt::print(stderr, "Cannot set parameters ({})\n", snd_strerror(err));
    return nullptr;
  }

  snd_pcm_hw_params_free(hw_params);

  if ((err = snd_pcm_prepare(handle)) < 0) {
    fmt::print(stderr, "Cannot prepare audio interface for use ({})\n", snd_strerror(err));
    return nullptr;
  }

  return handle;
}

template <snd_pcm_uframes_t frames>
auto listen(snd_pcm_t* const handle)
    -> std::optional<std::pair<std::array<sample_t, frames>, snd_pcm_uframes_t>> {

  std::array<sample_t, frames> buf;

  long int err;
  if ((err = snd_pcm_readi(handle, buf.data(), frames)) != frames) {
    fmt::print(stderr, "Read from audio interface failed ({})\n", snd_strerror(int(err)));
    return std::nullopt;
  }

  return std::make_pair(buf, err);
}

template <typename It> auto rms(It&& begin, It&& end) -> float {
  using namespace std::literals;
  using value_type = typename std::iterator_traits<It>::value_type;

  auto constexpr square_add = [](value_type a, value_type b) {
    auto const a_fat = fat_t{a};
    auto const b_fat = fat_t{b};

    return (a_fat * a_fat) + (b_fat * b_fat);
  };

  auto const count = end - begin;

  auto const square =
      std::accumulate(std::forward<It>(begin), std::forward<It>(end), value_type{0}, square_add);
  auto const mean_square = float(square) / float(count);

  return std::sqrt(std::abs(mean_square));
}

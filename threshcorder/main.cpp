#include <alsa/asoundlib.h>
#include <array>
#include <cstdio>
#include <fmt/core.h>
#include <memory>
#include <optional>

using namespace std::literals;

auto get_handle(std::string_view device, snd_pcm_format_t format, unsigned int rate) -> snd_pcm_t*;

template <snd_pcm_uframes_t frames>
auto listen(snd_pcm_t* const handle)
    -> std::optional<std::pair<std::array<std::uint8_t, frames>, snd_pcm_uframes_t>>;

auto main(int const argc, char const* const* const argv) -> int {
  if (argc != 2) {
    fmt::print(stderr, "Must have 2 args\n");
    return -1;
  }

  auto& dev_name = argv[1];

  auto constexpr format = SND_PCM_FORMAT_S16_LE;

  auto handle = get_handle(dev_name, format, 44100u);
  if (!handle) {
    fmt::print(stderr, "Failed to open and configure device {}\n", dev_name);
    return -1;
  }

  while (true) {
    auto const [data, count] = listen<128ul>(handle).value();

    for (snd_pcm_uframes_t j = 0; j < count; ++j)
      fmt::print("{} ", data[j]);
  }

  snd_pcm_close(handle);
}

template <snd_pcm_uframes_t frames>
auto listen(snd_pcm_t* const handle)
    -> std::optional<std::pair<std::array<std::uint8_t, frames>, snd_pcm_uframes_t>> {

  std::array<std::uint8_t, frames> buf;

  long int err;
  if ((err = snd_pcm_readi(handle, buf.data(), frames)) != frames) {
    fmt::print(stderr, "Read from audio interface failed ({})\n",
               snd_strerror(static_cast<int>(err)));
    return std::nullopt;
  }

  return std::make_pair(buf, err);
}

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

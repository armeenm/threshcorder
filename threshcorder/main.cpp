#include <alsa/asoundlib.h>
#include <array>
#include <fmt/core.h>
#include <iostream>
#include <memory>

using namespace std::literals;

auto setup(snd_pcm_t** handle, std::string_view device, snd_pcm_format_t format, unsigned int rate)
    -> int;

auto main(int argc, char** argv) -> int {
  if (argc != 2) {
    fmt::print(stderr, "Must have 2 args\n");
    return -1;
  }

  auto format = SND_PCM_FORMAT_S16_LE;

  snd_pcm_t* handle;

  auto err = setup(&handle, argv[1], format, 44100u);
  if (err)
    return -1;

  auto frames = 128l;
  auto buf = std::make_unique<uint8_t[]>(frames * snd_pcm_format_width(format) / 8);

  for (int i = 0; i < 10; ++i) {
    long int errl;
    if ((errl = snd_pcm_readi(handle, buf.get(), frames)) != frames) {
      fmt::print(stderr, "Read from audio interface failed ({})\n",
                 snd_strerror(static_cast<int>(errl)));
      return -1;
    }

    for (int j = 0; j < frames; ++j) {
      fmt::print("{}", buf[j]);
    }
  }

  snd_pcm_close(handle);
}

auto setup(snd_pcm_t** handle, std::string_view device, snd_pcm_format_t format, unsigned int rate)
    -> int {
  int err;
  snd_pcm_hw_params_t* hw_params;

  if ((err = snd_pcm_open(handle, device.data(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fmt::print(stderr, "Cannot open audio device {} ({})\n", device.data(), snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    fmt::print(stderr, "Cannot allocate hardware parameter structure ({})\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_any(*handle, hw_params)) < 0) {
    fmt::print(stderr, "Cannot initialize hardware parameter structure ({})\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_access(*handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fmt::print(stderr, "Cannot set access type ({})\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_format(*handle, hw_params, format)) < 0) {
    fmt::print(stderr, "Cannot set sample format ({})\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_rate_near(*handle, hw_params, &rate, 0)) < 0) {
    fmt::print(stderr, "Cannot set sample rate ({})\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_channels(*handle, hw_params, 2)) < 0) {
    fmt::print(stderr, "Cannot set channel count ({})\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params(*handle, hw_params)) < 0) {
    fmt::print(stderr, "Cannot set parameters ({})\n", snd_strerror(err));
    return err;
  }

  snd_pcm_hw_params_free(hw_params);

  if ((err = snd_pcm_prepare(*handle)) < 0) {
    fmt::print(stderr, "Cannot prepare audio interface for use ({})\n", snd_strerror(err));
    return err;
  }

  return 0;
}

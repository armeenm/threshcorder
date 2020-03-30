#include "threshcorder/audio.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <iostream>

using namespace std::literals;

volatile std::sig_atomic_t sigint_status;

auto sig_handler(int signal) -> void { sigint_status = signal; }

auto main(int const argc, char const* const* const argv) -> int {
  auto constexpr format = SND_PCM_FORMAT_S16_LE;
  auto constexpr rate = 44100u;
  auto constexpr threshold = 1.5f;
  auto constexpr buf_size = rate / 4;

  if (argc != 3) {
    fmt::print(stderr, "Must provide audio device name and wav folder\n");
    return -1;
  }

  std::signal(SIGINT, sig_handler);

  auto& dev_name = argv[1];

  auto handle = get_handle(dev_name, format, rate);
  if (!handle) {
    fmt::print(stderr, "Failed to open and configure device '{}'\n", dev_name);
    return -1;
  }

  if (!std::filesystem::create_directory(argv[2])) {
    fmt::print(stderr, "Failed to create output directory '{}'\n", argv[2]);
    return -1;
  }

  auto triggered = false;
  std::chrono::system_clock::time_point trigger_point;

  while (!sigint_status) {
    auto const start = std::chrono::high_resolution_clock::now();

    auto const [data, count] = listen<buf_size>(handle).value();

    auto const stop = std::chrono::high_resolution_clock::now();

    if (!triggered) {
      auto const rms_val = rms(data.begin(), data.begin() + count);

      fmt::print("RMS: {}, Count: {}", rms_val, count);
      fmt::print(", Time: {}ms",
                 std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());

      if (rms_val > threshold) {
        triggered = true;
        trigger_point = std::chrono::system_clock::now();
      }

    } else {

      triggered = ((std::chrono::system_clock::now() <= trigger_point + 5s) ||
                   (rms(data.begin(), data.begin() + count) <= threshold));
    }
  }

  fmt::print("\nDone!");

  snd_pcm_close(handle);
}

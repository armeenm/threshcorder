#include "threshcorder/audio.h"

#include <chrono>
#include <csignal>
#include <iostream>

using namespace std::literals;

volatile std::sig_atomic_t sigint_status;

auto sig_handler(int signal) -> void { sigint_status = signal; }

auto main(int const argc, char const* const* const argv) -> int {
  auto constexpr format = SND_PCM_FORMAT_S16_LE;
  auto constexpr rate = 44100u;
  auto constexpr buf_size = rate / 2;

  if (argc != 3) {
    fmt::print(stderr, "Must provide audio device name and wav filepath\n");
    return -1;
  }

  std::signal(SIGINT, sig_handler);

  auto& dev_name = argv[1];
  auto& file = argv[2];

  auto handle = get_handle(dev_name, format, rate);
  if (!handle) {
    fmt::print(stderr, "Failed to open and configure device {}\n", dev_name);
    return -1;
  }

  while (!sigint_status) {
    auto const start = std::chrono::high_resolution_clock::now();

    auto const [data, count] = listen<buf_size>(handle).value();
    auto const rms_val = rms(data.begin(), data.begin() + count);

    auto const stop = std::chrono::high_resolution_clock::now();

    fmt::print("RMS: {}, Count: {}", rms_val, count);
    fmt::print(", Time: {}ms",
               std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());

    /*
    for (auto i = 0; i < 10; ++i)
      fmt::print("{} ", data[i]);
    */

    std::cout << '\n';
  }

  fmt::print("\nDone!");

  snd_pcm_close(handle);
}

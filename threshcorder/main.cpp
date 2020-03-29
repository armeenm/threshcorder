#include "threshcorder/util.h"

#include <chrono>
#include <iostream>

using namespace std::literals;

auto main(int const argc, char const* const* const argv) -> int {
  if (argc != 2) {
    fmt::print(stderr, "Must have 2 args\n");
    return -1;
  }

  auto constexpr format = SND_PCM_FORMAT_S16_LE;
  auto constexpr rate = 44100u;
  auto constexpr buf_size = rate / 8;

  auto& dev_name = argv[1];

  auto handle = get_handle(dev_name, format, rate);
  if (!handle) {
    fmt::print(stderr, "Failed to open and configure device {}\n", dev_name);
    return -1;
  }

  while (true) {
    auto const start = std::chrono::high_resolution_clock::now();

    auto const [data, count] = listen<buf_size>(handle).value();
    auto const rms_val = rms(data.begin(), data.begin() + count);

    auto const stop = std::chrono::high_resolution_clock::now();

    fmt::print("RMS: {}, Count: {}", rms_val, count);
    fmt::print(", Time: {}ms, Samples: ",
               std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());

    for (auto i = 0; i < 10; ++i)
      fmt::print("{} ", data[i]);

    std::cout << '\n';
  }

  snd_pcm_close(handle);
}

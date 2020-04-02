#include "threshcorder/audio.h"
#include "threshcorder/wav_file.h"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <iostream>
#include <optional>
#include <utility>

using namespace std::literals;
using Event = std::pair<std::chrono::high_resolution_clock::time_point, WavFile>;

volatile std::sig_atomic_t sigint_status;

auto sig_handler(int signal) -> void { sigint_status = signal; }

auto main(int const argc, char const* const* const argv) -> int {
  auto constexpr wav_info = WavFile::Info{44100u, 1, SND_PCM_FORMAT_S16_LE};
  auto constexpr buf_size = wav_info.rate / 4;

  if (argc < 4) {
    fmt::print(stderr, "Must provide audio device name, wav folder, and threshold\n");
    return -1;
  }

  std::signal(SIGINT, sig_handler);

  auto& dev_name = argv[1];
  auto& dir = argv[2];
  auto threshold = std::stoi(argv[3]);
  auto keepalive = (argc == 5) ? std::stoi(argv[4]) : threshold;

  fmt::print("Threshold: {}\n", threshold);

  auto handle = get_handle(dev_name, wav_info.format, wav_info.rate);
  if (!handle) {
    fmt::print(stderr, "Failed to open and configure device '{}'\n", dev_name);
    return -1;
  }

  auto errc = std::error_code{};
  if (!std::filesystem::create_directories(dir, errc) && errc) {
    fmt::print(stderr, "Failed to create output directory '{}': {}\n", dir, errc.message());
    return -1;
  }

  auto event_opt = std::optional<Event>{std::nullopt};

  while (!sigint_status) {
    auto const [data, count] = listen<buf_size>(handle).value();

    // Untriggered //
    if (!event_opt) {
      auto const max_val = *std::max_element(data.begin(), data.begin() + count);

      fmt::print("Max: {}\n", max_val);

      if (max_val > threshold) {
        auto const trigger_point = std::chrono::high_resolution_clock::now();
        auto const time = std::time(nullptr);
        auto const filename =
            fmt::format("{}/{:%Y-%m-%d_%H-%M-%S}.wav", dir, *std::localtime(&time));

        fmt::print("Triggered!\n");

        event_opt = std::move(std::make_pair(trigger_point, WavFile{filename, wav_info}));
      } else
        event_opt = std::nullopt;

    } else {

      auto& [trigger_point, file] = *event_opt;

      auto const max_val = *std::max_element(data.begin(), data.begin() + count);

      fmt::print("Triggered state. Filepath: {}, Max val: {}\n", file.path().native(), max_val);

      file.append(data.begin(), count);

      auto const is_elapsed = std::chrono::system_clock::now() > trigger_point + 5s;

      if (is_elapsed && max_val <= keepalive)
        event_opt = std::nullopt;
    }
  }

  fmt::print("\nDone!");

  snd_pcm_close(handle);
}

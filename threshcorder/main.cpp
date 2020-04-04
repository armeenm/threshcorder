#include "third_party/magic_enum.hpp"
#include "threshcorder/audio.h"
#include "threshcorder/wav_file.h"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
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

  try {
    if (argc < 4)
      throw std::runtime_error(
          "Must provide at least audio device name, wav folder, and threshold");

    std::signal(SIGINT, sig_handler);

    auto dev_name = argv[1];
    auto dir = argv[2];
    auto threshold_str = argv[3];
    auto keepalive_str = argv[4];
    auto detection_method_str = argv[5];

    auto const threshold = std::strtof(threshold_str, nullptr);
    auto const keepalive = (argc >= 5) ? std::strtof(keepalive_str, nullptr) : threshold;

    auto const detection_method_opt =
        (argc >= 6) ? magic_enum::enum_cast<DetectionMethod>(detection_method_str)
                    : DetectionMethod::RMS;

    if (!detection_method_opt)
      throw std::runtime_error(fmt::format("Unknown detection method '{}'", detection_method_str));

    auto const detection_method = detection_method_opt.value();

    fmt::print("Threshold: {}\n", threshold);

    auto handle = get_handle(dev_name, wav_info.format, wav_info.rate);
    if (!handle)
      throw std::runtime_error(fmt::format("Failed to open and configure device '{}'", dev_name));

    auto errc = std::error_code{};
    if (!std::filesystem::create_directories(dir, errc) && errc)
      throw std::runtime_error(
          fmt::format("Failed to create output directory '{}': {}", dir, errc.message()));

    // Main Loop //

    auto event_opt = std::optional<Event>{std::nullopt};

    while (!sigint_status) {
      auto const [data, count] = listen<buf_size>(handle).value();
      auto const max_val = *std::max_element(data.begin(), data.begin() + count);

      // Untriggered //
      if (!event_opt) {

        fmt::print("Max: {}\n", max_val);

        if (max_val > threshold) {
          auto const trigger_point = std::chrono::high_resolution_clock::now();
          auto const time = std::time(nullptr);
          auto const filename =
              fmt::format("{}/{:%Y-%m-%d_%H-%M-%S}.wav", dir, *std::localtime(&time));

          fmt::print("Triggered!\n");

          event_opt = std::make_pair(trigger_point, WavFile{filename, wav_info});
        } else
          event_opt = std::nullopt;

        // Triggered //
      } else {

        auto& [trigger_point, file] = *event_opt;

        fmt::print("Triggered state. Filepath: {}, Max val: {}\n", file.path().native(), max_val);

        file.append(data.begin(), count);

        auto const now = std::chrono::system_clock::now();

        if (max_val > keepalive)
          event_opt->first = now;
        else if (now > trigger_point + 5s)
          event_opt = std::nullopt;
      }
    }

    snd_pcm_close(handle);

  } catch (std::exception const& e) {
    fmt::print(stderr, "Failed: {}", e.what());
    return -1;
  }

  fmt::print("\nDone!");
}

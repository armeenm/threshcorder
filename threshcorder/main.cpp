#include "threshcorder/audio.h"
#include "threshcorder/wav_file.h"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <optional>
#include <utility>

using namespace std::literals;
using Event = std::pair<std::chrono::system_clock::time_point, WavFile>;

volatile std::sig_atomic_t sigint_status;

auto sig_handler(int signal) -> void { sigint_status = signal; }

auto main(int const argc, char const* const* const argv) -> int {
  auto constexpr wav_info = WavFile::Info{44100u, 1, SND_PCM_FORMAT_S16_LE};
  auto constexpr threshold = 1.5f;
  auto constexpr buf_size = wav_info.rate / 4;

  if (argc != 3) {
    fmt::print(stderr, "Must provide audio device name and wav folder\n");
    return -1;
  }

  std::signal(SIGINT, sig_handler);

  auto& dev_name = argv[1];
  auto& dir = argv[2];

  auto handle = get_handle(dev_name, wav_info.format, wav_info.rate);
  if (!handle) {
    fmt::print(stderr, "Failed to open and configure device '{}'\n", dev_name);
    return -1;
  }

  if (!std::filesystem::create_directory(dir)) {
    fmt::print(stderr, "Failed to create output directory '{}'\n", dir);
    return -1;
  }

  auto iter = [&](std::optional<Event>&& event_opt = std::nullopt) -> std::optional<Event> {
    auto const [data, count] = listen<buf_size>(handle).value();

    // Untriggered //
    if (!event_opt) {
      auto const rms_val = rms(data.begin(), data.begin() + count);

      fmt::print("RMS: {}\n", rms_val);

      if (rms_val > threshold) {
        auto const trigger_point = std::chrono::system_clock::now();
        return std::make_pair(trigger_point, WavFile(fmt::format("temp.wav"), wav_info));
      } else
        return std::nullopt;

    } else {
      auto& [time, file] = *event_opt;

      file.append(data.begin(), count);

      auto const is_elapsed = std::chrono::system_clock::now() > time + 5s;

      if (is_elapsed && (rms(data.begin(), data.begin() + count) <= threshold))
        return std::nullopt;

      return std::move(event_opt);
    }
  };

  /*
  auto event_opt = iter();

  while (!sigint_status)
    event_opt = iter(std::move(event_opt));
    */

  auto x = WavFile("temp.wav", wav_info);
  auto y = std::array<std::int16_t, 3>{1, 2, 3};

  x.append(y.data(), y.size());

  fmt::print("\nDone!");

  snd_pcm_close(handle);
}

#pragma once

#include <alsa/asoundlib.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <type_traits>

class WavFile {
public:
  struct Info {
    std::uint32_t rate;
    std::uint16_t channels;
    snd_pcm_format_t format;
    bool overwrite = false;
  };

  WavFile(std::filesystem::path path, Info info);

  WavFile(WavFile const&) = delete;
  WavFile(WavFile&&) noexcept = default;

  auto operator=(WavFile const&) -> WavFile& = delete;
  auto operator=(WavFile&&) noexcept -> WavFile& = default;

  ~WavFile();

  [[nodiscard]] auto path() const noexcept -> std::filesystem::path;
  [[nodiscard]] auto info() const noexcept -> Info;

  auto append(const void* data, std::size_t size) -> std::size_t;

private:
  std::filesystem::path path_;
  Info info_;
  FILE* fd_;
};

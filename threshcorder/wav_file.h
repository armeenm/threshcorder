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
  WavFile(WavFile&&) noexcept;

  auto operator=(WavFile const&) -> WavFile& = delete;
  auto operator=(WavFile&&) noexcept -> WavFile&;

  ~WavFile();

  [[nodiscard]] auto path() const noexcept -> std::filesystem::path;
  [[nodiscard]] auto info() const noexcept -> Info;

  auto append(const void* data, std::size_t size) -> bool;

private:
  std::filesystem::path path_;
  Info info_;
  std::uint16_t sample_size_ = std::uint16_t(snd_pcm_format_width(info_.format) / 8);
  FILE* fd_ = nullptr;
  std::uint32_t subchunk2_size_ = 0;

  template <typename T> auto write_elem_(T const* buf) const -> void {
    if (std::fwrite(buf, sizeof(*buf), 1, fd_) != 1)
      throw std::runtime_error("Failed to write element to file");
  }

  auto cleanup_fd() noexcept -> void;
};

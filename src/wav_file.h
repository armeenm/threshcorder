#pragma once

#include <alsa/asoundlib.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <memory>
#include <type_traits>

struct FileCloser {
  auto operator()(std::FILE* fp) const noexcept -> void {
    std::cout << "Closing " << fp << '\n';
    std::fclose(fp);
  }
};

class WavFile {
public:
  struct Info {
    std::uint32_t rate;
    std::uint16_t channels;
    snd_pcm_format_t format;
  };

  WavFile(std::filesystem::path path, Info info, bool overwrite = false);

  WavFile(WavFile const&) = delete;
  WavFile(WavFile&&) noexcept = default;

  auto operator=(WavFile const&) -> WavFile& = delete;
  auto operator=(WavFile&&) noexcept -> WavFile& = default;

  ~WavFile();

  [[nodiscard]] auto path() const noexcept -> std::filesystem::path;
  [[nodiscard]] auto info() const noexcept -> Info;

  auto append(const void* data, std::size_t size) -> bool;

private:
  std::filesystem::path path_;
  Info info_;
  std::unique_ptr<std::FILE, FileCloser> fp_ = nullptr;
  std::uint16_t sample_size_ = std::uint16_t(snd_pcm_format_width(info_.format) / 8);
  std::uint32_t subchunk2_size_ = 0;

  template <typename T> auto write_elem_(T const* buf) const -> void {
    if (std::fwrite(buf, sizeof(*buf), 1, fp_.get()) != 1)
      throw std::runtime_error("Failed to write element to file");
  }
};

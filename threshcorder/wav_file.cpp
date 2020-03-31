#include "threshcorder/wav_file.h"

WavFile::WavFile(std::filesystem::path path, Info info)
    : path_(std::move(path)), info_(std::move(info)) {
  if (std::filesystem::exists(path_) && !info.overwrite)
    throw std::runtime_error(
        fmt::format("File {} exists and overwriting is disallowed", path_.native()));

  if (!(fd_ = std::fopen(path_.c_str(), "w")))
    throw std::runtime_error(
        fmt::format("Could not open file {}: {}", path_.native(), std::strerror(errno)));

  auto const to_uint32_p = [](auto&& ptr) {
    return reinterpret_cast<std::uint32_t const*>(std::forward<decltype(ptr)>(ptr));
  };

  // ChunkID
  auto const chunk_id_p = to_uint32_p("RIFF");
  std::fwrite(chunk_id_p, sizeof(*chunk_id_p), 1, fd_);

  // ChunkSize
  // Should be updated after each append
  auto const chunk_size = std::uint32_t{36};
  std::fwrite(&chunk_size, sizeof(chunk_size), 1, fd_);

  // Format
  auto const format_p = to_uint32_p("WAVE");
  std::fwrite(format_p, sizeof(*format_p), 1, fd_);

  // Subchunk1ID
  auto const subchunk1_id_p = to_uint32_p("fmt ");
  std::fwrite(subchunk1_id_p, sizeof(*subchunk1_id_p), 1, fd_);

  // Subchunk1Size
  auto constexpr subchunk1_size = std::uint32_t{16};
  std::fwrite(&subchunk1_size, sizeof(subchunk1_size), 1, fd_);

  // AudioFormat
  auto constexpr audio_format = std::uint16_t{1};
  std::fwrite(&audio_format, sizeof(audio_format), 1, fd_);

  // NumChannels
  std::fwrite(&info_.channels, sizeof(info_.channels), 1, fd_);

  // SampleRate
  std::fwrite(&info_.rate, sizeof(info_.rate), 1, fd_);

  // ByteRate, BlockAlign, BitsPerSample
  auto const bits_per_sample = static_cast<std::uint16_t>(snd_pcm_format_width(info_.format));
  auto const block_align = static_cast<std::uint16_t>(info_.channels * bits_per_sample / 8);
  auto const byte_rate = info_.rate * block_align;

  std::fwrite(&byte_rate, sizeof(byte_rate), 1, fd_);
  std::fwrite(&block_align, sizeof(block_align), 1, fd_);
  std::fwrite(&bits_per_sample, sizeof(bits_per_sample), 1, fd_);

  // Subchunk2ID
  auto const subchunk2_id_p = to_uint32_p("data");
  std::fwrite(subchunk2_id_p, sizeof(*subchunk2_id_p), 1, fd_);

  // Subchunk2Size
  // Should be updated after each append
  auto constexpr subchunk2_size = std::uint32_t{0};
  std::fwrite(&subchunk2_size, sizeof(subchunk2_size), 1, fd_);
}

WavFile::~WavFile() { std::fclose(fd_); }

auto WavFile::path() const noexcept -> std::filesystem::path { return path_; }
auto WavFile::info() const noexcept -> WavFile::Info { return info_; }

auto WavFile::append(const void* data, std::size_t const size) -> std::size_t {
  return std::fwrite(data, size, 1, fd_);
}

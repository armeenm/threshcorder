#include "wav_file.h"

#include <iostream>

WavFile::WavFile(std::filesystem::path path, Info info, bool overwrite)
    : path_(std::move(path)), info_(std::move(info)) {

  if (std::filesystem::exists(path_) && !overwrite)
    throw std::runtime_error(
        fmt::format("File {} exists and overwriting is disallowed", path_.native()));

  fp_.reset(std::fopen(path_.c_str(), "w"));

  if (!fp_)
    throw std::runtime_error(
        fmt::format("Could not open file {}: {}", path_.native(), std::strerror(errno)));

  auto const to_uint32_p = [](auto&& ptr) {
    return reinterpret_cast<std::uint32_t const*>(std::forward<decltype(ptr)>(ptr));
  };

  // ChunkID
  auto const chunk_id_p = to_uint32_p("RIFF");
  write_elem_(chunk_id_p);

  // Skip ChunkSize
  // Should be updated after each append
  std::fseek(fp_.get(), 4, SEEK_CUR);

  // Format
  auto const format_p = to_uint32_p("WAVE");
  write_elem_(format_p);

  // Subchunk1ID
  auto const subchunk1_id_p = to_uint32_p("fmt ");
  write_elem_(subchunk1_id_p);

  // Subchunk1Size
  auto constexpr subchunk1_size = std::uint32_t{16};
  write_elem_(&subchunk1_size);

  // AudioFormat
  auto constexpr audio_format = std::uint16_t{1};
  write_elem_(&audio_format);

  // NumChannels
  write_elem_(&info_.channels);

  // SampleRate
  write_elem_(&info_.rate);

  // ByteRate, BlockAlign
  auto const block_align = std::uint32_t(info_.channels * sample_size_);
  auto const byte_rate = std::uint16_t(info_.rate * block_align);

  write_elem_(&byte_rate);
  write_elem_(&block_align);

  // BitsPerSample
  auto const bits_per_sample = std::uint16_t(sample_size_ * 8);
  write_elem_(&bits_per_sample);

  // Subchunk2ID
  auto const subchunk2_id_p = to_uint32_p("data");
  write_elem_(subchunk2_id_p);

  // Subchunk2Size
  // Should be updated after each append
  std::fseek(fp_.get(), 4, SEEK_CUR);
}

WavFile::~WavFile() {
  if (fp_) {
    auto fp = fp_.get();

    // ChunkSize
    std::fseek(fp, 4, SEEK_SET);
    auto const chunk_size = 36 + subchunk2_size_;
    std::fwrite(&chunk_size, sizeof(chunk_size), 1, fp);

    // Subchunk2Size
    std::fseek(fp, 40, SEEK_SET);
    std::fwrite(&subchunk2_size_, sizeof(subchunk2_size_), 1, fp);
  }
}

auto WavFile::path() const noexcept -> std::filesystem::path { return path_; }
auto WavFile::info() const noexcept -> WavFile::Info { return info_; }

auto WavFile::append(const void* const data, std::size_t const count) -> bool {
  auto const written = std::fwrite(data, sample_size_, count, fp_.get());
  if (written != count)
    return false;

  subchunk2_size_ += std::uint32_t(count * info_.channels * sample_size_);

  return true;
}

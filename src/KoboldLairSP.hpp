#pragma once

#include <MagicScrollBB>

using ByteBuffer    = MagicScrollBB;
using buffer_size_t = Buffer::buffer_size_t;

class KoboldLairSP {
public:
  using block_id_t = std::int32_t;
  using slot_id_t  = std::int32_t;

  // sizeof(block_id_t) == 4 bytes
  static const buffer_offset_t CURR_BLOCK_ID_OFFSET =  0;

  // pointer to the available free space on the page
  static const buffer_offset_t FREE_SPACE_OFFSET    =  4;

  // current slot count
  static const buffer_offset_t SLOT_COUNT_OFFSET    =  8;

  // Width of the header
  static const buffer_offset_t SLOTTED_PAGE_HEADER  =  12;

  // The offset of the first tuple
  static const buffer_offset_t TUPLE_OFFSET_OFFSET  = 12;

  // The size of the first tuple
  static const buffer_offset_t TUPLE_SIZE_OFFSET    = 16;

  // The size of a tuple slot
  static const int32_t         TUPLE_SLOT_SIZE      =  8;

  KoboldLairSP(buffer_size_t buffer_size,
               block_id_t block_id)
    : buffer_        (buffer_size),
      curr_block_id_ (block_id)
  {
    write_curr_block_id(block_id);
    write_free_space_offset(buffer_size);
    write_slot_count(0);
  }

  /*
   * read/write_curr_block_id
   */

  void
  write_curr_block_id(block_id_t block_id) noexcept {
    buffer_.write_block_id(CURR_BLOCK_ID_OFFSET,
                           curr_block_id);
  }

  block_id_t
  read_curr_block_id() const noexcept {
    return buffer_.read_block_id(CURR_BLOCK_ID_OFFSET);
  }

  /*
   * read/write_slot_count
   */

  void
  write_slot_count(int32_t slot_count) noexcept {
    buffer_.write_int32(SLOT_COUNT_OFFSET,
                        slot_count);
  }

  /*
   * read_slot_count -- Returns the maximum slot ID
   */
  slot_id_t
  read_slot_count() const noexcept {
    return buffer_.read_int32(SLOT_COUNT_OFFSET);
  }

  /*
   * space_available -- Space available, in bytes
   */

  buffer_size_t
  space_available() const noexcept {
    return read_free_space_offset() -
      TUPLE_BLOCK_HEADER - TUPLE_SLOT_SIZE * read_slot_count();
  }

  /*
   * read_slot_as_buffer
   */

  const ByteBuffer
  read_slot_as_buffer(slot_id_t slot_id)
    const noexcept
  {
    auto offset      = read_slot_offset_at(slot_id);
    auto buffer_size = read_slot_size_at(slot_id);
    auto new_buffer  = ByteBuffer(buffer_size);
    buffer_.read_buffer(offset, new_buffer);
    return new_buffer;
  }

  /*
   * read/write_slot_offset_at
   */

  buffer_offset_t
  read_slot_offset_at(slot_id_t slot_id) const noexcept
  {
    auto offset = TUPLE_OFFSET_OFFSET + (TUPLE_SLOT_SIZE * slot_id);
    return buffer_.read_int32(offset);
  }

  void
  write_slot_offset_at(slot_id_t slot_id,
                       buffer_offset_t slot_offset) noexcept
  {
    auto offset = TUPLE_OFFSET_OFFSET + (TUPLE_SLOT_SIZE * slot_id);
    buffer_.write_int32(offset, slot_offset);
  }

  /*
   * read/write_slot_size_at
   */

  void
  write_slot_size_at(slot_id_t slot_id, int32_t slot_size) const noexcept {
    auto offset = TUPLE_SIZE_OFFSET + (TUPLE_SLOT_SIZE * slot_id);
    buffer_.write_int32(offset, slot_size);
  }

  int32_t
  read_slot_size_at(slot_id_t slot_id) const noexcept {
    auto offset = TUPLE_SIZE_OFFSET + (TUPLE_SLOT_SIZE * slot_id);
    return buffer_.read_int32(offset);
  }

  /*
   * mark_slot_as_deleted / apply_deletion
   */

  void
  mark_slot_as_deleted(slot_id_t slot_id)
    const noexcept
  {
    auto size = read_slot_size_at(slot_id);
    // NOTE: We leave immediately if the tuple is already
    // marked as deleted.
    if (!is_deleted(size)) {
      write_slot_size_at(slot_id, -size);
    }
  }

  void
  apply_deletion(slot_id_t slot_id)
    noexcept
  {
    auto deleted_buffer_offset = read_slot_offset_at(slot_id);
    auto deleted_buffer_size   = read_slot_size_at(slot_id);
    if (is_deleted(deleted_buffer_size)) {
      deleted_buffer_size = -deleted_buffer_size;
    }

    auto free_space_offset = read_free_space_offset();
    auto from_offset       = free_space_offset;
    auto to_offset         = free_space_offset + deleted_buffer_size;

    // Move data and over-write the the deleted tuple
    buffer_.copy(from_offset,
                 to_offset,
                 deleted_read_offset - free_space_offset);

    // Update the slot
    write_free_space_offset(to_offset);
    write_slot_size_at(slot_id, 0);
    write_slot_offset_at(slot_id, 0);

    // Cleanup: Update slots that came "before" the deleted tuple
    auto max_slot_id = read_slot_count();
    for (slot_id_t i = 0; i < max_slot_id; ++i) {
      auto slot_offset = read_slot_offset_at(i);
      auto slot_size   = read_slot_size_at(i);
      if (slot_size != 0 && slot_offset < deleted_buffer_offset) {
        write_slot_offset_at(i, slot_offset + deleted_buffer_size);
      }
    }
  }

  bool
  is_deleted(int32_t size) const noexcept {
    // NOTE: Marking a tuple as deleted is done via
    // using a negative number for the tuple size.
    return size < 0;
  }

  /*
   * underlying buffer
   */

  ByteBuffer&
  buffer() noexcept {
    return buffer_;
  }

  const ByteBuffer&
  buffer() const noexcept {
    return buffer_;
  }

private:
  block_id_t curr_block_id_;
  ByteBuffer buffer_;
};

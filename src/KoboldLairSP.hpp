#pragma once

#include <MagicScrollBB.hpp>

using ByteBuffer      = MagicScrollBB;
using buffer_offset_t = ByteBuffer::buffer_offset_t;
using buffer_size_t   = ByteBuffer::buffer_size_t;

class KoboldLairSP {
public:
  using block_id_t = std::int32_t;
  using slot_id_t  = std::int32_t;
  const slot_id_t INVALID_SLOT_ID = -1;

  // sizeof(block_id_t) == 4 bytes
  static const buffer_offset_t CURR_BLOCK_ID_OFFSET =  0;

  // pointer to the available free space on the page
  static const buffer_offset_t FREE_SPACE_OFFSET    =  4;

  // current slot count
  static const buffer_offset_t SLOT_COUNT_OFFSET    =  8;

  // Width of the header
  static const buffer_offset_t SLOTTED_PAGE_HEADER  =  12;

  // The offset of the first buffer
  static const buffer_offset_t BUFFER_OFFSET_OFFSET  = 12;

  // The size of the first tuple
  static const buffer_offset_t BUFFER_SIZE_OFFSET    = 16;

  // The size of a tuple slot
  static const int32_t         BUFFER_SLOT_SIZE      =  8;

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
   * space_available -- Space available, in bytes
   */

  buffer_size_t
  space_available() const noexcept {
    return read_free_space_offset() -
      SLOTTED_PAGE_HEADER - BUFFER_SLOT_SIZE * read_slot_count();
  }

  /*
   * can_contain_buffer -- Determines if the underlying page buffer
   *                       can contain the given buffer.
   */

  bool
  can_contain(buffer_size_t buffer_size) const noexcept {
    return space_available() > buffer_size;
  }

  /*
   * read/write_curr_block_id
   */

  void
  write_curr_block_id(block_id_t block_id) noexcept {
    curr_block_id_ = block_id;
    buffer_.write_int32(CURR_BLOCK_ID_OFFSET,
                        block_id);
  }

  block_id_t
  read_curr_block_id() const noexcept {
    return buffer_.read_int32(CURR_BLOCK_ID_OFFSET);
  }

  /*
   * read/write_slot_count
   */

  int32_t
  read_slot_count() const noexcept {
    return buffer_.read_int32(SLOT_COUNT_OFFSET);
  }

  void
  write_slot_count(int32_t slot_count) noexcept {
    buffer_.write_int32(SLOT_COUNT_OFFSET,
                        slot_count);
  }

  /*
   * first_free_slot
   */

  slot_id_t
  first_free_slot() const noexcept {
    slot_id_t slot_id = 0;
    int32_t count = read_slot_count();
    for (; slot_id < count; ++slot_id) {
      if (slot_is_empty(slot_id)) {
        return slot_id;
      }
    }
    return slot_id;
  }

  /*
   * slot_is_empty
   */

  bool
  slot_is_empty(slot_id_t slot_id) const noexcept {
    return read_slot_size_at(slot_id) == 0;
  }

  /*
   * read/write_free_space_offset
   */

  buffer_offset_t
  read_free_space_offset() const noexcept {
    return buffer_.read_offset(FREE_SPACE_OFFSET);
  }

  void
  write_free_space_offset(buffer_offset_t offset) noexcept {
    buffer_.write_offset(FREE_SPACE_OFFSET,
                         offset);
  }

  /*
   * read_slot_buffer
   */

  ByteBuffer const
  read_slot_buffer(slot_id_t slot_id)
    const noexcept
  {
    auto offset      = read_slot_offset_at(slot_id);
    auto buffer_size = read_slot_size_at(slot_id);
    auto new_buffer  = ByteBuffer(buffer_size);
    buffer_.read_buffer(offset, new_buffer);
    return new_buffer;
  }

  /*
   * mark_slot_as_deleted
   */

  void
  mark_slot_as_deleted(slot_id_t slot_id)
    noexcept
  {
    auto size = read_slot_size_at(slot_id);
    // NOTE: We leave immediately if the record is already
    // marked as deleted.
    if (!is_deleted(size)) {
      write_slot_size_at(slot_id, -size);
    }
  }

  /*
   * insert_slot
   */

  std::tuple<bool, slot_id_t>
  insert_slot(ByteBuffer const& buffer) {
    auto available = space_available();
    if (available < buffer.size() + BUFFER_SLOT_SIZE) {
      return std::make_tuple(false, INVALID_SLOT_ID);
    }

    // Find the first free slot
    auto free_slot_id = first_free_slot();
    // Claim the space by writing updating the free space offset
    write_free_space_offset(read_free_space_offset() - buffer.size());
    // Copy the data into the space we've claimed
    buffer_.write_buffer(read_free_space_offset(), buffer);
    // Check if it's the last slot
    if (free_slot_id == read_slot_count()) {
      // If it is, increase the slot count in the page.
      write_slot_count(free_slot_id + 1);
    }

    // Update the slot size & offset
    write_slot_offset_at(free_slot_id, read_free_space_offset());
    write_slot_size_at(free_slot_id, buffer.size());

    return std::make_tuple(true, free_slot_id);
  }

  /*
   * apply_deletion
   */

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

    // Move data and over-write the the deleted buffer
    buffer_.copy(from_offset,
                 to_offset,
                 deleted_buffer_offset - free_space_offset);

    // Cleanup: Update the free space offset, and the size & offset of the deleted slot.
    write_free_space_offset(to_offset);
    write_slot_size_at(slot_id, 0);
    write_slot_offset_at(slot_id, 0);

    // Cleanup: Update slot offsets that came "before" the deleted buffer offset
    auto max_slot_id = read_slot_count();
    for (slot_id_t i = 0; i < max_slot_id; ++i) {
      auto slot_offset = read_slot_offset_at(i);
      auto slot_size   = read_slot_size_at(i);
      if (slot_size != 0 && slot_offset < deleted_buffer_offset) {
        write_slot_offset_at(i, slot_offset + deleted_buffer_size);
      }
    }

    // TODO: Final cleanup
    // Update the slot_array by removing entries with size == 0

    // To delete the slot from the slot_array, we have to do the same copy operation we did
    // in the buffer_.copy line above. Except it's on the slot_array

    if (read_slot_count() == slot_id) {
      // TODO: Handle the case where we remove an element from the end of
      // the slot_array
      trim_slot_array(slot_id);
    } else {
      // TODO: We are removing one element, so we can copy other elements
      copy_over_slot_id(slot_id);
    }
  }

  void trim_slot_array(slot_id_t slot_id) noexcept {
    // Overwrite the slot offset with 0
    auto offset_offset = slot_buffer_offset_for(slot_id);
    buffer_.write_int32(offset_offset, 0);

    // Overwrite the slot size with 0
    auto size_offset   = BUFFER_SIZE_OFFSET   + (slot_id * BUFFER_SLOT_SIZE);
    buffer_.write_int32(size_offset, 0);

    write_slot_count(slot_id);
  }

  void copy_over_slot_id(slot_id_t curr_slot_id) noexcept {
    auto curr_slot_offset = slot_buffer_offset_for(curr_slot_id);

    auto next_slot_id     = curr_slot_id + 1;
    auto next_slot_offset = slot_buffer_offset_for(next_slot_id);

    auto max_slot_id      = read_slot_count();
    auto max_slot_offset  = slot_buffer_offset_for(max_slot_id);
    auto width_in_bytes   = max_slot_offset - next_slot_offset;

    buffer_.copy(next_slot_offset,
                 curr_slot_offset,
                 width_in_bytes);

    write_slot_count(max_slot_id - 1);
  }

  buffer_offset_t
  slot_buffer_offset_for(slot_id_t slot_id) const noexcept {
    return BUFFER_OFFSET_OFFSET + (slot_id * BUFFER_SLOT_SIZE);
  }

  buffer_offset_t
  slot_size_offset_for(slot_id_t slot_id) const noexcept {
    return BUFFER_OFFSET_OFFSET + (slot_id * BUFFER_SLOT_SIZE);
  }

  /*
   * is_deleted
   */

  bool
  is_deleted(int32_t size) const noexcept {
    // NOTE: Marking a buffer as deleted is done via
    // using a negative number for the buffer size.
    return size < 0;
  }

  /*
   * read/write_slot_offset_at
   */

  buffer_offset_t
  read_slot_offset_at(slot_id_t slot_id) const noexcept
  {
    auto offset = BUFFER_OFFSET_OFFSET + (BUFFER_SLOT_SIZE * slot_id);
    return buffer_.read_int32(offset);
  }

  void
  write_slot_offset_at(slot_id_t slot_id,
                       buffer_offset_t slot_offset) noexcept
  {
    auto offset = BUFFER_OFFSET_OFFSET + (BUFFER_SLOT_SIZE * slot_id);
    buffer_.write_int32(offset, slot_offset);
  }

  /*
   * read/write_slot_size_at
   */

  void
  write_slot_size_at(slot_id_t slot_id, int32_t slot_size) noexcept {
    auto offset = BUFFER_SIZE_OFFSET + (BUFFER_SLOT_SIZE * slot_id);
    buffer_.write_int32(offset, slot_size);
  }

  int32_t
  read_slot_size_at(slot_id_t slot_id) const noexcept {
    auto offset = BUFFER_SIZE_OFFSET + (BUFFER_SLOT_SIZE * slot_id);
    return buffer_.read_int32(offset);
  }

  /*
   * underlying buffer
   */

  ByteBuffer&
  buffer() noexcept {
    return buffer_;
  }

  ByteBuffer const&
  buffer() const noexcept {
    return buffer_;
  }

private:
  block_id_t curr_block_id_;
  ByteBuffer buffer_;
};

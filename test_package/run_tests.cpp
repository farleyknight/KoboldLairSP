#include <fmt/core.h>
#include <gtest/gtest.h>

#include <MagicScrollBB.hpp>
#include <KoboldLairSP.hpp>

using ByteBuffer  = MagicScrollBB;
using SlottedPage = KoboldLairSP;


TEST(KoboldLairSP, FirstSlotIDIsZero) {
  auto page = SlottedPage(1024, 1);

  auto [success, slot_id] = page.insert_slot(ByteBuffer::from_string("hello, world!"));

  EXPECT_EQ(success, true);
  EXPECT_EQ(slot_id, 0);
}

TEST(KoboldLairSP, InsertSlotThenReadSlot) {
  auto page = SlottedPage(1024, 1);

  auto test_string = "hello, world!";

  auto [success, slot_id] = page.insert_slot(ByteBuffer::from_string(test_string));

  auto string_buffer = page.read_slot_buffer(slot_id);

  auto actual_string = string_buffer.read_string(0);

  EXPECT_EQ(actual_string, test_string);
}

TEST(KoboldLairSP, InsertSlotThenDeleteSlot) {
  auto page = SlottedPage(1024, 1);
  EXPECT_EQ(page.read_slot_count(), 0);
  auto start_size = page.space_available();

  fmt::print("The starting size of the page is {}\n", start_size);

  // Create a test string and insert it into the page.
  auto test_string = "hello, world!";
  auto [success, slot_id] = page.insert_slot(ByteBuffer::from_string(test_string));

  // TODO: Create new method `page.delete_slot(slot_id)`
  // that does the work of these two:
  page.mark_slot_as_deleted(slot_id);
  page.apply_deletion(slot_id);

  // Slot count should go back to zero
  EXPECT_EQ(page.read_slot_count(), 0);

  // Expect the page size to go back to it's starting size
  EXPECT_EQ(page.space_available(), start_size);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

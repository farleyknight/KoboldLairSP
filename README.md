# KoboldLairSP

(SP = Slotted Page)

* Store variable-length records in a data buffer. 
* Built on top of `MagicScrollBB`.

## How to use

```c++
#include <fmt/core.h>
#include <MagicScrollBB>
#include <KoboldLairSP>

using ByteBuffer  = MagicScrollBB;
using SlottedPage = KoboldLairSP;

int main() {
  // First start with some data you want to store in a buffer
  auto buffer = ByteBuffer::from_string("Hello, world!");
  // Create a slotted page...
  auto page = SlottedPage(1024);
  // Then insert the buffer into a new slot on the page. 
  auto slot_id = page.insert_slot(buffer);
  // The first slot_id is 0
  fmt::print("The first slot_id == {}\n", slot_id);
  
  // Besides the actual `data`, the page will hold some extra 
  // metadata about the number of slots it has.
  auto curr_slot_count = page.slot_count();
  fmt::print("The page now has {} elements\n", curr_slot_count);

  // It also knows how much space is remaining
  auto available      = page.available_space();
  fmt::print("The page has {} free bytes\n", available);
  
  // We can use that slot to read it back out
  auto slot_buffer    = page.read_slot(slot_id);
  fmt::print("The buffer has string: {}", slot_buffer.read_string());
  
  // Deletion is a two step process
  // This is to support DB transactions
  // - First mark the slot as deleted
  page.mark_slot_as_delete(slot_id);
  // The count is decremented
  fmt::print("The page now has {} elements\n", page.slot_count());
  // But we still have the same available space
  fmt::print("Available space: {}\n", page.space_available());
  
  // To regain that lost space, we have to apply the deletion.
  page.apply_deletion(slot_id);
  auto available      = page.space_available();
  fmt::print("Avilable space: {}\n", available);  
}
```

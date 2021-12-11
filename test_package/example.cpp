#include <fmt/core.h>
#include <KoboldLairSP.hpp>

using SlottedPage = KoboldLairSP;

int main() {
  auto page = SlottedPage(1024);

  auto string_buffer = Buffer::from_string("hello, world!");

  auto slot_id = page.insert_slot(string_buffer);

  auto slot_count = page.slot_count();
  fmt::print("Number of slot counts: {}", slot_count);

  auto slot_buffer = page.read_slot(slot_id);
  fmt::print("The slot buffer has data: {}", slot_buffer.read_string());


  KoboldLairSP();
}

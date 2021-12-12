#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <fmt/core.h>
#include <KoboldLairSP.hpp>

using SlottedPage = KoboldLairSP;
using slot_id_t   = SlottedPage::slot_id_t;

class StringGenerator {
public:
  StringGenerator() {
    std::ifstream words_file("words.txt");

    std::string line;
    while (std::getline(words_file, line)) {
      words_.push_back(line);
    }
  }

  const std::string
  generate() const noexcept {
    return words_[random_index()];
  }

  int32_t
  random_index() const noexcept {
    return std::rand() % words_.size();
  }

private:
  std::vector<std::string> words_;
};

int main() {
  const auto PAGE_SIZE     = 1024;
  const auto CURR_BLOCK_ID = 0;
  auto page                = SlottedPage(PAGE_SIZE, CURR_BLOCK_ID);

  fmt::print("The page was just created. It currently has size: {}\n", page.space_available());

  auto [success, slot_id] = page.insert_slot(ByteBuffer::from_string("hello, world!"));

  auto slot_count = page.read_slot_count();
  fmt::print("Number of slot counts: {}\n", slot_count);

  auto slot_buffer = page.read_slot_buffer(slot_id);
  fmt::print("The slot buffer has data: {}\n", slot_buffer.read_string());

  auto page_size = page.buffer().size();
  fmt::print("How many random strings can we fit inside a page of size: {}?\n", page_size);
  fmt::print("Let's find out, by generating a bunch of random strings and adding them to the page.\n");

  auto gen           = StringGenerator();
  auto random_string = gen.generate();

  int32_t count = 0;
  // TODO: I would like to find a way to encode the netstring size of a string
  //
  // I think maybe a new component can be created from this?
  // ElvenChainNS
  //
  // NS = NetString -- Simple implementation of a netstring via a `struct net_string`
  // and a `class NetString` that interoperate as needed.
  while (page.can_contain(random_string.size() + sizeof(int32_t))) {
    auto [success, slot_id] = page.insert_slot(ByteBuffer::from_string(random_string));
    if (!success) {
      fmt::print("Could not insert random_string == {}\n", random_string);
      break;
    }

    fmt::print("Inserted random_string == {} into slot_id == {}\n",
               random_string, slot_id);
    count += 1;
    random_string = gen.generate();
  }

  fmt::print("We managed to fit {} random strings into a page\n", count);
  fmt::print("Now let's delete them all!\n");

  while (count >= 0) {
    slot_id_t slot_id = count;
    auto slot_buffer = page.read_slot_buffer(slot_id);
    auto slot_string = slot_buffer.read_string();
    fmt::print("Reading slot w/ string data: {}\n", slot_string);

    fmt::print("Marking deletion for slot ID: {}\n", slot_id);
    page.mark_slot_as_deleted(slot_id);

    fmt::print("Applying deletion for slot ID: {}\n", slot_id);
    page.apply_deletion(slot_id);

    count -= 1;
  }

  fmt::print("The page should now be empty! It currently has size: {}\n", page.space_available());
}

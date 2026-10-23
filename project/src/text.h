#ifndef NANO_EDR_TEXT_H
#define NANO_EDR_TEXT_H

#include <cstddef>
#include <string>

namespace nano_edr {

bool StartsAt(const std::string& text, std::size_t pos,
              const std::string& word);
bool TextEquals(const std::string& text, const std::string& value);
bool TextContains(const std::string& text, const std::string& part);
bool TextEndsWith(const std::string& text, const std::string& suffix);

}  // namespace nano_edr

#endif  // NANO_EDR_TEXT_H

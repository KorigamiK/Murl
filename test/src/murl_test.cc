#include <string>

#include "murl/murl.hh"

auto main() -> int
{
  auto const exported = exported_class {};

  return std::string("murl") == exported.name() ? 0 : 1;
}

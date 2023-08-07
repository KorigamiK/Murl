#include <string>

#include "murl/murl.hh"

exported_class::exported_class()
    : m_name {"murl"}
{
}

auto exported_class::name() const -> char const*
{
  return m_name.c_str();
}

#pragma once
#include <string>

namespace vsqlite_utils {
  void BytesToHexString(const std::string &bytes, std::string &dest);
  void HexStringToBinString(const std::string str, std::string &dest);
}

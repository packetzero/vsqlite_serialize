#include "utils.h"
namespace vsqlite_utils {
  static const char hexCharsLower[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
  };
  static const char hexCharsUpper[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
  };


  void BytesToHexString(const unsigned char *bytes, size_t len, std::string &dest)
  {
    static bool useLowerCase = true;
    const char *hexChars = hexCharsLower;
    if (!useLowerCase) hexChars = hexCharsUpper;

    const unsigned char *src = bytes;
    size_t offset = dest.length();
    dest.resize(offset + len*2);
    for (size_t i=0;i < len; i++) {
      char b = *src++;
      dest[offset++] = hexChars[ ((b >> 4) & 0x0F) ];
      dest[offset++] = hexChars[ (b & 0x0F) ];
    }
  }

  void BytesToHexString(const std::string &bytes, std::string &dest)
  {
    BytesToHexString((const unsigned char *)bytes.data(), bytes.size(), dest);
  }

  static uint8_t hexDigitValue(char c)
  {
    if (c >= 'a') {
      return (uint8_t)(c - 'a') + 10;
    } else if (c >= 'A') {
      return (uint8_t)(c - 'A') + 10;
    }
    return (uint8_t)(c - '0');
  }

  void HexStringToBinString(const std::string str, std::string &dest)
  {
    dest.resize(str.length() / 2);
    auto p = (uint8_t*)dest.data();
    for (int i=0;i < str.length(); i+=2) {
      uint8_t val = hexDigitValue(str[i]) << 4 | hexDigitValue(str[i+1]);
      *p++ = val;
      //dest.push_back(val);
    }
  }

}

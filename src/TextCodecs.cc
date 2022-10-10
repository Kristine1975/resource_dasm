#include "TextCodecs.hh"

#include <phosg/Strings.hh>

// MacRoman to UTF-8
static constexpr const char mac_roman_table[0x100][4] = {
  // 00
  // Note: we intentionally incorrectly decode \r as \n here to convert CR line
  // breaks to LF line breaks which modern systems use
  "\x00", "\x01", "\x02", "\x03", "\x04", "\x05", "\x06", "\x07",
  "\x08", "\t", "\n", "\x0B", "\x0C", "\n", "\x0E",  "\x0F",
  // 10
  "\x10", "\xE2\x8C\x98", "\xE2\x87\xA7", "\xE2\x8C\xA5",
  "\xE2\x8C\x83", "\x15", "\x16", "\x17",
  "\x18", "\x19", "\x1A", "\x1B", "\x1C", "\x1D", "\x1E", "\x1F",
  // 20
  " ", "!", "\"", "#", "$", "%", "&", "\'",
  "(", ")", "*", "+", ",", "-", ".", "/",
  // 30
  "0", "1", "2", "3", "4", "5", "6", "7",
  "8", "9", ":", ";", "<", "=", ">", "?",
  // 40
  "@", "A", "B", "C", "D", "E", "F", "G",
  "H", "I", "J", "K", "L", "M", "N", "O",
  // 50
  "P", "Q", "R", "S", "T", "U", "V", "W",
  "X", "Y", "Z", "[", "\\", "]", "^", "_",
  // 60
  "`", "a", "b", "c", "d", "e", "f", "g",
  "h", "i", "j", "k", "l", "m", "n", "o",
  // 70
  "p", "q", "r", "s", "t", "u", "v", "w",
  "x", "y", "z", "{", "|", "}", "~", "\x7F",
  // 80
  "\xC3\x84", "\xC3\x85", "\xC3\x87", "\xC3\x89",
  "\xC3\x91", "\xC3\x96", "\xC3\x9C", "\xC3\xA1",
  "\xC3\xA0", "\xC3\xA2", "\xC3\xA4", "\xC3\xA3",
  "\xC3\xA5", "\xC3\xA7", "\xC3\xA9", "\xC3\xA8",
  // 90
  "\xC3\xAA", "\xC3\xAB", "\xC3\xAD", "\xC3\xAC",
  "\xC3\xAE", "\xC3\xAF", "\xC3\xB1", "\xC3\xB3",
  "\xC3\xB2", "\xC3\xB4", "\xC3\xB6", "\xC3\xB5",
  "\xC3\xBA", "\xC3\xB9", "\xC3\xBB", "\xC3\xBC",
  // A0
  "\xE2\x80\xA0", "\xC2\xB0", "\xC2\xA2", "\xC2\xA3",
  "\xC2\xA7", "\xE2\x80\xA2", "\xC2\xB6", "\xC3\x9F",
  "\xC2\xAE", "\xC2\xA9", "\xE2\x84\xA2", "\xC2\xB4",
  "\xC2\xA8", "\xE2\x89\xA0", "\xC3\x86", "\xC3\x98",
  // B0
  "\xE2\x88\x9E", "\xC2\xB1", "\xE2\x89\xA4", "\xE2\x89\xA5",
  "\xC2\xA5", "\xC2\xB5", "\xE2\x88\x82", "\xE2\x88\x91",
  "\xE2\x88\x8F", "\xCF\x80", "\xE2\x88\xAB", "\xC2\xAA",
  "\xC2\xBA", "\xCE\xA9", "\xC3\xA6", "\xC3\xB8",
  // C0
  "\xC2\xBF", "\xC2\xA1", "\xC2\xAC", "\xE2\x88\x9A",
  "\xC6\x92", "\xE2\x89\x88", "\xE2\x88\x86", "\xC2\xAB",
  "\xC2\xBB", "\xE2\x80\xA6", "\xC2\xA0", "\xC3\x80",
  "\xC3\x83", "\xC3\x95", "\xC5\x92", "\xC5\x93",
  // D0
  "\xE2\x80\x93", "\xE2\x80\x94", "\xE2\x80\x9C", "\xE2\x80\x9D",
  "\xE2\x80\x98", "\xE2\x80\x99", "\xC3\xB7", "\xE2\x97\x8A",
  "\xC3\xBF", "\xC5\xB8", "\xE2\x81\x84", "\xE2\x82\xAC",
  "\xE2\x80\xB9", "\xE2\x80\xBA", "\xEF\xAC\x81", "\xEF\xAC\x82",
  // E0
  "\xE2\x80\xA1", "\xC2\xB7", "\xE2\x80\x9A", "\xE2\x80\x9E",
  "\xE2\x80\xB0", "\xC3\x82", "\xC3\x8A", "\xC3\x81",
  "\xC3\x8B", "\xC3\x88", "\xC3\x8D", "\xC3\x8E",
  "\xC3\x8F", "\xC3\x8C", "\xC3\x93", "\xC3\x94",
  // F0
  "\xEF\xA3\xBF", "\xC3\x92", "\xC3\x9A", "\xC3\x9B",
  "\xC3\x99", "\xC4\xB1", "\xCB\x86", "\xCB\x9C",
  "\xC2\xAF", "\xCB\x98", "\xCB\x99", "\xCB\x9A",
  "\xC2\xB8", "\xCB\x9D", "\xCB\x9B", "\xCB\x87",
};

/*static constexpr char8_t MAC_ROMAN[128][4] = {
  u8"\u00C4", // Latin capital letter a with diaeresis
  u8"\u00C5", // Latin capital letter a with ring above
  u8"\u00C7", // Latin capital letter c with cedilla
  u8"\u00C9", // Latin capital letter e with acute
  u8"\u00D1", // Latin capital letter n with tilde
  u8"\u00D6", // Latin capital letter o with diaeresis
  u8"\u00DC", // Latin capital letter u with diaeresis
  u8"\u00E1", // Latin small letter a with acute
  u8"\u00E0", // Latin small letter a with grave
  u8"\u00E2", // Latin small letter a with circumflex
  u8"\u00E4", // Latin small letter a with diaeresis
  u8"\u00E3", // Latin small letter a with tilde
  u8"\u00E5", // Latin small letter a with ring above
  u8"\u00E7", // Latin small letter c with cedilla
  u8"\u00E9", // Latin small letter e with acute
  u8"\u00E8", // Latin small letter e with grave
  u8"\u00EA", // Latin small letter e with circumflex
  u8"\u00EB", // Latin small letter e with diaeresis
  u8"\u00ED", // Latin small letter i with acute
  u8"\u00EC", // Latin small letter i with grave
  u8"\u00EE", // Latin small letter i with circumflex
  u8"\u00EF", // Latin small letter i with diaeresis
  u8"\u00F1", // Latin small letter n with tilde
  u8"\u00F3", // Latin small letter o with acute
  u8"\u00F2", // Latin small letter o with grave
  u8"\u00F4", // Latin small letter o with circumflex
  u8"\u00F6", // Latin small letter o with diaeresis
  u8"\u00F5", // Latin small letter o with tilde
  u8"\u00FA", // Latin small letter u with acute
  u8"\u00F9", // Latin small letter u with grave
  u8"\u00FB", // Latin small letter u with circumflex
  u8"\u00FC", // Latin small letter u with diaeresis
  u8"\u2020", // Dagger
  u8"\u00B0", // Degree sign
  u8"\u00A2", // Cent sign
  u8"\u00A3", // Pound sign
  u8"\u00A7", // Section sign
  u8"\u2022", // Bullet
  u8"\u00B6", // Pilcrow sign
  u8"\u00DF", // Latin small letter sharp s
  u8"\u00AE", // Registered sign
  u8"\u00A9", // Copyright sign
  u8"\u2122", // Trade mark sign
  u8"\u00B4", // Acute accent
  u8"\u00A8", // Diaeresis
  u8"\u2260", // Not equal to
  u8"\u00C6", // Latin capital letter ae
  u8"\u00D8", // Latin capital letter o with stroke
  u8"\u221E", // Infinity
  u8"\u00B1", // Plus-minus sign
  u8"\u2264", // Less-than or equal to
  u8"\u2265", // Greater-than or equal to
  u8"\u00A5", // Yen sign
  u8"\u00B5", // Micro sign
  u8"\u2202", // Partial differential
  u8"\u2211", // N-ary summation
  u8"\u220F", // N-ary product
  u8"\u03C0", // Greek small letter pi
  u8"\u222B", // Integral
  u8"\u00AA", // Feminine ordinal indicator
  u8"\u00BA", // Masculine ordinal indicator
  u8"\u03A9", // Greek capital letter omega
  u8"\u00E6", // Latin small letter ae
  u8"\u00F8", // Latin small letter o with stroke
  u8"\u00BF", // Inverted question mark
  u8"\u00A1", // Inverted exclamation mark
  u8"\u00AC", // Not sign
  u8"\u221A", // Square root
  u8"\u0192", // Latin small letter f with hook
  u8"\u2248", // Almost equal to
  u8"\u2206", // Increment
  u8"\u00AB", // Left-pointing double angle quotation mark
  u8"\u00BB", // Right-pointing double angle quotation mark
  u8"\u2026", // Horizontal ellipsis
  u8"\u00A0", // No-break space
  u8"\u00C0", // Latin capital letter a with grave
  u8"\u00C3", // Latin capital letter a with tilde
  u8"\u00D5", // Latin capital letter o with tilde
  u8"\u0152", // Latin capital ligature oe
  u8"\u0153", // Latin small ligature oe
  u8"\u2013", // En dash
  u8"\u2014", // Em dash
  u8"\u201C", // Left double quotation mark
  u8"\u201D", // Right double quotation mark
  u8"\u2018", // Left single quotation mark
  u8"\u2019", // Right single quotation mark
  u8"\u00F7", // Division sign
  u8"\u25CA", // Lozenge
  u8"\u00FF", // Latin small letter y with diaeresis
  u8"\u0178", // Latin capital letter y with diaeresis
  u8"\u2044", // Fraction slash
  u8"\u20AC", // Euro sign
  u8"\u2039", // Single left-pointing angle quotation mark
  u8"\u203A", // Single right-pointing angle quotation mark
  u8"\uFB01", // Latin small ligature fi
  u8"\uFB02", // Latin small ligature fl
  u8"\u2021", // Double dagger
  u8"\u00B7", // Middle dot
  u8"\u201A", // Single low-9 quotation mark
  u8"\u201E", // Double low-9 quotation mark
  u8"\u2030", // Per mille sign
  u8"\u00C2", // Latin capital letter a with circumflex
  u8"\u00CA", // Latin capital letter e with circumflex
  u8"\u00C1", // Latin capital letter a with acute
  u8"\u00CB", // Latin capital letter e with diaeresis
  u8"\u00C8", // Latin capital letter e with grave
  u8"\u00CD", // Latin capital letter i with acute
  u8"\u00CE", // Latin capital letter i with circumflex
  u8"\u00CF", // Latin capital letter i with diaeresis
  u8"\u00CC", // Latin capital letter i with grave
  u8"\u00D3", // Latin capital letter o with acute
  u8"\u00D4", // Latin capital letter o with circumflex
  u8"\uF8FF", // Apple logo
  u8"\u00D2", // Latin capital letter o with grave
  u8"\u00DA", // Latin capital letter u with acute
  u8"\u00DB", // Latin capital letter u with circumflex
  u8"\u00D9", // Latin capital letter u with grave
  u8"\u0131", // Latin small letter dotless i
  u8"\u02C6", // Modifier letter circumflex accent
  u8"\u02DC", // Small tilde
  u8"\u00AF", // Macron
  u8"\u02D8", // Breve
  u8"\u02D9", // Dot above
  u8"\u02DA", // Ring above
  u8"\u00B8", // Cedilla
  u8"\u02DD", // Double acute accent
  u8"\u02DB", // Ogonek
  u8"\u02C7", // Caron
};

string decode_mac_roman(char data, bool for_filename) {
  if (for_filename && should_escape_mac_roman_filename_char(data)) {
    return "_";
  } else if (data & 0x80) {
    return reinterpret_cast<const char*>(MAC_ROMAN[data & 0x7F]);
  } else {
    return mac_roman_table[static_cast<uint8_t>(data)];
  }
}*/


static string decode_mac_roman(char data, bool for_filename) {
  if (for_filename && should_escape_mac_roman_filename_char(data)) {
    return "_";
  } else {
    return mac_roman_table[static_cast<uint8_t>(data)];
  }
}


string decode_mac_roman(const char* data, size_t size, bool for_filename) {
  string ret;
  while (size--) {
    ret += decode_mac_roman(*data++, for_filename);
  }
  return ret;
}

string decode_mac_roman(const string& data, bool for_filename) {
  return decode_mac_roman(data.data(), data.size(), for_filename);
}


string string_for_resource_type(uint32_t type, bool for_filename) {
  string result;
  for (ssize_t s = 24; s >= 0; s -= 8) {
    uint8_t ch = (type >> s) & 0xFF;
    if (ch < 0x20 || (for_filename && should_escape_mac_roman_filename_char(ch))) {
      result += string_printf("\\x%02hhX", ch);
    } else if (ch == '\\') {
      result += "\\\\";
    } else  {
      result += decode_mac_roman(ch, for_filename);
    }
  }
  return result;
}

string raw_string_for_resource_type(uint32_t type) {
  string result;
  for (ssize_t s = 24; s >= 0; s -= 8) {
    result += static_cast<char>((type >> s) & 0xFF);
  }
  return result;
}

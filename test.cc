#include <iostream>
#include <cstring>
#include <vector>
#include <memory>
#include <iomanip>

#include "passende.hh"

using namespace passende;
using std::skipws;
using std::noskipws;

int main(int argc, char* argv[]) {
  if(argc < 2 || argc > 5) {
    std::cerr <<
      "\n"
      "Usage: " << argv[0] << " TYPE [SCRAMBLE [DIGITS_PER_GROUP [ROUNDMODE]]]\n"
      "\n"
      "TYPE is either a passende code to decode, or the empty string (\"\") to choose\n"
      "to encode a code.\n"
      "SCRAMBLE is a Passende-encoded scramble code.\n"
      "DIGITS_PER_GROUP is between 0 and 79 (default is 7)\n"
      "ROUNDMODE is one of:\n"
      "-pd | Pad with zeros to a whole number of digits\n"
      "-pg | Pad with zeros to a whole number of groups\n"
      "-cd | Pad with check bits to a whole number of digits\n"
      "-cg | Pad with check bits to a whole number of groups\n"
      "\n"
      "Encoder commands:\n"
      "            0 | Add a zero bit\n"
      "            1 | Add a one bit\n"
      "n <l> <h> <n> | Add an integer <n>, encoded in the range [<l>,<h>]\n"
      "            C | Add an explicit check bit\n"
      "\n"
      "Decoder commands:\n"
      "       . or ? | Get a single bit\n"
      "    n <l> <h> | Get an integer encoded in the range [<l>,<h>]\n"
      "            C | Test an explicit check bit\n"
      "\n"
      "On non-Windows platforms, you can end input with control-D. On Windows, you\n"
      "can use control-Z. On either platform, you can end input with an exclamation\n"
      "mark (!).\n";
    return 1;
  }
  std::vector<bool> scramble;
  if(argc >= 3) {
    if(De::code_to_bits(argv[2], scramble) != Result::OKAY) {
      std::cerr << "invalid scramble code\n";
      return 1;
    }
  }
  int digits_per_group;
  if(argc >= 4) {
    digits_per_group = std::stoi(argv[3]);
    if(digits_per_group < 0 || digits_per_group > 79) {
      std::cerr << "third argument must be a number between 0 and 79 inclusive\n";
      return 1;
    }
  } else digits_per_group = 7;
  RoundMode round_mode;
  if(argc >= 5) {
    if(!strcmp(argv[4], "-pd")) round_mode = RoundMode::PAD_TO_DIGIT;
    else if(!strcmp(argv[4], "-pg")) round_mode = RoundMode::PAD_TO_GROUP;
    else if(!strcmp(argv[4], "-cd")) round_mode = RoundMode::CHECK_TO_DIGIT;
    else if(!strcmp(argv[4], "-cg")) round_mode = RoundMode::CHECK_TO_GROUP;
    else {
      std::cerr << "fourth argument must be one of -pd -cd -pg -cg\n";
      return 1;
    }
  } else round_mode = RoundMode::CHECK_TO_GROUP;
  if(argv[1][0]) {
    De de(argv[1], scramble, digits_per_group, round_mode);
    while(de.check_error() == Result::OKAY && std::cin) {
      char cmd;
      std::cin >> noskipws >> cmd;
      if(!std::cin) break;
      switch(cmd) {
      case ' ': case '\n': std::cout << cmd; break;
      case '?': case '.': {
        bool bit = de.read_bit();
        if(de.check_error() == Result::OKAY) {
          std::cout << (bit ? '1' : '0');
        }
        break;
      }
      case 'n': {
        int min, max, val;
        std::cin >> skipws >> min >> skipws >> max;
        if(!std::cin) {
          std::cerr << "n requires valid min max\n";
          return 1;
        }
        val = de.read_int(min, max);
        if(de.check_error() == Result::OKAY) {
          std::cout << val;
        }
        break;
      }
      case 'c': case 'C': de.read_check_bit(); break;
      case '!': goto owari_de;
      default:
        std::cerr << "Unknown command character!\n";
        return 1;
      }
    }
  owari_de:
    switch(de.finish()) {
      case Result::OKAY:
        std::cout << "OKAY\n";
        break;
      case Result::TOO_SHORT:
        std::cout << "TOO_SHORT\n";
        break;
      case Result::TOO_LONG:
        std::cout << "TOO_LONG\n";
        break;
      case Result::OUT_OF_RANGE:
        std::cout << "OUT_OF_RANGE\n";
        break;
      case Result::BAD_DIGIT:
        std::cout << "BAD_DIGIT\n";
        break;
      case Result::BAD_CHECK_BIT:
        std::cout << "BAD_CHECK_BIT\n";
        break;
    }
    return 0;
  }
  else {
    En en(scramble, digits_per_group, round_mode);
    while(true) {
      char cmd;
      std::cin >> noskipws >> cmd;
      if(!std::cin) break;
      switch(cmd) {
      case ' ': case '\n': break;
      case '0': en.write_bit(false); break;
      case '1': en.write_bit(true); break;
      case 'n': {
        int min, max, val;
        std::cin >> skipws >> min >> skipws >> max >> skipws >> val;
        if(!std::cin) {
          std::cerr << "n requires valid min max val\n";
          return 1;
        }
        en.write_int(min, max, val);
        break;
      }
      case 'c': case 'C': en.write_check_bit(); break;
      case '!': goto owari_en;
      default:
        std::cerr << "Unknown command character!\n";
        return 1;
      }
    }
  owari_en:
    std::cout << '"' << en.get_result() << "\"\n"
      "check bits: " << en.get_num_check_bits_had() << "\n";
    return 0;
  }
}

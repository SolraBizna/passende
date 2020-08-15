#include "passende.hh"

#include <cassert>

using namespace passende;

namespace {
  const char* DIGITS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ236789";
  const uint8_t ARABIC_DIGITS_TO_VALUES[10] = {
    14, 8, 26, 27, 0, 18, 28, 29, 30, 31
  };
  bool digit_to_value(char d, uint8_t& val) {
    // lowercase to uppercase
    if(d >= 'a' && d <= 'z') d = d & 0xDF;
    else if(d == '4') return false; // one billion people don't like this digit
    if(d >= 'A' && d <= 'Z') {
      val = (d - 'A');
      return true;
    }
    else if(d >= '0' && d <= '9') {
      val = ARABIC_DIGITS_TO_VALUES[d - '0'];
      return true;
    }
    else return false;
  }
  uint32_t update_crc(uint32_t crc, bool bit) {
    return (bit ? 0xEDB88320 : 0) ^ (crc >> 1);
  }
}

En::En(std::vector<bool> scramble,
       unsigned int digits_per_group, RoundMode round_mode)
  : result(), scramble(std::move(scramble)),
    scramble_pos(this->scramble.cbegin()), running_check(~uint32_t(0)),
    digits_per_group(digits_per_group), rem_in_group(digits_per_group),
    valid_cur_check_bits(0), num_check_bits_had(0), current_bits(0),
    next_bit(1<<4) {
  switch(round_mode) {
  case RoundMode::PAD_TO_DIGIT:
    pad_with_checks = false;
    pad_to_group = false;
    break;
  case RoundMode::CHECK_TO_DIGIT:
    pad_with_checks = true;
    pad_to_group = false;
    break;
  case RoundMode::PAD_TO_GROUP:
    pad_with_checks = false;
    pad_to_group = true;
    break;
  case RoundMode::CHECK_TO_GROUP:
    pad_with_checks = true;
    pad_to_group = true;
    break;
  }
}

void En::enc_bit(bool bit) {
  if(!scramble.empty()) {
    if(*scramble_pos++) bit = !bit;
    if(scramble_pos == scramble.cend()) scramble_pos = scramble.cbegin();
  }
  if(bit) current_bits |= next_bit;
  next_bit = next_bit >> 1;
  if(next_bit == 0) {
    if(digits_per_group > 0) {
      // add a space before the first digit in any group EXCEPT the first.
      if(!result.empty() && rem_in_group == digits_per_group) {
        result.push_back(' ');
      }
      if(--rem_in_group == 0) rem_in_group = digits_per_group;
    }
    result.push_back(DIGITS[current_bits]);
    current_bits = 0;
    next_bit = 1<<4;
  }
}

void En::write_bit(bool bit) {
  // CRC the non-scrambled bit
  valid_cur_check_bits = 0;
  running_check = update_crc(running_check, bit);
  enc_bit(bit);
}

void En::write_check_bit() {
  if(valid_cur_check_bits == 0) {
    cur_check = running_check;
    valid_cur_check_bits = 32;
  }
  enc_bit((cur_check & 1) != 0);
  cur_check = cur_check >> 1;
  --valid_cur_check_bits;
  ++num_check_bits_had;
}

void En::write_check_bits(unsigned int n) {
  while(n-- > 0) write_check_bit();
}

void En::write_pad_bit() {
  if(pad_with_checks) write_check_bit();
  else enc_bit(false);
}

const std::string& En::get_result() {
  while(next_bit != (1<<4)) write_pad_bit();
  if(pad_to_group) while(rem_in_group != digits_per_group) write_pad_bit();
  return result;
}

De::De(std::string code, std::vector<bool> scramble,
       unsigned int digits_per_group, RoundMode round_mode)
  : code(code), pos(this->code.cbegin()), scramble(std::move(scramble)),
    scramble_pos(this->scramble.cbegin()), running_check(~uint32_t(0)),
    digits_per_group(digits_per_group), rem_in_group(digits_per_group),
    valid_cur_check_bits(0), result(Result::OKAY), current_bits(0),
    next_bit(0) {
  switch(round_mode) {
  case RoundMode::PAD_TO_DIGIT:
    pad_with_checks = false;
    pad_to_group = false;
    break;
  case RoundMode::CHECK_TO_DIGIT:
    pad_with_checks = true;
    pad_to_group = false;
    break;
  case RoundMode::PAD_TO_GROUP:
    pad_with_checks = false;
    pad_to_group = true;
    break;
  case RoundMode::CHECK_TO_GROUP:
    pad_with_checks = true;
    pad_to_group = true;
    break;
  }
}

bool De::get_bit() {
  if(result != Result::OKAY) return false;
  if(next_bit == 0) {
    if(digits_per_group > 0 && --rem_in_group == 0)
      rem_in_group = digits_per_group;
    while(pos != code.cend() && *pos == ' ') ++pos;
    if(pos == code.cend()) {
      result = Result::TOO_SHORT;
      return false;
    }
    if(!digit_to_value(*pos++, current_bits)) {
      result = Result::BAD_DIGIT;
      return false;
    }
    next_bit = (1<<4);
  }
  bool ret = (current_bits & next_bit) != 0;
  next_bit >>= 1;
  if(!scramble.empty()) {
    if(*scramble_pos++) ret = !ret;
    if(scramble_pos == scramble.cend()) scramble_pos = scramble.cbegin();
  }
  return ret;
}

bool De::read_bit() {
  if(result != Result::OKAY) return false;
  bool ret = get_bit();
  valid_cur_check_bits = 0;
  running_check = update_crc(running_check, ret);
  return ret;
}

void De::read_check_bit() {
  if(result != Result::OKAY) return;
  if(valid_cur_check_bits == 0) {
    cur_check = running_check;
    valid_cur_check_bits = 32;
  }
  bool wanted = (cur_check & 1) != 0;
  cur_check = cur_check >> 1;
  --valid_cur_check_bits;
  bool got = get_bit();
  if(wanted != got) result = Result::BAD_CHECK_BIT;
}

void De::read_check_bits(unsigned int n) {
  while(n-- > 0) read_check_bit();
}

void De::read_pad_bit() {
  if(pad_with_checks) read_check_bit();
  else if(get_bit()) result = Result::BAD_CHECK_BIT;
}

Result De::finish() {
  while(result == Result::OKAY && next_bit != 0) read_pad_bit();
  if(pad_to_group) while(result == Result::OKAY
                         && rem_in_group != digits_per_group) read_pad_bit();
  while(result == Result::OKAY && pos != code.cend()) {
    if(*pos++ != ' ') result = Result::TOO_LONG;
  }
  return result;
}

Result De::code_to_bits(std::string code, std::vector<bool>& bits) {
  De r(code);
  while(true) {
    bool x = r.read_bit();
    if(r.check_error() != Result::OKAY) break;
    bits.push_back(x);
  }
  if(r.check_error() == Result::TOO_SHORT) return Result::OKAY;
  else return r.check_error();
}

std::vector<bool> De::assert_code_to_bits(std::string code) {
  std::vector<bool> ret;
  Result res = code_to_bits(code, ret);
  assert(res == Result::OKAY);
  // in case _NDEBUG is defined, we really do not want to allow a non-okay res
  if(res != Result::OKAY) abort();
  return ret;
}

#ifndef PASSENDEHH
#define PASSENDEHH

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace passende {
  enum class Result { OKAY, BAD_DIGIT, BAD_CHECK_BIT, TOO_SHORT, TOO_LONG,
                      OUT_OF_RANGE };
  enum class RoundMode { PAD_TO_DIGIT, CHECK_TO_DIGIT,
                         PAD_TO_GROUP, CHECK_TO_GROUP };
  class En {
    std::string result;
    const std::vector<bool> scramble;
    std::vector<bool>::const_iterator scramble_pos;
    uint32_t running_check, cur_check;
    unsigned int digits_per_group, rem_in_group, valid_cur_check_bits,
      num_check_bits_had;
    uint8_t current_bits, next_bit;
    bool pad_with_checks, pad_to_group;
    void enc_bit(bool);
    void write_pad_bit();
  public:
    En(std::vector<bool> scramble = std::vector<bool>(),
       unsigned int digits_per_group = 7,
       RoundMode round_mode = RoundMode::CHECK_TO_GROUP);
    void write_bit(bool);
    template<typename I> void write_int(I min, I max, I value) {
      if(min == max) return;
      else if(min > max) {
        I x(std::move(min));
        min = std::move(max);
        max = std::move(x);
      }
      if(value < min) value = min;
      else if(value > max) value = max;
      value = value - min;
      I range = max - min;
      int needed_bits = 1;
      while((static_cast<I>(1) << needed_bits) <= range
            && (static_cast<I>(1) << needed_bits) != 0) {
        ++needed_bits;
      }
      while(needed_bits > 0) {
        I next_bit = static_cast<I>(1) << --needed_bits;
        write_bit((value & next_bit) != 0);
      }
    }
    void write_check_bit();
    void write_check_bits(unsigned int);
    const std::string& get_result();
    unsigned int get_num_check_bits_had() const { return num_check_bits_had; }
  };
  class De {
    const std::string code;
    std::string::const_iterator pos;
    const std::vector<bool> scramble;
    std::vector<bool>::const_iterator scramble_pos;
    uint32_t running_check, cur_check;
    unsigned int digits_per_group, rem_in_group, valid_cur_check_bits;
    Result result;
    uint8_t current_bits, next_bit;
    bool pad_with_checks, pad_to_group;
    bool get_bit();
    void read_pad_bit();
  public:
    De(std::string code, std::vector<bool> scramble = std::vector<bool>(),
       unsigned int digits_per_group = 7,
       RoundMode round_mode = RoundMode::CHECK_TO_GROUP);
    bool read_bit();
    template<typename I> I read_int(I min, I max) {
      if(min == max) return min;
      else if(min > max) {
        I x(std::move(min));
        min = std::move(max);
        max = std::move(x);
      }
      I range = max - min;
      int needed_bits = 1;
      while((static_cast<I>(1) << needed_bits) <= range
            && (static_cast<I>(1) << needed_bits) != 0) {
        ++needed_bits;
      }
      I ret(0);
      while(needed_bits > 0) {
        I next_bit = static_cast<I>(1) << --needed_bits;
        if(read_bit()) ret |= next_bit;
      }
      if(ret > range) {
        result = Result::OUT_OF_RANGE;
        return max;
      }
      else {
        return ret + min;
      }
    }
    void read_check_bit();
    void read_check_bits(unsigned int n);
    Result check_error() const { return result; }
    Result finish();
    static Result code_to_bits(std::string code, std::vector<bool>& bits);
    static std::vector<bool> assert_code_to_bits(std::string code);
  };
}

#endif

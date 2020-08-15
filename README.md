Some classic retro games (such as Metroid and Mega Man), rather than including storage in which to save game state between sessions, use a "passcode" system. Instead of loading a save, the player enters a passcode the game previously gave them. For example, entering the following passcode at Metroid's "continue" screen will start the player in the final area with 250 missiles, all energy tanks, and most upgrades:

```
X----- --N?WO
dV-Gm9 W01GMI
```

Including SRAM on which to save a game, and a battery so the SRAM could retain its data for a while without external power, added significantly to the cost of a game cartridge. Passcode systems didn't need any expensive hardware, just a few hundred bytes of extra program code to encode and decode passcodes. Time and technology have erased the cost advantage, but other advantages remain; one major advantage is that a player's progress isn't tied to the specific system or cartridge with which they made it.

Passende is a simple passcode system inspired by those classic schemes. The name refers both to the encoding scheme and to this library. Encoded Passende strings are called passendecodes, or passcodes for short.

Passende divides the data into five-bit "digits", and groups those digits into an arbitrary number of fixed-size groups. Each digit corresponds to a Latin letter or Arabic digit. Since this gives 36 digits and we only need 32, four of the digits are removed:

- 0: Looks too similar to the letter O.
- 1: Looks too similar to the letter I (capital i).
- 4: Over one billion people around the world [hate this digit][1].
- 5: Looks too similar to the letter S.

[1]: https://en.wikipedia.org/wiki/Tetraphobia "Tetraphobia"

Bits are encoded most-significant-bit first.

To break up the monotony of long strings of 0 or 1 bits in a passcode, Passende supports "scramble codes". Every 1 bit in the scramble code flips the corresponding input bit. For example, the `A`- and `Q`-studded code `QAIAIQ DBAAAC`, combined with a scramble code `JUSTIN BAILEY`, becomes `ZU2TA7 CBILE2`. If you want to obfuscate your codes further, do like Metroid did and add random bits on encode which you then ignore on decode.

Passende supports ["check bits"](#check-bits). By default, it automatically adds check bits to pad a passcode to a whole number of groups. You can also explicitly add check bits, but there is normally no need to do this; the best way to include check bits in a code is to deliberately leave some digits of "spare room" after encoding. One check bit means roughly 50% of mistakes will be caught, and each check bit afterward doubles the detection rate, up to 32 bits.

This library is not very performant. It's explicitly designed to be written as expressively and safely as reasonable. It could probably run ten times faster or more if it were optimized. But, do you really need to decode 1&nbsp;000&nbsp;000 passcodes per second?

# Building

Add `passende.cc` to your build, and set up your include paths such that `passende.hh` will be seen.

If you want to try the test program that comes with this repository, and are on a UNIX-ish system (such as Linux, macOS, or MinGW), and have `make` and a C++ compiler installed, you may just be able to do:

```sh
make
```

and get a program named `passendetest` that provides a clumsy and crude (but complete) human interface to this library.

# API

This section describes the C++ API. Bindings to other languages should work similarly.

All use of the API requires you to `#include "passende.hh"`. Everything resides in the `passende` namespace.

## Encoding

Create an `En`, with the following optional constructor parameters:

- `std::vector<bool> scramble = (empty)`: A scramble code. May be an empty vector, in which case there will be no scrambling. (The `code_to_bits` static method of [`De`](#decoding) can convert a "raw passcode" to a scramble code.)
- `unsigned int digits_per_group = 7`: The number of digits per fixed-size group. May be zero, in which case all digits will be in one big group. Seven is the largest number most humans can easily [subitize][2], but in theory any value up to `UINT_MAX` will work.
- `RoundMode round_mode = RoundMode::CHECK_TO_GROUP`: How to pad the passcode. See [Rounding](#rounding).

[2]: https://en.wikipedia.org/wiki/Subitizing "Subitizing"

Once it's created, you can call the following members:

- `void write_bit(bool)`: Encode an individual bit.
- `void write_int(I min, I max, I value)`: Encode an integer `value` that can be anywhere between `min` and `max` inclusive. Any `int`-like type can be used for `I`, as long as it supports comparison and bit operations.
- `void write_check_bit()`: Write a [check bit](#check-bits). You shouldn't normally need this, as check bits are usually added automatically at the end of a passcode, and inline check bits can only detect errors in preceding parts of the passcode.
- `void write_check_bits(unsigned int)`: Write the given number of check bits. You may want to do e.g. `write_check_bits(5)` just before calling `get_result` if you want to guarantee a certain *minimum* number of check bits.
- `const std::string& get_result()`: Add pad or check bits as needed according to the [round mode](#rounding), and return the fully encoded passcode. (If you write more bits and then call this a second time, you've made a passcode that will be annoying to decode, so don't do that.)
- `unsigned int get_num_check_bits_had() const`: Returns the number of check bits that have been written so far. You can call this after `get_result` to make sure that a certain minimum number of check bits are present.

### Examples

```c++
passende::En en();
en.write_int(0, 20, 19);
en.write_int(0, 999, 641);
en.write_int(0, 10, 10);
en.write_int(0, 50, 11);
for(bool x : {false, false, false, false, false,
               true, false, false, false}) {
    en.write_bit(x);
}
std::cout << en.get_result() << "\n" << en.get_num_check_bits_had() << "\n";
// will print:
// TUBULAR
// 1
```

```C++
passende::En en(std::vector<bool>(), 4, passende::RoundMode::CHECK_TO_DIGIT);
en.write_int(0, 200000, 108561);
en.write_int(0, 200000, 4998);
en.write_int(0, 100, 36);
en.write_bit(true);
en.write_int(0, 100, 17);
en.write_bit(false);
std::cout << en.get_result() << "\n" << en.get_num_check_bits_had() << "\n";
// will print:
// NICE CODE SIR
// 3
```

## Decoding

Create a `De`. The first parameter to its constructor is mandatory, and is the passcode to decode.

Optional parameters:

- `std::vector<bool> scramble = (empty)`: A scramble code. May be an empty vector, in which case there will be no scrambling. (The `code_to_bits` function can convert a "raw passcode" to a scramble code.)
- `unsigned int digits_per_group = 7`: The number of digits per fixed-size group. Only significant if `round_mode` is `CHECK_TO_GROUP` or `PAD_TO_GROUP`, in which case it affects how many pad digits are expected.
- `RoundMode round_mode = RoundMode::CHECK_TO_GROUP`: How to pad the passcode. See [Rounding](#rounding).

Like `En`, `De` does not return errors in-band. Unlike `En`, it's possible for an error to occur from which recovery is not possible. `De` methods will return nonsense (but safe) values after an error occurs. You must check the return value of `finish` after decoding to see if an error occurred. You may also use `check_error` to check for an error at any time. Both functions return a `Result`, whose values are:

- `Result::OKAY`: Everything okay so far.
- `Result::BAD_DIGIT`: Hit an invalid character.
- `Result::BAD_CHECK_BIT`: A [check bit](#check-bits) didn't match.
- `Result::TOO_SHORT`: The passcode didn't contain enough digits.
- `Result::TOO_LONG`: There was (non-space) data in the string after the end of the passcode.
- `Result::OUT_OF_RANGE`: `read_int` saw a number that was out of the specified range.

While decoding, `De` interprets lowercase letters as their uppercase counterparts, and the digits `0`, `1`, and `5` as their counterpart letters `O`, `I` (capital `i`), and `S`. It also ignores ASCII space. Any other character leads to `Result::BAD_DIGIT`.

You must decode elements in the same order as they were originally encoded. The following members are available:

- `bool read_bit()`: Decode an individual bit.
- `I read_int(I min, I max)`: Decode an integer value that can be anywhere between `min` and `max` inclusive. Any `int`-like type can be used for `I`, as long as it supports comparison and bit operations.
- `void read_check_bit()`: Read and test a [check bit](#check-bits). You shouldn't normally need this, as check bits are usually added automatically at the end of a passcode, and inline check bits can only detect errors in preceding parts of the code.
- `void read_check_bits(unsigned int)`: Read and test the given number of check bits. You shouldn't normally need this either, unless the passcode encoding process had a `write_check_bits` call in it.
- `Result check_error() const`: If an error has already been detected, return it. Otherwise, return `Result::OKAY`.
- `Result finish()`: Test any pad or check bits that `En::get_result` added, and return whether or not an error was found.

The following two **static** methods are also available, mainly to make it easy to deal with "scramble codes":

- `Result code_to_bits(std::string code, std::vector<bool>& bits)`: Read all raw bits from the digits in `code`, putting them one by one into the `bits` vector as encountered. Returns `Result::OKAY` if there were no invalid digits or other problems, and an error otherwise.
- `std::vector<bool> assert_code_to_bits(std::string code)`: Read all raw bits from the digits in `code`, and return them in a vector. If an invalid digit or other problem is encountered, **aborts the program!** This is meant for use only in static initializers with *hardcoded, known-valid* passcodes!

## Rounding

There are four round modes available.

- `RoundMode::PAD_TO_DIGIT`: Zero bits will be added until a whole number of *digits* is present.
- `RoundMode::PAD_TO_GROUP`: Zero bits will be added until a whole number of *groups* is present.
- `RoundMode::CHECK_TO_DIGIT`: Check bits will be added until a whole number of *digits* is present.
- `RoundMode::CHECK_TO_GROUP`: Check bits will be added until a whole number of *groups* is present. This is the default, and you should probably always use it.

## Check Bits

As Passende processes a code, it keeps a running 32-bit CRC of the data bits it has encoded so far. When a check bit is needed, it comes from this CRC, starting with the least significant bit. If a data bit is encoded after a check bit, the "current bit" of the CRC is reset, and the next check bit would come from the least significant bit of the running CRC at that point.

How many check bits you need is up to you. *n* check bits means roughly *1/2<sup>n</n>* mistakes will go uncaught. In my opinion, five check bits (one extra digit) is the minimum you should strive for; this would catch about 97% of mistakes. Doubling this to ten check bits catches about 99.9% of mistakes.

# License

Passende is copyright ©2020 Solra Bizna.

The Passende library is distributed under the zlib license. This puts very few restrictions on use. See [`LICENSE.md`](LICENSE.md) for the complete, very short text of the license.

The Passende coding scheme is, in my opinion, *far* too simple to patent, copyright, or otherwise own or restrict.

#pragma once
#pragma unmanaged

#include <type_traits>
#include <cstdint>

using __uint64 = unsigned __int64;

template<class> struct is_bounded_char_array : std::false_type {};

template<class> struct is_bounded_wide_char_array : std::false_type {};

template<size_t N>
struct is_bounded_char_array<char[N]> : std::true_type {};

template<size_t N>
struct is_bounded_wide_char_array<wchar_t[N]> : std::true_type {};

template<class T>
constexpr bool is_binary_digit = std::_Is_any_of_v<std::remove_cv_t<T>,
	signed char, unsigned char, short, unsigned short, int, unsigned int, long, unsigned long,
	const signed char, const unsigned char, const short, const unsigned short, const int, const unsigned int, const long, const unsigned long,
	int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
	const int8_t, const uint8_t, const int16_t, const uint16_t, const int32_t, const uint32_t,
	long long, __int64, int64_t, const long long, const __int64, const int64_t,
	unsigned long long, unsigned __int64, uint64_t, const unsigned long long, const unsigned __int64, const uint64_t>;

template<class T>
constexpr bool is_floating_point = std::_Is_any_of_v <std::remove_cv_t<T>,
	float, double, const float, const double>;

template<class T>
constexpr bool is_single_byte_character = std::_Is_any_of_v<std::remove_cv_t<T>, char, const char>;

template<class T>
constexpr bool is_double_byte_character = std::_Is_any_of_v<std::remove_cv_t<T>, wchar_t, const wchar_t>;

template<class T>
constexpr bool is_character = is_single_byte_character<T> || is_double_byte_character<T>;

template<class T>
constexpr bool is_single_byte_character_array = std::_Is_any_of_v<std::remove_cv_t<T>, char*, const char*> || is_bounded_char_array<T>{};

template<class T>
constexpr bool is_double_byte_character_array = std::_Is_any_of_v<std::remove_cv_t<T>, wchar_t*, const wchar_t*> || is_bounded_wide_char_array<T>{};

template<class T>
constexpr bool is_character_array = is_single_byte_character_array<T> || is_double_byte_character_array<T>;

template<class T>
concept is_addition_supported = is_binary_digit<T> || is_character<T> || is_character_array<T> || is_floating_point<T>;
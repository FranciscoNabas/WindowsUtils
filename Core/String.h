#pragma once
#pragma unmanaged

#include "MemoryManagement.h"

///////////////////////////////////////////////////////////////////////////
//
//  ~ WindowsUtils base string.
//
// ------------------------------------------------------------------------

//  Simple template string.
//  This implementation is based on C++ std::base_string, and
//  .NET System.String.
//
//  This is probably not a good string implementation nor does
//  it have a reason to exist.
//
// This class serves as a base template for the 8-bit, 16-bit, and 32-bit
// types that should be used throughout the code.

// ------------------------------------------------------------------------
//
//  Copyright (c) Francisco Nabas 2023.
//
//  This code, and the WindowsUtils module are distributed under
//  the MIT license. (are you sure you want to use this?)
//
///////////////////////////////////////////////////////////////////////////

// These base structs were added so we can specialize other functionalities.

template <class _char_type>
struct _WChar_traits_ex : std::_WChar_traits<_char_type>
{
    _NODISCARD static inline int comparenocase(_In_reads_(count) const _char_type* const left,
        _In_reads_(count) const _char_type* const right, const size_t count) noexcept {
        return _wcsnicmp(reinterpret_cast<const wchar_t*>(left), reinterpret_cast<const wchar_t*>(right), count);
    }

    _NODISCARD static inline _char_type* format(WuAllocator* allocator, const _char_type* format, va_list args) {
        if (format != NULL) {
            size_t char_count = _vscwprintf(format, args) + 1;
            _char_type* buffer = static_cast<_char_type*>(allocator->allocate(char_count * sizeof(_char_type)));
            vswprintf_s(buffer, char_count, format, args);

            return buffer;
        }

        return NULL;
    }

    _NODISCARD static inline const _char_type* findstr(const _char_type* first, const _char_type* second) {
        return wcsstr(reinterpret_cast<const wchar_t*>(first), reinterpret_cast<const wchar_t*>(second));
    }
};

template <class _char_type, class _int_type>
struct _Narrow_char_traits_ex : std::_Narrow_char_traits<_char_type, _int_type>
{
    _NODISCARD static inline int comparenocase(_In_reads_(count) const _char_type* const left,
        _In_reads_(count) const _char_type* const right, const size_t count) noexcept {
        return _strnicmp(left, right, count);
    }

    _NODISCARD static inline _char_type* format(WuAllocator* allocator, const _char_type* format, va_list args) {
        if (format != NULL) {

            size_t char_count = _vscprintf(format, args) + 1;
            _char_type* buffer = static_cast<_char_type*>(allocator->allocate(char_count * sizeof(_char_type)));
            vsprintf_s(buffer, char_count, format, args);

            return buffer;
        }

        return NULL;
    }

    _NODISCARD static inline const _char_type* findstr(const _char_type* first, const _char_type* second) {
        return strstr(reinterpret_cast<const char*>(first), reinterpret_cast<const char*>(second));
    }
};

template <class _char_type>
struct char_traits : std::_Char_traits<_char_type, long> {};

template <>
struct char_traits<wchar_t> : _WChar_traits_ex<wchar_t> {};

template <>
struct char_traits<char> : _Narrow_char_traits_ex<char, int> {};

template <>
struct char_traits<char16_t> : _WChar_traits_ex<char16_t> {};

template <>
struct char_traits<char32_t> : std::_Char_traits<char32_t, unsigned int> {};

static constexpr auto npos{ static_cast<size_t>(-1) };

// Base string class.

template <class _char_type, class _traits = char_traits<_char_type>>
class WuBaseString
{
private:
    _char_type* _buffer;
    size_t _char_count;
    WuAllocator* _allocator;

public:

    WuBaseString() {
        _allocator = new WuAllocator();
        _char_count = 0;
        _buffer = static_cast<_char_type*>(_allocator->allocate(sizeof(_char_type)));
    }

    WuBaseString(const _char_type* ptr) {
        if (ptr == NULL)
        {
            _allocator = new WuAllocator();
            _char_count = 0;
            _buffer = static_cast<_char_type*>(_allocator->allocate(sizeof(_char_type)));
        }

        else {
            _allocator = new WuAllocator();
            _char_count = _traits::length(ptr);

            size_t buffer_size = (_char_count + 1) * sizeof(_char_type);
            _buffer = static_cast<_char_type*>(_allocator->allocate(buffer_size));

            _traits::copy(_buffer, ptr, _char_count);
        }
    }
    WuBaseString(const WuBaseString& other) {
        if (other._buffer == NULL)
        {
            _allocator = new WuAllocator();
            _char_count = 0;
            _buffer = static_cast<_char_type*>(_allocator->allocate(sizeof(_char_type)));
        }

        else {
            _allocator = new WuAllocator();
            _char_count = other._char_count;

            size_t buffer_size = (_char_count + 1) * sizeof(_char_type);
            _buffer = static_cast<_char_type*>(_allocator->allocate(buffer_size));

            _traits::copy(_buffer, other._buffer, _char_count);
        }
    }
    WuBaseString(WuBaseString&& other) noexcept
        : WuBaseString() {
        Swap(*this, other);
    }

    ~WuBaseString() {
        if (_buffer != NULL)
            _allocator->deallocate(static_cast<void*>(_buffer));

        _buffer = NULL;
        _char_count = 0;
        delete _allocator;
    }

    friend void Swap(WuBaseString& first, WuBaseString& second) {
        using std::swap;

        swap(first._char_count, second._char_count);
        swap(first._allocator, second._allocator);
        swap(first._buffer, second._buffer);
    }

    inline void SecureErase(bool deallocate = true) {
        if (_buffer != NULL) {
            size_t char_count = _traits::length(_buffer);
            if (char_count > 0) {
                RtlSecureZeroMemory(_buffer, char_count * sizeof(_char_type));
            }
            if (deallocate) {
                _allocator->deallocate(_buffer);
                _buffer = NULL;
                _char_count = 0;
            }
        }
    }

    inline const size_t Length() {
        size_t len = 0;
        if (_buffer != 0) {
            _char_count = _traits::length(_buffer);
            len = _char_count;
        }

        return len;
    }
    inline const size_t Length() const {
        size_t len = 0;
        if (_buffer != 0) {
            len = _traits::length(_buffer);
        }

        return len;
    }

    inline static bool IsNullOrEmpty(const _char_type* ptr) {
        if (ptr != NULL)
            return _traits::length(ptr) == 0;

        return true;
    }

    inline static bool IsNullOrEmpty(const WuBaseString& str) {
        return str.Length() == 0;
    }

    inline static bool IsNullOrWhiteSpace(const _char_type* ptr) {
        if (ptr == NULL)
            return true;

        size_t char_count = _traits::length(ptr);
        for (size_t i = 0; i < char_count; i++) {
            if (ptr[i] != ' ')
                return false;
        }

        return true;
    }

    inline static bool IsNullOrWhiteSpace(const WuBaseString& str) {
        for (size_t i = 0; i < str.Length(); i++) {
            if (str._buffer[i] != ' ')
                return false;
        }

        return true;
    }

    _NODISCARD inline _char_type* GetBuffer() {
        return _buffer;
    }

    _NODISCARD inline const _char_type* const GetBuffer() const {
        return _buffer;
    }

    void Format(const _char_type* format, ...) {
        va_list args;
        va_start(args, format);

        _buffer = _traits::format(_allocator, format, args);
        _char_count = _traits::length(_buffer);
    }

    void Format(const WuBaseString& format, ...) {
        if (!IsNullOrEmpty(format)) {
            va_list args;
            va_start(args, format);

            _buffer = _traits::format(_allocator, format._buffer, args);
            _char_count = _traits::length(_buffer);
        }
    }

    void Remove(const size_t index, const size_t count) {
        if (count > _char_count)
            throw "Count can't be greater than the string length.";

        if (index < 0 || index + count > _char_count - 1)
            throw "Index outside of string boundaries.";

        size_t new_count = _char_count - count + 1;
        _char_type* new_buffer = static_cast<_char_type*>(_allocator->allocate(new_count * sizeof(_char_type)));

        size_t last_index = 0;
        for (size_t i = 0; i < _char_count; i++) {
            if (i < index || i >= index + count) {
                new_buffer[last_index] = _buffer[i];
                last_index++;
            }
        }

        _allocator->deallocate(_buffer);
        _buffer = new_buffer;
        _char_count = new_count - 1;
    }

    void Remove(const size_t index) {
        if (index < 0 || index + 1 > _char_count - 1)
            throw "Index outside of string boundaries.";

        // +1 because of '\0'.
        size_t new_count = _char_count;
        _char_type* new_buffer = static_cast<_char_type*>(_allocator->allocate(new_count * sizeof(_char_type)));

        size_t last_index = 0;
        for (size_t i = 0; i < _char_count; i++) {
            if (i < index || i >= index + 1) {
                new_buffer[last_index] = _buffer[i];
                last_index++;
            }
        }

        _allocator->deallocate(_buffer);
        _buffer = new_buffer;
        _char_count = new_count - 1;
    }

    inline bool Contains(const _char_type tchar) const {
        if (_traits::find(_buffer, _char_count, tchar) != NULL)
            return true;

        return false;
    }

    inline bool Contains(const _char_type* ptr) const {
        if (ptr == NULL)
            throw "Input string cannot be null.";

        const _char_type* find = _traits::findstr(_buffer, ptr);
        if (find != NULL && find != _buffer)
            return true;

        return false;
    }

    inline bool Contains(const WuBaseString& str) const {
        const _char_type* find = _traits::findstr(_buffer, str._buffer);
        if (find != NULL && find != _buffer)
            return true;

        return false;
    }

    inline bool EndsWith(const _char_type tchar) const {
        // 0-based array plus /0.
        if (_buffer[_char_count - 2] == tchar)
            return true;

        return false;
    }

    inline bool EndsWith(const _char_type* ptr, bool ignore_case = false) const {
        if (ptr == NULL)
            throw "Input string cannot be null.";

        size_t input_len = _traits::length(ptr);
        if (input_len > this->Length())
            throw "Input string cannot be bigger than the original.";

        if (ignore_case)
            return _traits::comparenocase(_buffer + this->Length() - input_len, ptr, input_len) == 0;

        return _traits::compare(_buffer + this->Length() - input_len, ptr, input_len) == 0;
    }

    inline bool EndsWith(const WuBaseString& str, bool ignore_case = false) const {
        size_t input_len = str.Length();
        if (str.Length() > this->Length())
            throw "Input string cannot be bigger than the original.";

        if (ignore_case)
            return _traits::comparenocase(_buffer + this->Length() - input_len, str._buffer, input_len) == 0;

        return _traits::compare(_buffer + this->Length() - input_len, str._buffer, input_len) == 0;
    }

    inline _char_type operator[](const size_t index) {
        if (index < 0 && index > _char_count)
            throw "Index outside the boundaries of this string.";

        return _buffer[index];
    }

    friend WuBaseString operator+(const WuBaseString& left, const _char_type* right) {
        if (right == NULL)
            return WuBaseString(left);

        WuBaseString result;
        size_t left_len = left.Length();
        size_t right_len = _traits::length(right);
        size_t in_len = left_len + right_len + 1;

        result._buffer = static_cast<_char_type*>(result._allocator->allocate(in_len * sizeof(_char_type)));

        _traits::copy(result._buffer, left._buffer, left_len);
        _traits::copy(result._buffer + left_len, right, right_len);
        result._char_count = in_len - 1;

        return result;
    }

    friend WuBaseString operator+(const WuBaseString& left, const WuBaseString& right) {
        WuBaseString result;
        size_t left_len = left.Length();
        size_t right_len = right.Length();
        size_t in_len = left_len + right_len + 1;

        result._buffer = static_cast<_char_type*>(result._allocator->allocate(in_len * sizeof(_char_type)));

        _traits::copy(result._buffer, left._buffer, left_len);
        _traits::copy(result._buffer + left_len, right._buffer, right_len);
        result._char_count = in_len - 1;

        return result;
    }

    inline WuBaseString& operator=(const _char_type* other) {
        WuBaseString otherStr(other);
        Swap(*this, otherStr);

        return *this;
    }

    inline WuBaseString& operator=(WuBaseString other) {
        Swap(*this, other);

        return *this;
    }

    inline WuBaseString& operator+=(const _char_type* other) {
        *this = *this + other;

        return *this;
    }

    inline WuBaseString& operator+=(const WuBaseString& other) {
        *this = *this + other;

        return *this;
    }

    inline bool operator==(const _char_type* right) const {
        if (right == NULL) {
            return false;
        }
        else {
            size_t right_len = _traits::length(right);
            size_t biggest = (((this->Length()) > (right_len)) ? (this->Length()) : (right_len));
            return _traits::compare(_buffer, right, biggest) == 0;
        }
    }

    inline bool operator==(const WuBaseString& right) const {
        size_t biggest = (((this->Length()) > (right.Length())) ? (this->Length()) : (right.Length()));
        return _traits::compare(_buffer, right._buffer, biggest) == 0;
    }
    inline bool operator!=(const _char_type* right) const {
        return !(*this == right);
    }
    inline bool operator!=(const WuBaseString& right) const {
        return !(*this == right);
    }

    inline bool operator<(const _char_type* right) const {
        if (right == NULL) {
            return false;
        }
        else {
            size_t right_len = _traits::length(right);
            size_t biggest = (((this->Length()) > (right_len)) ? (this->Length()) : (right_len));
            return _traits::compare(_buffer, right, biggest) < 0;
        }
    }

    inline bool operator<(const WuBaseString& right) const {
        size_t biggest = (((this->Length()) > (right.Length())) ? (this->Length()) : (right.Length()));
        return _traits::compare(_buffer, right._buffer, biggest) < 0;
    }

    inline bool operator>(const _char_type* right) const {
        return !(*this < right);
    }

    inline bool operator>(const WuBaseString& right) const {
        return !(*this < right);
    }

    inline bool operator<=(const _char_type* right) const {
        if (right == NULL) {
            return false;
        }
        else {
            size_t right_len = _traits::length(right);
            size_t biggest = (((this->Length()) > (right_len)) ? (this->Length()) : (right_len));
            return _traits::compare(_buffer, right, biggest) <= 0;
        }
    }

    inline bool operator<=(const WuBaseString& right) const {
        size_t biggest = (((this->Length()) > (right.Length())) ? (this->Length()) : (right.Length()));
        return _traits::compare(_buffer, right._buffer, biggest) <= 0;
    }

    inline bool operator>=(const _char_type* right) const {
        if (right == NULL) {
            return false;
        }
        else {
            size_t right_len = _traits::length(right);
            size_t biggest = (((this->Length()) > (right_len)) ? (this->Length()) : (right_len));
            return _traits::compare(_buffer, right, biggest) >= 0;
        }
    }

    inline bool operator>=(const WuBaseString& right) const {
        size_t biggest = (((this->Length()) > (right.Length())) ? (this->Length()) : (right.Length()));
        return _traits::compare(_buffer, right._buffer, biggest) >= 0;
    }
};

using WWuString = WuBaseString<wchar_t, char_traits<wchar_t>>;
using WuString = WuBaseString<char, char_traits<char>>;
using u16WuString = WuBaseString<char16_t, char_traits<char16_t>>;
using u32WuString = WuBaseString<char32_t, char_traits<char32_t>>;

#if defined(__cpp_lib_char8_t)
using u8WuString = WuBaseString<char8_t, char_traits<char8_t>>;
#endif
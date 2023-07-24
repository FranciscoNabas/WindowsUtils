#pragma once
#pragma unmanaged

#include "pch.h"

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
//  Copyright (c) Francisco Nabas.
//
//  This code, and the WindowsUtils module are distributed under
//  the MIT license. (are you sure you want to use this?)
//
//  There is no warranty that this is a stable, optimized, and safe code.
//
///////////////////////////////////////////////////////////////////////////


// These base structs were added so we can make case insensitive comparison.

template <class _char_type>
struct _WChar_traits_ext : std::_WChar_traits<_char_type>
{
    _NODISCARD static inline int icompare(_In_reads_(count) const _char_type* const left,
        _In_reads_(_count) const _char_type* const right, const size_t count) noexcept {
        return _wcsnicmp(reinterpret_cast<const wchar_t*>(left), reinterpret_cast<const wchar_t*>(right), count);
    }
};

template <class _char_type, class _int_type>
struct _Narrow_char_traits_ex : std::_Narrow_char_traits<_char_type, _int_type>
{
    _NODISCARD static inline int icompare(_In_reads_(count) const _char_type* const left,
        _In_reads_(_count) const _char_type* const right, const size_t count) noexcept {
        return _strnicmp(left, right, count);
    }
};

template <class _char_type>
struct char_traits : std::_Char_traits<_char_type, long> {};

template <>
struct char_traits<wchar_t> : _WChar_traits_ext<wchar_t> {};

template <>
struct char_traits<char> : _Narrow_char_traits_ex<char, int> {};

template <>
struct char_traits<char16_t> : _WChar_traits_ext<char16_t> {};

template <>
struct char_traits<char32_t> : std::_Char_traits<char32_t, unsigned int> {};

// Base string class.

template <class _char_type, class _traits = char_traits<_char_type>>
class WuBaseString
{
private:
    _char_type* _buffer;
    size_t _char_count;
    WuAllocator* _allocator;
    bool _is_initialized;

public:
    //////////////////////////////////////////////////////////////////
    //
    //  ~ Constructors
    //
    //////////////////////////////////////////////////////////////////

    // Default constructor will construct the allocator, and default
    // the buffer and char count to zero;
    WuBaseString();

    // Construct from a raw pointer. This will construct the allocator,
    // and allocate memory for the buffer based on the ptr char count.
    WuBaseString(const _char_type* ptr);

    // Construct from another string.
    WuBaseString(const WuBaseString& other);

    // Destructor.
    ~WuBaseString();

    //////////////////////////////////////////////////////////////////
    //
    //  ~ Methods
    //
    //////////////////////////////////////////////////////////////////

    // Allocates the input number of characters to the buffer.
    inline void Initialize(const size_t char_count);

    // Fills the buffer with zeros, and deallocate the
    // buffer, if 'deallocate' = true;
    inline void SecureErase(bool deallocate = true);

    // Returns the string length without '\0'.
    // Updates the char count every time.
    inline const size_t Length();
    inline const size_t Length() const;

    // Check if a string is null, empty or consists only of
    // white spaces.
    inline static bool IsNullOrEmpty(const _char_type* ptr);
    inline static bool IsNullOrEmpty(const WuBaseString& str);
    inline static bool IsNullOrWhiteSpace(const _char_type* ptr);
    inline static bool IsNullOrWhiteSpace(const WuBaseString& str);

    // Returns the string raw pointer. Equivalent to the
    // std::string::c_str();
    _NODISCARD inline const _char_type* GetBuffer();
    _NODISCARD inline const _char_type* const GetBuffer() const;

    // Writes formatted data to a string.
    void Format(const _char_type* format, ...);
    void Format(const WuBaseString& format, ...);

    // Removes one or more characters from the string, starting
    // at the specified index.
    void Remove(const size_t index, const size_t count);
    void Remove(const size_t index);

    // Checks if a string contains a char, or other string.
    inline bool Contains(const _char_type tchar) const;
    inline bool Contains(const _char_type* ptr) const;
    inline bool Contains(const WuBaseString& str) const;

    // Checks if a string ends with a specified char, or string.
    inline bool EndsWith(const _char_type tchar) const;
    inline bool EndsWith(const _char_type* ptr, bool ignore_case = false) const;
    inline bool EndsWith(const WuBaseString& str, bool ignore_case = false) const;

    //////////////////////////////////////////////////////////////////
    //
    //  ~ Operators
    //
    //////////////////////////////////////////////////////////////////

    // Indexing operator.
    inline _char_type operator[](const size_t index);

    // Assignment operator.
    inline void operator=(const _char_type* other);
    inline void operator=(const WuBaseString& other);

    // Compound assignment addition operator.
    inline void operator+=(const _char_type* other);
    inline void operator+=(const WuBaseString& other);

    // Equality operators.
    inline bool operator==(const _char_type* right);
    inline bool operator==(const WuBaseString& right);
    inline bool operator!=(const _char_type* right);
    inline bool operator!=(const WuBaseString& right);

    // Size operators.
    inline bool operator<(const _char_type* right);
    inline bool operator<(const WuBaseString& right);
    inline bool operator>(const _char_type* right);
    inline bool operator>(const WuBaseString& right);
    inline bool operator<=(const _char_type* right);
    inline bool operator<=(const WuBaseString& right);
    inline bool operator>=(const _char_type* right);
    inline bool operator>=(const WuBaseString& right);

private:
    inline void InitializeToZero();
};

//////////////////////////////////////////////////////////////
//
//  ~ 8-bit, 16-bit, and 32-bit types
//
//////////////////////////////////////////////////////////////

using WWuString = WuBaseString<wchar_t, char_traits<wchar_t>>;
using WuString = WuBaseString<char, char_traits<char>>;
using u16WuString = WuBaseString<char16_t, char_traits<char16_t>>;
using u32WuString = WuBaseString<char32_t, char_traits<char32_t>>;

#if defined(__cpp_lib_char8_t)
using u8WuString = WuBaseString<char8_t, char_traits<char8_t>>;
#endif

//////////////////////////////////////////////////////////////
//
//  ~ Constructor implementation
//
//////////////////////////////////////////////////////////////

template <class _char_type, class _traits>
WuBaseString<_char_type, _traits>::WuBaseString() {
    this->InitializeToZero();
}

template <class _char_type, class _traits>
WuBaseString<_char_type, _traits>::WuBaseString(const _char_type* ptr) {
    if (ptr == NULL) {
        this->InitializeToZero();
    }
    else {
        _is_initialized = false;

        _allocator = new WuAllocator();
        _char_count = _traits::length(ptr);

        size_t buffer_size = (_char_count + 1) * sizeof(_char_type);
        _buffer = static_cast<_char_type*>(_allocator->allocate(buffer_size));
        _is_initialized = true;

        _traits::copy(_buffer, ptr, _char_count);
    }
}

template <class _char_type, class _traits>
WuBaseString<_char_type, _traits>::WuBaseString(const WuBaseString& other) {
    if (other._buffer == NULL) {
        this->InitializeToZero();
    }
    else {
        _is_initialized = false;

        _allocator = new WuAllocator();
        _char_count = other._char_count;

        size_t buffer_size = (_char_count + 1) * sizeof(_char_type);
        _buffer = static_cast<_char_type*>(_allocator->allocate(buffer_size));
        _is_initialized = true;

        _traits::copy(_buffer, other._buffer, _char_count);
    }
}

template <class _char_type, class _traits>
WuBaseString<_char_type, _traits>::~WuBaseString() {
    if (_is_initialized && _buffer != NULL) {
        _allocator->deallocate(static_cast<void*>(_buffer));
    }
    _buffer = NULL;
    _char_count = 0;
    _is_initialized = false;

    delete _allocator;
}

//////////////////////////////////////////////////////////////
//
//  ~ Method implementation
//
//////////////////////////////////////////////////////////////

template <class _char_type, class _traits>
inline void WuBaseString<_char_type, _traits>::InitializeToZero() {
    _buffer = NULL;
    _char_count = 0;
    _is_initialized = false;
    _allocator = new WuAllocator();
}

template <class _char_type, class _traits>
inline void WuBaseString<_char_type, _traits>::Initialize(const size_t char_count) {
    _is_initialized = false;
    _char_count = 0;

    size_t buffer_size = (char_count + 1) * sizeof(_char_type);
    _buffer = static_cast<_char_type*>(_allocator->allocate(buffer_size));
    _is_initialized = true;
}

template <class _char_type, class _traits>
inline void WuBaseString<_char_type, _traits>::SecureErase(bool deallocate) {
    if (_is_initialized && _buffer != NULL) {
        size_t char_count = _traits::length(_buffer);
        if (char_count > 0) {
            RtlSecureZeroMemory(_buffer, char_count * sizeof(_char_type));
        }
        if (deallocate) {
            _allocator->deallocate(_buffer);
            _is_initialized = false;
            _buffer = NULL;
            _char_count = 0;
        }
    }
}

template <class _char_type, class _traits>
inline const size_t WuBaseString<_char_type, _traits>::Length() {
    size_t len = 0;
    if (_is_initialized && _buffer != 0) {
        _char_count = _traits::length(_buffer);
        len = _char_count;
    }

    return len;
}

template <class _char_type, class _traits>
inline const size_t WuBaseString<_char_type, _traits>::Length() const {
    size_t len = 0;
    if (_is_initialized && _buffer != 0) {
        _char_count = _traits::length(_buffer);
        len = _char_count;
    }

    return len;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::IsNullOrEmpty(const _char_type* ptr) {
    if (ptr != NULL)
        return _traits::length(ptr) == 0;

    return true;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::IsNullOrEmpty(const WuBaseString& str) {
    if (str._is_initialized && str._buffer != NULL)
        return str.Length() == 0;

    return true;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::IsNullOrWhiteSpace(const _char_type* ptr) {
    if (ptr == NULL)
        return TRUE;

    size_t char_count = _traits::length(ptr);
    for (size_t i = 0; i < char_count; i++) {
        if (ptr[i] != ' ')
            return FALSE;
    }

    return TRUE;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::IsNullOrWhiteSpace(const WuBaseString& str) {
    if (!str._is_initialized || str._buffer == NULL)
        return true;

    for (size_t i = 0; i < str.Length(); i++) {
        if (str._buffer[i] != ' ')
            return FALSE;
    }

    return TRUE;
}

template <class _char_type, class _traits>
_NODISCARD inline const _char_type* WuBaseString<_char_type, _traits>::GetBuffer() {
    return _buffer;
}

template <class _char_type, class _traits>
_NODISCARD inline const _char_type* const WuBaseString<_char_type, _traits>::GetBuffer() const {
    return _buffer;
}

template <class _char_type, class _traits>
void WuBaseString<_char_type, _traits>::Format(const _char_type* format, ...) {
    if (format != NULL) {
        va_list args;
        va_start(args, format);

        _char_count = _vscprintf(format, args) + 1;
        if (_is_initialized && _buffer != NULL)
            _allocator->deallocate(_buffer);

        _buffer = static_cast<_char_type*>(_allocator->allocate(_char_count * sizeof(_char_type)));
        vsprintf_s(_buffer, _char_count, format, args);

        _is_initialized = true;
    }
}

template <class _char_type, class _traits>
void WuBaseString<_char_type, _traits>::Format(const WuBaseString& format, ...) {
    if (format._is_initialized && format._buffer != NULL) {
        va_list args;
        va_start(args, format._buffer);

        _char_count = _vscprintf(format._buffer, args) + 1;
        if (_is_initialized && _buffer != NULL)
            _allocator->deallocate(_buffer);

        _buffer = static_cast<_char_type*>(_allocator->allocate(_char_count * sizeof(_char_type)));
        vsprintf_s(_buffer, _char_count, format._buffer, args);

        _is_initialized = true;
    }
}

template <class _char_type, class _traits>
void WuBaseString<_char_type, _traits>::Remove(const size_t index, const size_t count) {
    if (_is_initialized && _buffer != NULL) {
        if (count > _char_count)
            throw "Count can't be greater than the string length.";

        if (index < 0 || index + count > _char_count - 2)
            throw "Index outside of string boundaries.";

        size_t new_count = _char_count - count + 1;
        _allocator->deallocate(_buffer);
        _char_type* new_buffer = static_cast<_char_type*>(_allocator->allocate(new_count * sizeof(_char_type)));

        size_t last_index = 0;
        for (size_t i = 0; i < _char_count - 1; i++) {
            if (i < index || i > index + count) {
                new_buffer[last_index] = _buffer[i];
                last_index++;
            }
        }

        _buffer = new_buffer;
        _char_count = new_count - 1;
    }
}

template <class _char_type, class _traits>
void WuBaseString<_char_type, _traits>::Remove(const size_t index) {
    if (_is_initialized && _buffer != NULL) {
        size_t count = 1;
        if (count > _char_count)
            throw "Count can't be greater than the string length.";

        if (index < 0 || index + count > _char_count - 2)
            throw "Index outside of string boundaries.";

        size_t new_count = _char_count - count + 1;
        _allocator->deallocate(_buffer);
        _char_type* new_buffer = static_cast<_char_type*>(_allocator->allocate(new_count * sizeof(_char_type)));

        size_t last_index = 0;
        for (size_t i = 0; i < _char_count - 1; i++) {
            if (i < index || i > index + count) {
                new_buffer[last_index] = _buffer[i];
                last_index++;
            }
        }

        _buffer = new_buffer;
        _char_count = new_count - 1;
    }
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::Contains(const _char_type tchar) const {
    if (!_is_initialized || _buffer == NULL)
        return false;

    if (_traits::find(_buffer, 1, &tchar) != NULL)
        return true;

    return false;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::Contains(const _char_type* ptr) const {
    if (!_is_initialized || _buffer == NULL)
        return false;

    if (ptr == NULL)
        throw "Input string cannot be null.";

    size_t char_count = _traits::length(ptr);
    if (_traits::find(_buffer, char_count, ptr) != NULL)
        return true;

    return false;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::Contains(const WuBaseString& str) const {
    if (!_is_initialized || _buffer == NULL)
        return false;

    if (!str._is_initialized || str._buffer == NULL)
        throw "Input string is not initialized.";

    if (_traits::find(_buffer, str.Length(), str > _buffer) != NULL)
        return true;

    return false;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::EndsWith(const _char_type tchar) const {
    if (!_is_initialized || _buffer == NULL)
        return false;

    // 0-based array plus /0.
    if (_buffer[_char_count - 2] == tchar)
        return TRUE;

    return FALSE;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::EndsWith(const _char_type* ptr, bool ignore_case) const {
    if (!_is_initialized || _buffer == NULL)
        return false;

    if (ptr == NULL)
        throw "Input string cannot be null.";

    size_t input_len = _traits::length(ptr);
    if (input_len > this->Length())
        throw "Input string cannot be bigger than the original.";

    if (ignore_case)
        return _traits::icompare(_buffer + this->Length() - input_len, ptr, input_len) == 0;

    return _traits::compare(_buffer + this->Length() - input_len, ptr, input_len) == 0;
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::EndsWith(const WuBaseString& str, bool ignore_case) const {
    if (!_is_initialized || _buffer == NULL)
        return false;

    if (!str._is_initialized || str._buffer == NULL)
        throw "Input string is not initialized.";

    size_t input_len = str.Length();
    if (str.Length() > this->Length())
        throw "Input string cannot be bigger than the original.";

    if (ignore_case)
        return _traits::icompare(_buffer + this->Length() - input_len, str._buffer, input_len) == 0;

    return _traits::compare(_buffer + this->Length() - input_len, str._buffer, input_len) == 0;
}

//////////////////////////////////////////////////////////////
//
//  ~ Operator implementation
//
//////////////////////////////////////////////////////////////

template <class _char_type, class _traits>
inline _char_type WuBaseString<_char_type, _traits>::operator[](const size_t index) {
    if (!_is_initialized || _buffer = NULL)
        throw "String is not initialized!";

    if (index < 0 && index > _char_count)
        throw "Index outside the boundaries of this string.";

    return _buffer[index];
}

template <class _char_type, class _traits>
inline void WuBaseString<_char_type, _traits>::operator=(const _char_type* other) {
    if (other == NULL)
        throw "Input string cannot be NULL.";

    size_t in_len = _traits::length(other) + 1;
    if (_is_initialized && _buffer != NULL) {
        _allocator->deallocate(_buffer);
    }

    _allocator->allocate(in_len * sizeof(_char_type));
    _is_initialized = true;

    in_len--;
    _traits::copy(_buffer, other, in_len);
    _char_count = in_len - 1;

    _traits::assign(_buffer[in_len], _char_type());
}

template <class _char_type, class _traits>
inline void WuBaseString<_char_type, _traits>::operator=(const WuBaseString& other) {
    if (!other._is_initialized || other._buffer == NULL)
        throw "Input string is not initialized.";

    size_t in_len = other.Length() + 1;
    if (_is_initialized && _buffer != NULL) {
        _allocator->deallocate(_buffer);
    }

    _allocator->allocate(in_len * sizeof(_char_type));
    _is_initialized = true;

    in_len--;
    _traits::copy(_buffer, other._buffer, in_len);
    _char_count = in_len - 1;

    _traits::assign(_buffer[in_len], _char_type());
}

template <class _char_type, class _traits>
inline void WuBaseString<_char_type, _traits>::operator+=(const _char_type* other) {
    if (other != NULL) {
        if (!_is_initialized || _buffer == NULL) {
            size_t total_len = _traits::length(other) + 1;
            _buffer = static_cast<_char_type*>(_allocator->allocate(total_len * sizeof(_char_type)));
            _is_initialized = true;

            total_len--;
            _traits::copy(_buffer, other, total_len);
            _char_count = total_len;

            _traits::assign(_buffer[total_len], _char_type());
        }
        else {
            size_t total_len = _traits::length(other) + this->Length() + 1;
            _char_type* new_ptr = static_cast<_char_type*>(_allocator->allocate(total_len * sizeof(_char_type)));
            _is_initialized = true;

            total_len--;
            _traits::copy(new_ptr, _buffer, _char_count);
            _traits::copy(new_ptr + _char_count, other, total_len);
            _char_count = total_len;

            if (_buffer != NULL)
                _allocator->deallocate(_buffer);

            _traits::assign(new_ptr[total_len], _char_type());
            _buffer = new_ptr;
        }
    }
}

template <class _char_type, class _traits>
inline void WuBaseString<_char_type, _traits>::operator+=(const WuBaseString& other) {
    if (!other._is_initialized || other._buffer == NULL) {
        if (!_is_initialized || _buffer == NULL) {
            size_t total_len = other.Length() + 1;
            _buffer = static_cast<_char_type*>(_allocator->allocate(total_len * sizeof(_char_type)));
            _is_initialized = true;

            total_len--;
            _traits::copy(_buffer, other._buffer, total_len);
            _char_count = total_len;

            _traits::assign(_buffer[total_len], _char_type());
        }
        else {
            size_t total_len = other.Length() + this->Length() + 1;
            _char_type* new_ptr = static_cast<_char_type*>(_allocator->allocate(total_len * sizeof(_char_type)));
            _is_initialized = true;

            total_len--;
            _traits::copy(new_ptr, _buffer, _char_count);
            _traits::copy(new_ptr + _char_count, other._buffer, total_len);
            _char_count = total_len;

            if (_buffer != NULL)
                _allocator->deallocate(_buffer);

            _traits::assign(new_ptr[total_len], _char_type());
            _buffer = new_ptr;
        }
    }
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator==(const _char_type* right) {
    if (_buffer == NULL) {
        if (right == NULL) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        if (right == NULL) {
            return false;
        }
        else {
            size_t right_len = _traits::length(right);
            size_t biggest = (((this->Length()) < (right_len)) ? (this->Length()) : (right_len));
            return _traits::compare(_buffer, right, biggest) == 0;
        }
    }
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator==(const WuBaseString& right) {
    if (_buffer == NULL) {
        if (right._buffer == NULL) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        if (right._buffer == NULL) {
            return false;
        }
        else {
            size_t biggest = (((this->Length()) < (right.Length())) ? (this->Length()) : (right.Length()));
            return _traits::compare(_buffer, right._buffer, biggest) == 0;
        }
    }
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator!=(const _char_type* right) {
    return !(*this == right);
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator!=(const WuBaseString& right) {
    return !(*this == right);
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator<(const _char_type* right) {
    if (_buffer == NULL) {
        if (right == NULL) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        if (right == NULL) {
            return false;
        }
        else {
            size_t right_len = _traits::length(right);
            size_t biggest = (((this->Length()) < (right_len)) ? (this->Length()) : (right_len));
            return _traits::compare(_buffer, right, biggest) < 0;
        }
    }
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator<(const WuBaseString& right) {
    if (_buffer == NULL) {
        if (right._buffer == NULL) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        if (right._buffer == NULL) {
            return false;
        }
        else {
            size_t biggest = (((this->Length()) < (right.Length())) ? (this->Length()) : (right.Length()));
            return _traits::compare(_buffer, right._buffer, biggest) < 0;
        }
    }
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator>(const _char_type* right) {
    return !(*this < right);
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator>(const WuBaseString& right) {
    return !(*this < right);
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator<=(const _char_type* right) {
    if (_buffer == NULL) {
        return true;
    }
    else {
        if (right == NULL) {
            return false;
        }
        else {
            size_t right_len = _traits::length(right);
            size_t biggest = (((this->Length()) < (right_len)) ? (this->Length()) : (right_len));
            return _traits::compare(_buffer, right, biggest) <= 0;
        }
    }
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator<=(const WuBaseString& right) {
    if (_buffer == NULL) {
        return true;
    }
    else {
        if (right._buffer == NULL) {
            return false;
        }
        else {
            size_t biggest = (((this->Length()) < (right.Length())) ? (this->Length()) : (right.Length()));
            return _traits::compare(_buffer, right._buffer, biggest) <= 0;
        }
    }
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator>=(const _char_type* right) {
    return !(*this <= right);
}

template <class _char_type, class _traits>
inline bool WuBaseString<_char_type, _traits>::operator>=(const WuBaseString& right) {
    return !(*this <= right);
}
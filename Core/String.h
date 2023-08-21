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
struct _WChar_traits_ex : std::_WChar_traits<_char_type> {
    // Compares [left] and [right] ignoring case.
    _NODISCARD static inline int compare_no_case(_In_reads_(count) const _char_type* const left,
        _In_reads_(count) const _char_type* const right, const size_t count) noexcept {
        return _wcsnicmp(reinterpret_cast<const wchar_t*>(left), reinterpret_cast<const wchar_t*>(right), count);
    }

    // Writes formatted data to a buffer.
    _NODISCARD static inline _char_type* format(WuAllocator* allocator, const _char_type* format, va_list args) {
        if (format != NULL) {
            size_t char_count = _vscwprintf(format, args) + 1;
            _char_type* buffer = static_cast<_char_type*>(allocator->allocate(char_count * sizeof(_char_type)));
            vswprintf_s(buffer, char_count, format, args);

            return buffer;
        }

        return NULL;
    }

    // Returns the first occurrence of [second] in [first], or NULL of it doesn't find it.
    _NODISCARD static inline const _char_type* find_str(const _char_type* first, const _char_type* second) {
        return wcsstr(reinterpret_cast<const wchar_t*>(first), reinterpret_cast<const wchar_t*>(second));
    }
};

template <class _char_type, class _int_type>
struct _Narrow_char_traits_ex : std::_Narrow_char_traits<_char_type, _int_type> {
    // Compares [left] and [right] ignoring case.
    _NODISCARD static inline int compare_no_case(_In_reads_(count) const _char_type* const left,
        _In_reads_(count) const _char_type* const right, const size_t count) noexcept {
        return _strnicmp(left, right, count);
    }

    // Writes formatted data to a buffer.
    _NODISCARD static inline _char_type* format(WuAllocator* allocator, const _char_type* format, va_list args) {
        if (format != NULL) {

            size_t char_count = _vscprintf(format, args) + 1;
            _char_type* buffer = static_cast<_char_type*>(allocator->allocate(char_count * sizeof(_char_type)));
            vsprintf_s(buffer, char_count, format, args);

            return buffer;
        }

        return NULL;
    }

    // Returns the first occurrence of [second] in [first], or NULL of it doesn't find it.
    _NODISCARD static inline const _char_type* find_str(const _char_type* first, const _char_type* second) {
        return strstr(reinterpret_cast<const char*>(first), reinterpret_cast<const char*>(second));
    }
};

template <class _char_type>
struct char_traits : std::_Char_traits<_char_type, long> { };

template <>
struct char_traits<wchar_t> : _WChar_traits_ex<wchar_t> { };

template <>
struct char_traits<char> : _Narrow_char_traits_ex<char, int> { };

template <>
struct char_traits<char16_t> : _WChar_traits_ex<char16_t> { };

template <>
struct char_traits<char32_t> : std::_Char_traits<char32_t, unsigned int> { };

static constexpr auto npos { static_cast<size_t>(-1) };

// Base string class.

template <class _char_type, class _traits = char_traits<_char_type>>
class WuBaseString {
private:
    _char_type* _buffer;        // The C-style array of characters backing up the string.
    size_t _char_count;         // The '_char_type' count NOT including '\0'.
    WuAllocator* _allocator;    // The allocator used for allocating/deallocating memory.

public:

    // Default constructor. This will create an empty string.
    WuBaseString() {
        _allocator = new WuAllocator();
        _char_count = 0;
        _buffer = static_cast<_char_type*>(_allocator->allocate(sizeof(_char_type)));
    }

    // Creates a string from a raw pointer.
    WuBaseString(const _char_type* ptr) {
        if (ptr == NULL) {
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

    // Copy constructor. Creates a string from another.
    WuBaseString(const WuBaseString& other) {
        if (other._buffer == NULL) {
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

    // Move constructor. Moves the other string to this.
    WuBaseString(WuBaseString&& other) noexcept
        : WuBaseString() {
        Swap(*this, other);
    }

    // Destructor. Deallocates the buffer, and deletes the allocator.
    ~WuBaseString() {
        if (_buffer != NULL)
            _allocator->deallocate(static_cast<void*>(_buffer));

        _buffer = NULL;
        _char_count = 0;
        delete _allocator;
    }

    // Swap friend function for moving strings.
    friend void Swap(WuBaseString& first, WuBaseString& second) {
        using std::swap;

        swap(first._char_count, second._char_count);
        swap(first._allocator, second._allocator);
        swap(first._buffer, second._buffer);
    }

    // This function clears the contents from the buffer.
    // We do not deallocate because most likely this string
    // will be reused, so we just zero out.
    inline void Clear() {
        RtlZeroMemory(_buffer, _char_count * sizeof(_char_type));
        _char_count = 0;
    }

    // This function fills the buffer with zeroes, and deallocates it,
    // if deallocate = true. With deallocating only, there is no guarantee
    // the contents of the string are going to be immediately overwritten by
    // something else.
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

    // Returns the current string length. It calculates
    // it every time, because the buffer might have been
    // modified by the application.
    inline const size_t Length() {
        size_t len = 0;
        if (_buffer != 0) {
            _char_count = _traits::length(_buffer);
            len = _char_count;
        }

        return len;
    }

    // Const-qualified version of Length.
    inline const size_t Length() const {
        size_t len = 0;
        if (_buffer != 0) {
            len = _traits::length(_buffer);
        }

        return len;
    }

    // Returns true if a C-style string is null or empty.
    inline static bool IsNullOrEmpty(const _char_type* ptr) {
        if (ptr != NULL)
            return _traits::length(ptr) == 0;

        return true;
    }

    // Returns true if a string is null or empty.
    inline static bool IsNullOrEmpty(const WuBaseString& str) {
        return str.Length() == 0;
    }

    // Returns true if a C-style string is null, or composed only by white spaces.
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

    // Returns true if a string is null, or composed only by white spaces.
    inline static bool IsNullOrWhiteSpace(const WuBaseString& str) {
        for (size_t i = 0; i < str.Length(); i++) {
            if (str._buffer[i] != ' ')
                return false;
        }

        return true;
    }

    // Returns the _char_type raw pointer.
    _NODISCARD inline _char_type* GetBuffer() {
        return _buffer;
    }

    // Const-qualified version of GetBuffer.
    _NODISCARD inline const _char_type* const GetBuffer() const {
        return _buffer;
    }

    // Writes C-style formatted data to a string.
    _NODISCARD static WuBaseString Format(const _char_type* format, ...) {
        va_list args;
        va_start(args, format);

        WuAllocator* allocator = new WuAllocator();

        WuBaseString output;
        output._buffer = _traits::format(allocator, format, args);
        output._char_count = _traits::length(output._buffer);

        delete allocator;

        return output;
    }

    // Writes string formatted data to a string.
    _NODISCARD static WuBaseString Format(const WuBaseString& format, ...) {
        if (!IsNullOrEmpty(format)) {
            va_list args;
            va_start(args, format);

            WuAllocator* allocator = new WuAllocator();

            WuBaseString output;
            output._buffer = _traits::format(allocator, format._buffer, args);
            output._char_count = _traits::length(output._buffer);

            delete allocator;

            return output;
        }
    }

    // Removes [count] characters from the string, starting at [index].
    WuBaseString Remove(const size_t index, const size_t count) {
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

        WuBaseString output(new_buffer);
        return output;
    }

    // Removes one character from the string, starting at [index].
    WuBaseString Remove(const size_t index) {
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

        WuBaseString output(new_buffer);
        return output;
    }

    // Returns true if [t_char] is found in the string.
    inline bool Contains(const _char_type t_char) const {
        if (_traits::find(_buffer, _char_count, t_char) != NULL)
            return true;

        return false;
    }

    // Returns true if a C-style string is found in the string.
    inline bool Contains(const _char_type* ptr) const {
        if (ptr == NULL)
            throw "Input string cannot be null.";

        const _char_type* find = _traits::find_str(_buffer, ptr);
        if (find != NULL && find != _buffer)
            return true;

        return false;
    }

    // Returns true if [str] is found in the string.
    inline bool Contains(const WuBaseString& str) const {
        const _char_type* find = _traits::find_str(_buffer, str._buffer);
        if (find != NULL && find != _buffer)
            return true;

        return false;
    }

    // Returns true if the string starts with [t_char].
    inline bool StartsWith(const _char_type t_char) {
        if (_char_count == 0 || _buffer == NULL)
            return false;

        if (_buffer[0] == t_char)
            return true;

        return false;
    }

    // Returns true if the string ends with [t_char].
    inline bool EndsWith(const _char_type t_char) const {
        if (_char_count == 0 || _buffer == NULL)
            return false;
        
        // 0-based array plus /0.
        if (_buffer[_char_count - 2] == t_char)
            return true;

        return false;
    }

    // Returns true if the string ends with the C-style string [ptr]. ignore_case = true for case insensitive comparison.
    inline bool EndsWith(const _char_type* ptr, bool ignore_case = false) const {
        if (ptr == NULL)
            throw "Input string cannot be null.";

        if (_char_count == 0 || _buffer == NULL)
            return false;

        size_t input_len = _traits::length(ptr);
        if (input_len > this->Length())
            return false;

        if (ignore_case)
            return _traits::compare_no_case(_buffer + this->Length() - input_len, ptr, input_len) == 0;

        return _traits::compare(_buffer + this->Length() - input_len, ptr, input_len) == 0;
    }

    // Returns true if the string ends with [str]. ignore_case = true for case insensitive comparison.
    inline bool EndsWith(const WuBaseString& str, bool ignore_case = false) const {
        size_t input_len = str.Length();
        if (str.Length() > this->Length())
            return false;

        if (_char_count == 0 || _buffer == NULL)
            return false;

        if (ignore_case)
            return _traits::compare_no_case(_buffer + this->Length() - input_len, str._buffer, input_len) == 0;

        return _traits::compare(_buffer + this->Length() - input_len, str._buffer, input_len) == 0;
    }

    // Replaces all occurrences of [to_replace] with [replace_with].
    WuBaseString Replace(const _char_type to_replace, const _char_type replace_with) {
        std::vector<_char_type> buffer;
        for (size_t i = 0; i < _char_count; i++) {
            if (_buffer[i] == to_replace)
                buffer.push_back(replace_with);
            else
                buffer.push_back(_buffer[i]);
        }

        buffer.push_back('\0');
        return WuBaseString(buffer.data());
    }

    // Replaces all occurrences of [to_replace] with the C-style string [replace_with].
    WuBaseString Replace(const _char_type to_replace, const _char_type* replace_with) {
        if (replace_with == NULL)
            return *this;

        size_t replace_length = _traits::length(replace_with);
        std::vector<_char_type> buffer;
        for (size_t i = 0; i < _char_count; i++) {
            if (_buffer[i] == to_replace) {
                for (size_t i = 0; i < replace_length; i++) {
                    buffer.push_back(replace_with[i]);
                }
            }
            else
                buffer.push_back(_buffer[i]);
        }

        buffer.push_back('\0');
        WuBaseString output(buffer.data());

        return output;
    }

    // Replaces all occurrences of [to_replace] with the string [replace_with].
    WuBaseString Replace(const _char_type to_replace, const WuBaseString& replace_with) {
        if (replace_with == NULL || replace_with.Length() == 0)
            return *this;

        std::vector<_char_type> buffer;
        for (size_t i = 0; i < _char_count; i++) {
            if (_buffer[i] == to_replace) {
                for (size_t i = 0; i < replace_with->Length(); i++) {
                    buffer.push_back(replace_with[i]);
                }
            }
            else
                buffer.push_back(_buffer[i]);
        }

        buffer.push_back('\0');
        WuBaseString output(buffer.data());

        return output;
    }

    // Replaces all occurrences of the C-style string [to_replace] with the char [replace_with].
    WuBaseString Replace(const _char_type* to_replace, const _char_type replace_with) {
        if (to_replace == NULL)
            return *this;

        WuBaseString output(_buffer);

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        std::vector<_char_type> buffer;
        size_t to_replace_length = _traits::length(to_replace);

        // Every time the string is found in the buffer we replace it with [replace_with],
        // until _traits::find_str returns NULL.
        do {
            found_index = _traits::find_str(output._buffer, to_replace);
            if (found_index == NULL)
                break;

            has_buffer = true;
            buffer.clear();

            // Adding each character from the start of the buffer to the [found_index] offset.
            for (size_t i = 0; i < output.Length(); i++) {
                _char_type* current_offset = output._buffer + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    
                    // Since we are replacing a string with a character, we don't want to replace
                    // all [to_replace] characters with [replace_with].
                    if (!already_inserted) {
                        buffer.push_back(replace_with);
                        already_inserted = true;
                    }
                }
                else
                    buffer.push_back(output[i]);
            }

            buffer.push_back('\0');
            already_inserted = false;
            output = WuBaseString(buffer.data());

        } while (found_index != NULL);

        return output;
    }

    // Replaces all occurrences of the C-style string [to_replace] with the C-style string [replace_with].
    WuBaseString Replace(const _char_type* to_replace, const _char_type* replace_with) {
        if (to_replace == NULL || replace_with == NULL)
            return *this;

        WuBaseString output(_buffer);

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        std::vector<_char_type> buffer;
        size_t to_replace_length = _traits::length(to_replace);
        size_t replace_with_length = _traits::length(replace_with);

        // Every time the string is found in the buffer we replace it with [replace_with],
        // until _traits::find_str returns NULL.
        do {
            found_index = _traits::find_str(output._buffer, to_replace);
            if (found_index == NULL)
                break;

            has_buffer = true;
            buffer.clear();

            // Adding each character from the start of the buffer to the [found_index] offset.
            for (size_t i = 0; i < output.Length(); i++) {
                _char_type* current_offset = output._buffer + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {

                        // Same principle of before, but for every character from [replace_with].
                        for (size_t j = 0; j < replace_with_length; j++) {
                            buffer.push_back(replace_with[j]);
                        }
                        already_inserted = true;
                    }
                }
                else
                    buffer.push_back(output[i]);
            }

            buffer.push_back('\0');
            already_inserted = false;
            output = WuBaseString(buffer.data());

        } while (found_index != NULL);

        return output;
    }

    // Replaces all occurrences of the C-style string [to_replace] with the string [replace_with].
    WuBaseString Replace(const _char_type* to_replace, const WuBaseString& replace_with) {
        if (to_replace == NULL || replace_with.Length() == 0)
            return *this;

        WuBaseString output(_buffer);

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        std::vector<_char_type> buffer;
        size_t to_replace_length = _traits::length(to_replace);
        size_t replace_with_length = replace_with.Length();

        // Every time the string is found in the buffer we replace it with [replace_with],
        // until _traits::find_str returns NULL.
        do {
            found_index = _traits::find_str(output._buffer, to_replace);
            if (found_index == NULL)
                break;

            has_buffer = true;
            buffer.clear();

            // Adding each character from the start of the buffer to the [found_index] offset.
            for (size_t i = 0; i < output.Length(); i++) {
                _char_type* current_offset = output._buffer + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {

                        // Same principle of before for every character from [replace_with].
                        for (size_t j = 0; j < replace_with_length; j++) {
                            buffer.push_back(replace_with[j]);
                        }
                        already_inserted = true;
                    }
                }
                else
                    buffer.push_back(output[i]);
            }

            buffer.push_back('\0');
            already_inserted = false;
            output = WuBaseString(buffer.data());

        } while (found_index != NULL);

        return output;
    }

    // Replaces all occurrences of the string [to_replace] with [replace_with].
    WuBaseString Replace(const WuBaseString& to_replace, const _char_type replace_with) {
        if (to_replace.Length() == 0)
            return *this;

        WuBaseString output(_buffer);

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        std::vector<_char_type> buffer;
        size_t to_replace_length = to_replace.Length();
        do {
            found_index = _traits::find_str(output._buffer, to_replace._buffer);
            if (found_index == NULL)
                break;

            has_buffer = true;
            buffer.clear();
            for (size_t i = 0; i < output.Length(); i++) {
                _char_type* current_offset = output._buffer + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {
                        buffer.push_back(replace_with);
                        already_inserted = true;
                    }
                }
                else
                    buffer.push_back(output[i]);
            }

            buffer.push_back('\0');
            already_inserted = false;
            output = WuBaseString(buffer.data());

        } while (found_index != NULL);

        return output;
    }

    // Replaces all occurrences of the string [to_replace] with the C-style string [replace_with].
    WuBaseString Replace(const WuBaseString& to_replace, const _char_type* replace_with) {
        if (to_replace.Length() == 0 || replace_with == NULL)
            return *this;

        WuBaseString output(_buffer);

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        std::vector<_char_type> buffer;
        size_t to_replace_length = to_replace.Length();
        size_t replace_with_length = _traits::length(replace_with);
        do {
            found_index = _traits::find_str(output._buffer, to_replace._buffer);
            if (found_index == NULL)
                break;

            has_buffer = true;
            buffer.clear();
            for (size_t i = 0; i < output.Length(); i++) {
                _char_type* current_offset = output._buffer + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {
                        for (size_t j = 0; j < replace_with_length; j++) {
                            buffer.push_back(replace_with[j]);
                        }
                        already_inserted = true;
                    }
                }
                else
                    buffer.push_back(output[i]);
            }

            buffer.push_back('\0');
            already_inserted = false;
            output = WuBaseString(buffer.data());

        } while (found_index != NULL);

        return output;
    }

    // Replaces all occurrences of the string [to_replace] with the string [replace_with].
    WuBaseString Replace(const WuBaseString& to_replace, const WuBaseString& replace_with) {
        if (to_replace.Length() == 0 || replace_with.Length() == 0)
            return *this;

        WuBaseString output(_buffer);

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        std::vector<_char_type> buffer;
        size_t to_replace_length = to_replace.Length();
        size_t replace_with_length = replace_with.Length();
        do {
            found_index = _traits::find_str(output._buffer, to_replace._buffer);
            if (found_index == NULL)
                break;

            has_buffer = true;
            buffer.clear();
            for (size_t i = 0; i < output.Length(); i++) {
                _char_type* current_offset = output._buffer + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {
                        for (size_t j = 0; j < replace_with_length; j++) {
                            buffer.push_back(replace_with._buffer[j]);
                        }
                        already_inserted = true;
                    }
                }
                else
                    buffer.push_back(output[i]);
            }

            buffer.push_back('\0');
            already_inserted = false;
            output = WuBaseString(buffer.data());

        } while (found_index != NULL);

        return output;
    }

    // Splits the string into a vector of strings, at every occurrence of [split_on].
    std::vector<WuBaseString> Split(const _char_type split_on) {
        std::vector<WuBaseString> output;
        std::vector<_char_type> buffer;
        
        // Adds every character to a buffer until the char is found.
        // Then pushes the string to the vector, and cleans the buffer.
        for (size_t i = 0; i < _char_count; i++) {
            if (_buffer[i] == split_on) {
                if (!buffer.empty()) {
                    buffer.push_back('\0');
                    output.push_back(buffer.data());
                    buffer.clear();
                }
            }
            else {
                buffer.push_back(_buffer[i]);
            }
        }

        // If there are characters after the last occurrence, they will be in the buffer.
        if (!buffer.empty()) {
            buffer.push_back('\0');
            output.push_back(buffer.data());
        }

        return output;
    }

    // Splits the string into a vector of strings, at every occurrence of [split_on].
    std::vector<WuBaseString> Split(const _char_type split_on) const {
        std::vector<WuBaseString> output;
        std::vector<_char_type> buffer;

        // Adds every character to a buffer until the char is found.
        // Then pushes the string to the vector, and cleans the buffer.
        for (size_t i = 0; i < _char_count; i++) {
            if (_buffer[i] == split_on) {
                if (!buffer.empty()) {
                    buffer.push_back('\0');
                    output.push_back(buffer.data());
                    buffer.clear();
                }
            }
            else {
                buffer.push_back(_buffer[i]);
            }
        }

        // If there are characters after the last occurrence, they will be in the buffer.
        if (!buffer.empty()) {
            buffer.push_back('\0');
            output.push_back(buffer.data());
        }

        return output;
    }

    // Splits the string into a vector of strings, at every occurrence of the C-style string [split_on].
    std::vector<WuBaseString> Split(const _char_type* split_on) {
        std::vector<WuBaseString> output;
        if (split_on == NULL) {
            output.push_back(_buffer);
            return output;
        }

        const _char_type* found_offset;
        std::vector<_char_type> buffer;
        _char_type* current_offset = _buffer;
        size_t split_on_length = _traits::length(split_on);

        // Going through each occurrence. For each one we advance the
        // [current_offset] to the occurrence offset, plus its length.
        do {
            found_offset = _traits::find_str(current_offset, split_on);
            if (found_offset != NULL) {

                // Pushing every character until we reach [found_offset].
                buffer.clear();
                while (current_offset < found_offset) {
                    buffer.push_back(current_offset[0]);
                    current_offset++;
                }

                // If [found_offset] is at the beginning of the string, and
                // we don't check if the buffer is empty, we create an empty
                // string entry in the output.
                if (!buffer.empty()) {
                    buffer.push_back('\0');
                    output.push_back(buffer.data());
                }

                current_offset += split_on_length;
                if (current_offset >= _buffer + _char_count)
                    break;
            }
            else {
                // Checking if we are at the end of the string.
                if (current_offset >= _buffer + _char_count)
                    break;

                // Adding each char until the end of the string.
                buffer.clear();
                while (current_offset < _buffer + _char_count) {
                    buffer.push_back(current_offset[0]);
                    current_offset++;
                }

                buffer.push_back('\0');
                output.push_back(buffer.data());

                break;
            }

        } while (found_offset != NULL);

        return output;
    }

    // Splits the string into a vector of strings, at every occurrence of the C-style string [split_on].
    std::vector<WuBaseString> Split(const _char_type* split_on) const {
        std::vector<WuBaseString> output;
        if (split_on == NULL) {
            output.push_back(_buffer);
            return output;
        }

        const _char_type* found_offset;
        std::vector<_char_type> buffer;
        _char_type* current_offset = _buffer;
        size_t split_on_length = _traits::length(split_on);

        // Going through each occurrence. For each one we advance the
        // [current_offset] to the occurrence offset, plus its length.
        do {
            found_offset = _traits::find_str(current_offset, split_on);
            if (found_offset != NULL) {

                // Pushing every character until we reach [found_offset].
                buffer.clear();
                while (current_offset < found_offset) {
                    buffer.push_back(current_offset[0]);
                    current_offset++;
                }

                // If [found_offset] is at the beginning of the string, and
                // we don't check if the buffer is empty, we create an empty
                // string entry in the output.
                if (!buffer.empty()) {
                    buffer.push_back('\0');
                    output.push_back(buffer.data());
                }

                current_offset += split_on_length;
                if (current_offset >= _buffer + _char_count)
                    break;
            }
            else {
                // Checking if we are at the end of the string.
                if (current_offset >= _buffer + _char_count)
                    break;

                // Adding each char until the end of the string.
                buffer.clear();
                while (current_offset < _buffer + _char_count) {
                    buffer.push_back(current_offset[0]);
                    current_offset++;
                }

                buffer.push_back('\0');
                output.push_back(buffer.data());

                break;
            }

        } while (found_offset != NULL);

        return output;
    }

    // Splits the string into a vector of strings, at every occurrence of the string [split_on].
    std::vector<WuBaseString> Split(const WuBaseString& split_on) {
        std::vector<WuBaseString> output;
        if (split_on.Length() == 0) {
            output.push_back(_buffer);
            return output;
        }

        const _char_type* found_offset;
        std::vector<_char_type> buffer;
        _char_type* current_offset = _buffer;
        size_t split_on_length = split_on.Length();

        // Going through each occurrence. For each one we advance the
        // [current_offset] to the occurrence offset, plus its length.
        do {
            found_offset = _traits::find_str(current_offset, split_on._buffer);
            if (found_offset != NULL) {
                
                // Pushing every character until we reach [found_offset].
                buffer.clear();
                while (current_offset < found_offset) {
                    buffer.push_back(current_offset[0]);
                    current_offset++;
                }

                // If [found_offset] is at the beginning of the string, and
                // we don't check if the buffer is empty, we create an empty
                // string entry in the output.
                if (!buffer.empty()) {
                    buffer.push_back('\0');
                    output.push_back(buffer.data());
                }

                current_offset += split_on_length;
                if (current_offset >= _buffer + _char_count)
                    break;
            }
            else {
                // Checking if we are at the end of the string.
                if (current_offset >= _buffer + _char_count)
                    break;

                // Adding each char until the end of the string.
                buffer.clear();
                while (current_offset < _buffer + _char_count) {
                    buffer.push_back(current_offset[0]);
                    current_offset++;
                }

                buffer.push_back('\0');
                output.push_back(buffer.data());

                break;
            }

        } while (found_offset != NULL);

        return output;
    }

    // Splits the string into a vector of strings, at every occurrence of the string [split_on].
    std::vector<WuBaseString> Split(const WuBaseString& split_on) const {
        std::vector<WuBaseString> output;
        if (split_on.Length() == 0) {
            output.push_back(_buffer);
            return output;
        }

        const _char_type* found_offset;
        std::vector<_char_type> buffer;
        _char_type* current_offset = _buffer;
        size_t split_on_length = split_on.Length();

        // Going through each occurrence. For each one we advance the
        // [current_offset] to the occurrence offset, plus its length.
        do {
            found_offset = _traits::find_str(current_offset, split_on._buffer);
            if (found_offset != NULL) {

                // Pushing every character until we reach [found_offset].
                buffer.clear();
                while (current_offset < found_offset) {
                    buffer.push_back(current_offset[0]);
                    current_offset++;
                }

                // If [found_offset] is at the beginning of the string, and
                // we don't check if the buffer is empty, we create an empty
                // string entry in the output.
                if (!buffer.empty()) {
                    buffer.push_back('\0');
                    output.push_back(buffer.data());
                }

                current_offset += split_on_length;
                if (current_offset >= _buffer + _char_count)
                    break;
            }
            else {
                // Checking if we are at the end of the string.
                if (current_offset >= _buffer + _char_count)
                    break;

                // Adding each char until the end of the string.
                buffer.clear();
                while (current_offset < _buffer + _char_count) {
                    buffer.push_back(current_offset[0]);
                    current_offset++;
                }

                buffer.push_back('\0');
                output.push_back(buffer.data());

                break;
            }

        } while (found_offset != NULL);

        return output;
    }

    // Addition operator.
    // Adds the C-style string [right] to [left].
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

    // Adds the string [right] to [left].
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

    // Indexing operator. Checks if the index is within boundaries, and returns
    // the corresponding _char_type.
    inline _char_type operator[](const size_t index) {
        if (index < 0 && index > _char_count)
            throw "Index outside the boundaries of this string.";

        return _buffer[index];
    }

    // Assignment operator.
    // Effectively swaps the string by a new string constructed from [other].
    // Extremely recommend to check this out, where I based this solution from:
    // https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
    inline WuBaseString& operator=(const _char_type* other) {
        WuBaseString otherStr(other);
        Swap(*this, otherStr);

        return *this;
    }

    // Swaps the string with [other].
    inline WuBaseString& operator=(WuBaseString other) {
        Swap(*this, other);

        return *this;
    }

    // Compound addition operator.
    // Concatenates the string with the C-style string [other].
    inline WuBaseString& operator+=(const _char_type* other) {
        *this = *this + other;

        return *this;
    }

    // Concatenates the string with [other].
    inline WuBaseString& operator+=(const WuBaseString& other) {
        *this = *this + other;

        return *this;
    }

    // Equality operator.
    // Checks if the string is equal to the C-style string [right].
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

    // Checks if the string is equal to [right].
    inline bool operator==(const WuBaseString& right) const {
        size_t biggest = (((this->Length()) > (right.Length())) ? (this->Length()) : (right.Length()));
        return _traits::compare(_buffer, right._buffer, biggest) == 0;
    }

    // Inequality operator.
    inline bool operator!=(const _char_type* right) const {
        return !(*this == right);
    }

    inline bool operator!=(const WuBaseString& right) const {
        return !(*this == right);
    }

    // Less-than operator.
    // Checks if the string is less than the C-style string [right].
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

    // Checks if the string is less than [right].
    inline bool operator<(const WuBaseString& right) const {
        size_t biggest = (((this->Length()) > (right.Length())) ? (this->Length()) : (right.Length()));
        return _traits::compare(_buffer, right._buffer, biggest) < 0;
    }

    // Greater-than operator.
    // Checks if the string is greater than the C-style string [right].
    inline bool operator>(const _char_type* right) const {
        return !(*this < right);
    }

    // Checks if the string is greater than [right].
    inline bool operator>(const WuBaseString& right) const {
        return !(*this < right);
    }

    // Less-than-or-equal operator.
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

    // Greater-than-or-equal operator.
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

// These expressions represent each char type strings.
using WWuString = WuBaseString<wchar_t, char_traits<wchar_t>>;
using WuString = WuBaseString<char, char_traits<char>>;
using u16WuString = WuBaseString<char16_t, char_traits<char16_t>>;
using u32WuString = WuBaseString<char32_t, char_traits<char32_t>>;

#if defined(__cpp_lib_char8_t)
using u8WuString = WuBaseString<char8_t, char_traits<char8_t>>;
#endif

// Conversion from narrow to wide, and vice versa is inevitable. Most Windows APIs
// supports Unicode, which we use as default, but there are functions that use only ANSI.
// TODO:
//     See if it's possible to implement in the char traits, so it can be called
//     from the instances.
//
_NODISCARD static WWuString WuStringToWide(const WuString& other) {
    WuAllocator* allocator = new WuAllocator();

    size_t other_size = other.Length();
    size_t bytes_count = other.Length() * 2;
    size_t converted_count = 0;

    wchar_t* new_buffer = static_cast<wchar_t*>(allocator->allocate(bytes_count + 2));

    mbstate_t state = { 0 };

    // Third argument is the size in words. Last is count of wide chars minus \0.
    const char* buffer = other.GetBuffer();
    mbsrtowcs_s(&converted_count, new_buffer, other_size + 2, &buffer, other_size, &state);

    WWuString result(new_buffer);
    return result;
}

_NODISCARD static WWuString WuStringToWide(const char* other) {
    WuAllocator* allocator = new WuAllocator();

    size_t other_size = strlen(other);
    size_t bytes_count = other_size * 2;
    size_t converted_count = 0;

    wchar_t* new_buffer = static_cast<wchar_t*>(allocator->allocate(bytes_count + 2));

    mbstate_t state = { 0 };

    // Third argument is the size in words. Last is count of wide chars minus \0.
    mbsrtowcs_s(&converted_count, new_buffer, other_size + 2, &other, other_size, &state);

    WWuString result(new_buffer);
    return result;
}

_NODISCARD static WWuString WuStringToWide(const char* other, DWORD code_page) {
    WuAllocator* allocator = new WuAllocator();

    size_t other_size = strlen(other);
    size_t bytes_count = other_size * 2;
    size_t converted_count = 0;

    wchar_t* new_buffer = static_cast<wchar_t*>(allocator->allocate(bytes_count + 2));

    MultiByteToWideChar(code_page, 0, other, -1, new_buffer, static_cast<int>(bytes_count + 2));

    WWuString result(new_buffer);
    return result;
}

_NODISCARD static WWuString WuStringToWide(const WuString& other, DWORD code_page) {
    WuAllocator* allocator = new WuAllocator();

    size_t other_size = other.Length();
    size_t bytes_count = other_size * 2;
    size_t converted_count = 0;

    wchar_t* new_buffer = static_cast<wchar_t*>(allocator->allocate(bytes_count + 2));

    MultiByteToWideChar(code_page, 0, other.GetBuffer(), -1, new_buffer, static_cast<int>(bytes_count + 2));

    WWuString result(new_buffer);
    return result;
}

_NODISCARD static WuString WWuStringToNarrow(const WWuString& other) {
    WuAllocator* allocator = new WuAllocator();

    // Allocate two bytes in the multibyte output string for every wide
    // character in the input string (including a wide character
    // null). Because a multibyte character can be one or two bytes,
    // you should allot two bytes for each character. Having extra
    // space for the new string isn't an error, but having
    // insufficient space is a potential security problem.
    //
    // https://learn.microsoft.com/en-us/cpp/text/how-to-convert-between-various-string-types?view=msvc-170
    //
    size_t other_count = other.Length();
    size_t new_size = (other_count + 1) * 2;
    size_t converted_count = 0;
    char* new_buffer = static_cast<char*>(allocator->allocate(new_size));
    wcstombs_s(&converted_count, new_buffer, new_size, other.GetBuffer(), other_count);

    WuString result(new_buffer);
    
    return(result);
}

_NODISCARD static WuString WWuStringToNarrow(const wchar_t* other) {
    WuAllocator* allocator = new WuAllocator();

    size_t other_count = wcslen(other);
    size_t new_size = (other_count + 1) * 2;
    size_t converted_count = 0;
    char* new_buffer = static_cast<char*>(allocator->allocate(new_size));
    wcstombs_s(&converted_count, new_buffer, new_size, other, other_count);

    WuString result(new_buffer);
    
    return(result);
}

_NODISCARD static WuString WWuStringToNarrow(const wchar_t* other, DWORD codePage) {
    WuAllocator* allocator = new WuAllocator();

    size_t other_count = wcslen(other);
    size_t new_size = (other_count + 1) * 2;
    char* new_buffer = static_cast<char*>(allocator->allocate(new_size));
    WideCharToMultiByte(codePage, 0, other, -1, new_buffer, static_cast<int>(new_size), NULL, NULL);

    WuString result(new_buffer);
    
    return(result);
}

_NODISCARD static WuString WWuStringToNarrow(const WWuString& other, DWORD codePage) {
    WuAllocator* allocator = new WuAllocator();

    size_t other_count = other.Length();
    size_t new_size = (other_count + 1) * 2;
    char* new_buffer = static_cast<char*>(allocator->allocate(new_size));
    WideCharToMultiByte(codePage, 0, other.GetBuffer(), -1, new_buffer, static_cast<int>(new_size), NULL, NULL);

    WuString result(new_buffer);

    return(result);
}
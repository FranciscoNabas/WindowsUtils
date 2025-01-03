#pragma once
#pragma unmanaged

#include <cstdint>
#include <xstring>
#include <xmemory>

#include "Windows.h"

#include "WuList.h"
#include "Expressions.h"

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
//  This class serves as a base template for the 8-bit, 16-bit, and 32-bit
//  types that should be used throughout the code.

// ------------------------------------------------------------------------
//
//  Copyright (c) Francisco Nabas 2024.
//
//  This code, and the WindowsUtils module are distributed under
//  the MIT license. (are you sure you want to use this?)
//
///////////////////////////////////////////////////////////////////////////

#pragma region Iterators

template <class MyStr>
class WuStringConstIterator : public std::_Iterator_base
{
public:
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename MyStr::value_type;
    using difference_type = typename MyStr::difference_type;
    using pointer = typename MyStr::const_pointer;
    using reference = const value_type&;

    constexpr WuStringConstIterator() noexcept : Pointer() {}

    constexpr WuStringConstIterator(pointer pointerArg, const std::_Container_base* pointerString) noexcept
        : Pointer(pointerArg)
    {
        this->_Adopt(pointerString);
    }

    _NODISCARD constexpr reference operator*() const noexcept { return *Pointer; }
    _NODISCARD constexpr pointer operator->() const noexcept { return std::pointer_traits<pointer>::pointer_to(**this); }

    constexpr WuStringConstIterator& operator++() noexcept
    {
        ++Pointer;
        return *this;
    }

    constexpr WuStringConstIterator operator++(int) noexcept
    {
        WuStringConstIterator temp = *this;
        ++*this;
        return temp;
    }

    constexpr WuStringConstIterator& operator--() noexcept
    {
        --Pointer;
        return *this;
    }

    constexpr WuStringConstIterator operator--(int) noexcept
    {
        WuStringConstIterator temp = *this;
        --*this;
        return temp;
    }

    constexpr WuStringConstIterator& operator+=(const difference_type offset) noexcept
    {
        Pointer += offset;
        return *this;
    }

    _NODISCARD constexpr WuStringConstIterator operator+(const difference_type offset) const noexcept
    {
        WuStringConstIterator temp = *this;
        temp += offset;
        return temp;
    }

    _NODISCARD friend constexpr WuStringConstIterator operator+(const difference_type offset, WuStringConstIterator next) noexcept
    {
        next += offset;
        return next;
    }

    constexpr WuStringConstIterator& operator-=(const difference_type offset) noexcept
    {
        return *this += -offset;
    }

    _NODISCARD constexpr WuStringConstIterator operator-(const difference_type offset) const noexcept
    {
        WuStringConstIterator temp = *this;
        temp -= offset;
        return temp;
    }

    _NODISCARD constexpr difference_type operator-(const WuStringConstIterator& right) const noexcept
    {
        return static_cast<difference_type>(Pointer - right.Pointer);
    }

    _NODISCARD constexpr reference operator[](const difference_type offset) const noexcept
    {
        return *(*this + offset);
    }

    _NODISCARD constexpr bool operator==(const WuStringConstIterator& right) const noexcept
    {
        return Pointer == right.Pointer;
    }

    _NODISCARD constexpr std::strong_ordering operator<=>(const WuStringConstIterator& right) const noexcept
    {
        return std::_Unfancy_maybe_null(Pointer) <=> std::_Unfancy_maybe_null(right.Pointer);
    }

    using _Prevent_inheriting_unwrap = WuStringConstIterator;

    _NODISCARD constexpr const value_type* _Unwrapped() const noexcept
    {
        return std::_Unfancy_maybe_null(Pointer);
    }

    constexpr void _Seek_to(const value_type* it) noexcept
    {
        Pointer = std::_Refancy_maybe_null<pointer>(const_cast<value_type*>(it));
    }

    pointer Pointer;
};

template <class MyStr>
struct std::pointer_traits<WuStringConstIterator<MyStr>>
{
    using pointer = WuStringConstIterator<MyStr>;
    using element_type = const pointer::value_type;
    using difference_type = pointer::difference_type;

    _NODISCARD static constexpr element_type* to_address(const pointer iterator) noexcept { return const_cast<element_type*>(std::to_address(iterator.Pointer)); }
};

template <class MyStr>
class WuStringIterator : public WuStringConstIterator<MyStr>
{
public:
    using MyBase = WuStringConstIterator<MyStr>;
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename MyStr::value_type;
    using difference_type = typename MyStr::difference_type;
    using pointer = typename MyStr::pointer;
    using reference = value_type&;

    using MyBase::MyBase;

    _NODISCARD constexpr reference operator*() const noexcept { return const_cast<reference>(MyBase::operator*()); }
    _NODISCARD constexpr pointer operator->() const noexcept { return std::pointer_traits<pointer>::pointer_to(**this); }

    constexpr WuStringIterator& operator++() noexcept
    {
        MyBase::operator++();
        return *this;
    }

    constexpr WuStringIterator operator++(int) noexcept
    {
        WuStringIterator temp = *this;
        MyBase::operator++();
        return temp;
    }

    constexpr WuStringIterator& operator--() noexcept
    {
        MyBase::operator--();
        return *this;
    }

    constexpr WuStringIterator operator--(int) noexcept
    {
        WuStringIterator temp = *this;
        MyBase::operator--();
        return temp;
    }

    constexpr WuStringIterator& operator+=(const difference_type offset) noexcept
    {
        MyBase::operator+=(offset);
        return *this;
    }

    _NODISCARD constexpr WuStringIterator operator+(const difference_type offset) const noexcept
    {
        WuStringIterator temp = *this;
        temp += offset;
        return temp;
    }

    _NODISCARD friend constexpr WuStringIterator operator+(
        const difference_type offset, WuStringIterator next) noexcept
    {
        next += offset;
        return next;
    }

    constexpr WuStringIterator& operator-=(const difference_type offset) noexcept
    {
        MyBase::operator-=(offset);
        return *this;
    }

    using MyBase::operator-;

    _NODISCARD constexpr WuStringIterator operator-(const difference_type offset) const noexcept
    {
        WuStringIterator temp = *this;
        temp -= offset;
        return temp;
    }

    _NODISCARD constexpr reference operator[](const difference_type offset) const noexcept
    {
        return const_cast<reference>(MyBase::operator[](offset));
    }

    using _Prevent_inheriting_unwrap = WuStringIterator;

    _NODISCARD constexpr value_type* _Unwrapped() const noexcept
    {
        return const_cast<value_type*>(std::_Unfancy_maybe_null(this->Pointer));
    }
};

template <class MyStr>
struct std::pointer_traits<WuStringIterator<MyStr>>
{
    using pointer = WuStringIterator<MyStr>;
    using element_type = pointer::value_type;
    using difference_type = pointer::difference_type;

    _NODISCARD static constexpr element_type* to_address(const pointer iterator) noexcept { return const_cast<element_type*>(std::to_address(iterator.Pointer)); }
};

#pragma endregion

#pragma region Support

template<class ValueType, class SizeType, class DifferenceType, class Pointer, class ConstPointer>
struct WuStringIterTypes
{
    using value_type = ValueType;
    using size_type = SizeType;
    using difference_type = DifferenceType;
    using pointer = Pointer;
    using const_pointer = ConstPointer;
};

template<class ValTypes>
class WuStringValue : public std::_Container_base
{
public:
    using value_type = typename ValTypes::value_type;
    using size_type = typename ValTypes::size_type;
    using difference_type = typename ValTypes::difference_type;
    using pointer = typename ValTypes::pointer;
    using const_pointer = typename ValTypes::const_pointer;
    using reference = value_type&;
    using const_reference = const value_type&;

    static constexpr size_type BUFFER_SIZE = 16 / sizeof(value_type) < 1 ? 1 : 16 / sizeof(value_type);
    static constexpr size_type SmallStringCapacity = BUFFER_SIZE - 1;
    static constexpr size_type AllocMask = sizeof(value_type) <= 1 ? 15
        : sizeof(value_type) <= 2 ? 7
        : sizeof(value_type) <= 4 ? 3
        : sizeof(value_type) <= 8 ? 1
        : 0;

    constexpr WuStringValue() noexcept
        : Storage()
    {
    }

    _NODISCARD constexpr value_type* GetStorage() noexcept
    {
        value_type* result = Storage.Buffer;
        if (LargeModeEngaged())
            result = std::_Unfancy(Storage.Pointer);

        return result;
    }

    _NODISCARD constexpr const value_type* GetStorage() const noexcept
    {
        const value_type* result = Storage.Buffer;
        if (LargeModeEngaged())
            result = std::_Unfancy(Storage.Pointer);

        return result;
    }

    _NODISCARD constexpr bool LargeModeEngaged() const noexcept { return MyCapacity > SmallStringCapacity; }

    _NODISCARD _CONSTEXPR20 size_type ClampSuffixSize(const size_type offset, const size_type size) const noexcept
    {
        return (std::min)(size, MySize - offset);
    }

    constexpr void ActivateSmallStringBuffer() noexcept
    {
        if (std::is_constant_evaluated()) {
            for (size_type i = 0; i < BUFFER_SIZE; i++) {
                Storage.Buffer[i] = value_type();
            }
        }
    }

    // Storage for small buffer or pointer to larger one.
    union BufferType
    {
        pointer Pointer;
        char _Alias[BUFFER_SIZE];
        value_type Buffer[BUFFER_SIZE];

        constexpr BufferType() noexcept
            : Buffer()
        {
        }

        constexpr ~BufferType() noexcept {}

        constexpr void _Switch_to_buf() noexcept
        {
            std::_Destroy_in_place(Pointer);

            if (std::is_constant_evaluated()) {
                for (size_type i = 0; i < BUFFER_SIZE; i++) {
                    Buffer[i] = value_type();
                }
            }
        }
    };

    BufferType Storage;

    size_type MySize = 0;
    size_type MyCapacity = 0;
};

enum class TrimType
{
    Head = 0x1,
    Tail = 0x2,
    Both = 0x3,
};

inline TrimType operator |(TrimType first, TrimType second) { return static_cast<TrimType>(static_cast<int>(first) | static_cast<int>(second)); }
inline TrimType operator &(TrimType first, TrimType second) { return static_cast<TrimType>(static_cast<int>(first) & static_cast<int>(second)); }
inline TrimType operator |=(TrimType first, TrimType second) { return first = first | second; }
inline bool operator ==(TrimType first, TrimType second) { return static_cast<int>(first) == static_cast<int>(second); }
inline bool operator !=(TrimType first, TrimType second) { return !(first == second); }

#pragma endregion

#pragma region CharTraits

template <class _char_type>
struct _WChar_traits_ex : std::_WChar_traits<_char_type>
{
    _NODISCARD static constexpr int compare_no_case(_In_reads_(count) const _char_type* const left,
        _In_reads_(count) const _char_type* const right, const size_t count) noexcept
    {
        return _wcsnicmp(left, right, count);
    }

    template <class TAlloc, class... TArgs>
    _NODISCARD static constexpr _char_type* format(TAlloc allocator, const _char_type* fmt, TArgs&&... args)
    {
        if (fmt) {
            int char_count = std::swprintf(nullptr, 0, fmt, args...) + 1;
            _char_type* buffer = allocator.allocate(char_count);
            std::swprintf(buffer, char_count, fmt, args...);

            return buffer;
        }

        return nullptr;
    }

    _NODISCARD static constexpr const _char_type* find_str(const _char_type* first, const _char_type* second)
    {
        return wcsstr(first, second);
    }

    static constexpr void to_upper(_In_reads_(length) _char_type* str, const size_t length)
    {
        if (!str || length == 0)
            return;

        _wcsupr_s(str, length);
    }

    static constexpr void to_lower(_In_reads_(length) _char_type* str, const size_t length)
    {
        if (!str || length == 0)
            return;

        _wcslwr_s(str, length);
    }

    template<class TAllocator, is_addition_supported T>
    _NODISCARD static constexpr _char_type* to_string(TAllocator allocator, const T& obj)
    {
        const wchar_t* template_str{ };
        if constexpr (is_binary_digit<T>) template_str = L"%llu";
        else if constexpr (is_floating_point<T>) template_str = L"%lf";
        else if constexpr (is_single_byte_character<T>) template_str = L"%C";
        else if constexpr (is_double_byte_character<T>) template_str = L"%c";
        else if constexpr (is_single_byte_character_array<T> || is_bounded_char_array<T>{}) template_str = L"%S";
        else if constexpr (is_double_byte_character_array<T> || is_bounded_wide_char_array<T>{}) template_str = L"%s";
        else return nullptr;

        int char_count = std::swprintf(nullptr, 0, template_str, obj) + 1;
        _char_type* buffer = allocator.allocate(char_count);
        std::swprintf(buffer, char_count, template_str, obj);

        return buffer;
    }

    static constexpr bool is_null_or_white_space(_In_reads_(count) const _char_type* ptr, const size_t count)
    {
        for (size_t i = 0; i < count; i++) {
            if (ptr[i] != L' ')
                return false;
        }

        return true;
    }

    template <class TAllocator>
    _NODISCARD static constexpr wchar_t* to_wide(TAllocator allocator, _In_reads_(count) const _char_type* ptr, const size_t count, const uint32_t code_page)
    {
        return ptr;
    }

    template <class TAllocator>
    _NODISCARD static constexpr char* to_narrow(TAllocator allocator, _In_reads_(count) const _char_type* ptr, const size_t count)
    {
        size_t converted_count = 0;
        const size_t new_size = count + 1;
        char* new_buffer = reinterpret_cast<char*>(allocator.allocate(new_size));
        memset(new_buffer, 0, new_size);
        wcstombs_s(&converted_count, new_buffer, new_size, ptr, count);

        return new_buffer;
    }

    template <class TAllocator>
    _NODISCARD static constexpr char* to_mb(TAllocator allocator, _In_reads_(count) const _char_type* ptr, const size_t count, const uint32_t code_page, _In_opt_ const char* default_char)
    {
        BOOL useDefaultChar = default_char ? TRUE : FALSE;
        const int our_count = static_cast<int>(count);
        const int size = our_count + 1;
        char* buffer = reinterpret_cast<char*>(allocator.allocate(size));
        memset(buffer, 0, size);
        if (!WideCharToMultiByte(code_page, 0, ptr, size, buffer, size * sizeof(wchar_t), default_char, &useDefaultChar))
            return nullptr;

        return buffer;
    }

    static inline bool is_null_or_white_space(const _char_type character)
    {
        return character == L'\0' || character == L' ';
    }
};

template <class _char_type, class _int_type>
struct _Narrow_char_traits_ex : std::_Narrow_char_traits<_char_type, _int_type>
{
    _NODISCARD static constexpr int compare_no_case(_In_reads_(count) const _char_type* const left,
        _In_reads_(count) const _char_type* const right, const size_t count) noexcept
    {
        return _strnicmp(left, right, count);
    }

    template <class TAlloc, class... TArgs>
    _NODISCARD static constexpr _char_type* format(TAlloc allocator, const _char_type* fmt, TArgs&&... args)
    {
        if (fmt) {
            int char_count = std::snprintf(nullptr, 0, fmt, args...) + 1;
            _char_type* buffer = allocator.allocate(char_count);
            std::snprintf(buffer, char_count, fmt, args...);

            return buffer;
        }

        return nullptr;
    }

    _NODISCARD static constexpr const _char_type* find_str(const _char_type* first, const _char_type* second)
    {
        return strstr(first, second);
    }

    static constexpr void to_upper(_In_reads_(length) _char_type* str, const size_t length)
    {
        if (!str || length == 0)
            return;

        _strupr_s(str, length);
    }

    static constexpr void to_lower(_In_reads_(length) _char_type* str, const size_t length)
    {
        if (!str || length == 0)
            return;

        _strlwr_s(str, length);
    }

    template<class TAllocator, is_addition_supported T>
    _NODISCARD static constexpr _char_type* to_string(TAllocator allocator, const T& obj)
    {
        const char* template_str{ };
        if constexpr (is_binary_digit<T>) template_str = "%llu";
        else if constexpr (is_floating_point<T>) template_str = "%lf";
        else if constexpr (is_single_byte_character<T>) template_str = "%C";
        else if constexpr (is_double_byte_character<T>) template_str = "%c";
        else if constexpr (is_single_byte_character_array<T> || is_bounded_char_array<T>{}) template_str = "%S";
        else if constexpr (is_double_byte_character_array<T> || is_bounded_wide_char_array<T>{}) template_str = "%s";
        else return nullptr;

        int char_count = std::snprintf(nullptr, 0, template_str, obj) + 1;
        _char_type* buffer = allocator.allocate(char_count);
        std::snprintf(buffer, char_count, template_str, obj);

        return buffer;
    }

    static constexpr bool is_null_or_white_space(_In_reads_(count) const _char_type* ptr, const size_t count)
    {
        for (size_t i = 0; i < count; i++) {
            if (ptr[i] != ' ')
                return false;
        }

        return true;
    }

    template <class TAllocator>
    _NODISCARD static constexpr wchar_t* to_wide(TAllocator allocator, _In_reads_(count) const _char_type* ptr, const size_t count, const uint32_t code_page)
    {
        const int our_count = static_cast<int>(count);
        const int size = (our_count + 1) * sizeof(wchar_t);
        wchar_t* buffer = reinterpret_cast<wchar_t*>(allocator.allocate(size));
        memset(buffer, 0, size);
        if (!MultiByteToWideChar(code_page, 0, ptr, our_count, buffer, size))
            return nullptr;

        return buffer;
    }

    template <class TAllocator>
    _NODISCARD static constexpr char* to_narrow(TAllocator allocator, _In_reads_(count) const _char_type* ptr, const size_t count)
    {
        return ptr;
    }

    template <class TAllocator>
    _NODISCARD static constexpr char* to_mb(TAllocator allocator, _In_reads_(count) const _char_type* ptr, const size_t count, const uint32_t code_page, _In_opt_ const char* default_char)
    {
        return ptr;
    }

    static inline bool is_null_or_white_space(_char_type character)
    {
        return character == '\0' || character == ' ';
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

template <class _char_type, bool = std::_Is_character<_char_type>::value>
class WuStringBitmap
{
public:
    constexpr bool Mark(const _char_type* first, const _char_type* last) noexcept
    {
        for (; first != last; ++first) {
            m_matches[static_cast<unsigned char>(*first)] = true;
        }

        return true;
    }

    constexpr bool Match(const _char_type c) const noexcept
    {
        return m_matches[static_cast<unsigned char>(c)];
    }

private:
    bool m_matches[256] = { };
};

template <class _char_type>
class WuStringBitmap<_char_type, false>
{
public:
    static_assert(std::is_unsigned_v<_char_type>,
        "Standard char_traits is only provided for char, wchar_t, char16_t, and char32_t. See N4950 [char.traits]. "
        "Visual C++ accepts other unsigned integral types as an extension."
        );

    constexpr bool Mark(const _char_type* first, const _char_type* last) noexcept
    {
        for (; first != last; ++first) {
            const auto c = *first;
            if (c >= 256U) {
                return false;
            }

            m_matches[static_cast<unsigned char>(c)] = true;
        }

        return true;
    }

    constexpr bool Match(const _char_type c) const noexcept
    {
        return c < 256U && m_matches[c];
    }

private:
    bool m_matches[256] = { };
};

#pragma endregion

// Base string class.

template <class _char_type, class _traits = char_traits<_char_type>, class _allocator = std::allocator<_char_type>>
class WuBaseString
{
private:
    friend std::_Tidy_deallocate_guard<WuBaseString>;

    using allocator_type  = std::_Rebind_alloc_t<_allocator, _char_type>;
    using alty_traits     = std::allocator_traits<allocator_type>;

    using _scary_val = WuStringValue<std::conditional_t<std::_Is_simple_alloc_v<allocator_type>, std::_Simple_types<_char_type>,
        WuStringIterTypes<_char_type, typename alty_traits::size_type, typename alty_traits::difference_type,
        typename alty_traits::pointer, typename alty_traits::const_pointer>>>;

public:
    using value_type              = _char_type;
    using size_type               = typename alty_traits::size_type;
    using difference_type         = typename alty_traits::difference_type;
    using pointer                 = typename alty_traits::pointer;
    using const_pointer           = typename alty_traits::const_pointer;
    using reference               = value_type&;
    using const_reference         = const value_type&;

    using iterator                = WuStringIterator<_scary_val>;
    using const_iterator          = WuStringConstIterator<_scary_val>;

    using reverse_iterator        = std::reverse_iterator<iterator>;
    using const_reverse_iterator  = std::reverse_iterator<const_iterator>;

private:
    static constexpr size_type BUFFER_SIZE          = _scary_val::BUFFER_SIZE;
    static constexpr size_type AllocMask            = _scary_val::AllocMask;
    static constexpr size_type SmallStringCapacity  = _scary_val::SmallStringCapacity;
    static constexpr size_type LeastAllocationSize  = SmallStringCapacity + 1 + 1;
    static constexpr size_t MemcpyValOffset         = std::_Size_after_ebco_v<std::_Container_base>;
    static constexpr size_t MemcpyValSize           = sizeof(_scary_val) - MemcpyValOffset;
    static constexpr bool CanMemcpyVal              = std::_Is_specialization_v<_traits, char_traits>&& std::is_trivial_v<pointer>;

    template <class IT>
    using _is_elem_cptr = std::bool_constant<std::_Is_any_of_v<IT, const _char_type* const, _char_type* const, const _char_type*, _char_type*>>;

public:

    constexpr WuBaseString() noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
        : m_pair(std::_Zero_then_variadic_args_t{})
    {
        construct_empty();
    }

    constexpr explicit WuBaseString(const _allocator& allocator) noexcept
        : m_pair(std::_One_then_variadic_args_t{}, allocator)
    {
        construct_empty();
    }

    constexpr WuBaseString(const WuBaseString& other)
        : m_pair(std::_One_then_variadic_args_t{}, alty_traits::select_on_container_copy_construction(other.get_allocator()))
    {
        construct<construct_strategy::from_string>(other.m_pair._Myval2.GetStorage(), other.m_pair._Myval2.MySize);
    }

    constexpr WuBaseString(WuBaseString&& other) noexcept
        : m_pair(std::_One_then_variadic_args_t{}, std::move(other.get_allocator()))
    {
        take_contents(other);
    }

    constexpr WuBaseString(const _char_type* ptr, _allocator& allocator = _allocator())
        : m_pair(std::_One_then_variadic_args_t{}, allocator)
    {
        ptr ? construct<construct_strategy::from_ptr>(ptr, std::_Convert_size<size_type>(_traits::length(ptr))) : construct_empty();
    }

    constexpr WuBaseString(_In_reads_(count) const _char_type* ptr, const size_type count, _allocator& allocator = _allocator())
        : m_pair(std::_One_then_variadic_args_t{}, allocator)
    {
        ptr || count > 0 ? construct<construct_strategy::from_ptr>(ptr, count) : construct_empty();
    }

    constexpr WuBaseString(const size_type length)
        : m_pair(std::_Zero_then_variadic_args_t{})
    {
        construct<construct_strategy::from_char>(_char_type(), length);
    }

    constexpr ~WuBaseString() noexcept { _Tidy_deallocate(); }

    _NODISCARD constexpr _char_type* Raw() noexcept { return m_pair._Myval2.GetStorage(); }
    _NODISCARD constexpr const _char_type* Raw() const noexcept { return m_pair._Myval2.GetStorage(); }

    constexpr void Clear() noexcept
    {
        m_pair._Myval2.MySize = 0;
        _traits::assign(m_pair._Myval2.GetStorage()[0], _char_type());
    }

    constexpr void SecureErase() noexcept
    {
        auto& data = m_pair._Myval2;
        _traits::assign(data.GetStorage(), data.MyCapacity, _char_type());
    }

    constexpr size_type Length() noexcept { return m_pair._Myval2.MySize; }

    constexpr const size_type Length() const noexcept { return m_pair._Myval2.MySize; }

    static constexpr bool IsNullOrEmpty(const _char_type* ptr)
    {
        if (ptr)
            return _traits::length(ptr) == 0;

        return true;
    }

    static constexpr bool IsNullOrEmpty(const WuBaseString& str)
    {
        return str.Length() == 0;
    }

    static constexpr bool IsNullOrWhiteSpace(const _char_type* ptr)
    {
        if (!ptr)
            return true;

        size_type char_count = _traits::length(ptr);
        return _traits::is_null_or_white_space(ptr, char_count);
    }

    static constexpr bool IsNullOrWhiteSpace(const WuBaseString& str)
    {
        auto& data = str.m_pair._Myval2;
        return _traits::is_null_or_white_space(data.GetStorage(), data.MySize);
    }

    template <class... TArgs>
    _NODISCARD static constexpr WuBaseString Format(const _char_type* fmt, TArgs&&... args)
    {
        const std::allocator<_char_type>& allocator{ };
        return WuBaseString(_traits::format(allocator, fmt, std::forward<TArgs>(args)...));
    }

    template <class... TArgs>
    _NODISCARD static constexpr WuBaseString Format(const WuBaseString& fmt, TArgs&&... args)
    {
        return WuBaseString(_traits::format(fmt.get_allocator(), fmt.Raw(), std::forward<TArgs>(args)...));
    }

    _NODISCARD constexpr WuBaseString Remove(const size_type index, size_type count)
    {
        const size_type mySize = m_pair._Myval2.MySize;
        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (count > mySize)
            throw "Count can't be greater than the string length.";

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (index < 0 || index + count > mySize - 1)
            throw "Index outside of string boundaries.";


        WuBaseString output(*this);

        count = output.m_pair._Myval2.ClampSuffixSize(index, count);
        const size_type old_size = output.m_pair._Myval2.MySize;
        _char_type* const my_ptr = output.m_pair._Myval2.GetStorage();
        _char_type* const erase_at = my_ptr + index;
        const size_type new_size = old_size - count;
        _traits::move(erase_at, erase_at + count, new_size - index + 1);
        output.m_pair._Myval2.MySize = new_size;

        return output;
    }

    _NODISCARD constexpr WuBaseString Remove(const size_type index)
    {
        const size_type mySize = m_pair._Myval2.MySize;
        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (index < 0 || index + 1 > mySize - 1)
            throw "Index outside of string boundaries.";

        WuBaseString output(*this);

        size_type count = 1;
        count = output.m_pair._Myval2.ClampSuffixSize(index, count);
        const size_type old_size = output.m_pair._Myval2.MySize;
        _char_type* const my_ptr = output.m_pair._Myval2.GetStorage();
        _char_type* const erase_at = my_ptr + index;
        const size_type new_size = old_size - count;
        _traits::move(erase_at, erase_at + count, new_size - index + 1);
        output.m_pair._Myval2.MySize = new_size;

        return output;
    }

    _NODISCARD constexpr WuBaseString Remove(const size_type index, size_type count) const
    {
        const size_type mySize = m_pair._Myval2.MySize;
        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (count > mySize)
            throw "Count can't be greater than the string length.";

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (index < 0 || index + count > mySize - 1)
            throw "Index outside of string boundaries.";


        WuBaseString output(*this);

        count = output.m_pair._Myval2.ClampSuffixSize(index, count);
        const size_type old_size = output.m_pair._Myval2.MySize;
        _char_type* const my_ptr = output.m_pair._Myval2.GetStorage();
        _char_type* const erase_at = my_ptr + index;
        const size_type new_size = old_size - count;
        _traits::move(erase_at, erase_at + count, new_size - index + 1);
        output.m_pair._Myval2.MySize = new_size;

        return output;
    }

    _NODISCARD constexpr WuBaseString Remove(const size_type index) const
    {
        const size_type mySize = m_pair._Myval2.MySize;
        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (index < 0 || index + 1 > mySize - 1)
            throw "Index outside of string boundaries.";

        WuBaseString output(*this);

        size_type count = 1;
        count = output.m_pair._Myval2.ClampSuffixSize(index, count);
        const size_type old_size = output.m_pair._Myval2.MySize;
        _char_type* const my_ptr = output.m_pair._Myval2.GetStorage();
        _char_type* const erase_at = my_ptr + index;
        const size_type new_size = old_size - count;
        _traits::move(erase_at, erase_at + count, new_size - index + 1);
        output.m_pair._Myval2.MySize = new_size;

        return output;
    }

    constexpr bool Contains(const _char_type t_char) const
    {
        auto& data = m_pair._Myval2;
        if (_traits::find(data.GetStorage(), data.MySize, t_char))
            return true;

        return false;
    }

    constexpr bool Contains(const _char_type* ptr) const
    {
        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (!ptr)
            throw "Input string cannot be null.";

        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        const _char_type* find = _traits::find_str(storage, ptr);
        if (find && find != storage)
            return true;

        return false;
    }

    constexpr bool Contains(const WuBaseString& str) const
    {
        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        const _char_type* find = _traits::find_str(storage, str.m_pair._Myval2.GetStorage());
        if (find && find != storage)
            return true;

        return false;
    }

    constexpr bool StartsWith(const _char_type t_char)
    {
        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        if (data.MySize == 0 || !storage)
            return false;

        return storage[0] == t_char;
    }

    constexpr const bool StartsWith(const _char_type t_char) const
    {
        auto& data = m_pair._Myval2;
        const pointer& storage = data.GetStorage();
        if (data.MySize == 0 || !storage)
            return false;

        return storage[0] == t_char;
    }

    constexpr bool StartsWith(const _char_type* str)
    {
        if (!str)
            return false;

        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        if (str == storage)
            return true;

        size_type str_length = _traits::length(str);
        if (str_length > Length())
            return false;

        for (size_type i = 0; i < str_length; i++) {
            if (storage[i] != str[i])
                return false;
        }

        return true;
    }

    constexpr const bool StartsWith(const _char_type* str) const
    {
        if (!str)
            return false;

        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        if (str == storage)
            return true;

        size_type str_length = _traits::length(str);
        if (str_length > Length())
            return false;

        for (size_type i = 0; i < str_length; i++) {
            if (storage[i] != str[i])
                return false;
        }

        return true;
    }

    constexpr bool StartsWith(const WuBaseString& str)
    {
        auto& str_data = str.m_pair._Myval2;
        const _char_type* str_storage = str_data.GetStorage();

        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        if (str_storage == storage)
            return true;

        size_type str_length = str.Length();
        if (str_length > Length())
            return false;

        for (size_type i = 0; i < str_length; i++) {
            if (storage[i] != str_storage[i])
                return false;
        }

        return true;
    }

    constexpr const bool StartsWith(const WuBaseString& str) const
    {
        auto& str_data = str.m_pair._Myval2;
        const _char_type* str_storage = str_data.GetStorage();

        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        if (str_storage == storage)
            return true;

        size_type str_length = str.Length();
        if (str_length > Length())
            return false;

        for (size_type i = 0; i < str_length; i++) {
            if (storage[i] != str_storage[i])
                return false;
        }

        return true;
    }

    constexpr bool EndsWith(const _char_type t_char) const
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const _char_type* storage = data.GetStorage();

        if (size == 0 || !storage)
            return false;

        // 0-based array plus /0.
        return storage[size - 1] == t_char;
    }

    constexpr bool EndsWith(const _char_type* ptr, bool ignore_case = false) const
    {
        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (!ptr)
            throw "Input string cannot be null.";

        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const _char_type* storage = data.GetStorage();
        if (size == 0 || !storage)
            return false;

        size_t input_len = _traits::length(ptr);
        if (input_len > size)
            return false;

        if (ignore_case)
            return _traits::compare_no_case(storage + size - input_len, ptr, input_len) == 0;

        return _traits::compare(storage + size - input_len, ptr, input_len) == 0;
    }

    constexpr bool EndsWith(const WuBaseString& str, bool ignore_case = false) const
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const _char_type* storage = data.GetStorage();

        size_t input_len = str.Length();
        const _char_type* str_storage = str.m_pair._Myval2.GetStorage();
        if (str.Length() > size)
            return false;

        if (size == 0 || !storage)
            return false;

        if (ignore_case)
            return _traits::compare_no_case(storage + size - input_len, str_storage, input_len) == 0;

        return _traits::compare(storage + size - input_len, str_storage, input_len) == 0;
    }

    // [needle_size] is necessary because _traits::length will return invalid lengths if the character
    // sequence contains '\0'.
    constexpr size_type IndexOfAny(const _char_type* needle, const size_type needle_size) noexcept
    {
        auto& data = m_pair._Myval2;
        return find_first_of(data.GetStorage(), data.MySize, 0, needle, needle_size);
    }

    constexpr size_type IndexOfAny(const _char_type* needle, const size_type needle_size, const size_type offset) noexcept
    {
        auto& data = m_pair._Myval2;
        return find_first_of(data.GetStorage(), data.MySize, offset, needle, needle_size);
    }

    constexpr size_type IndexOfAny(const WuBaseString& needle) noexcept
    {
        auto& data = m_pair._Myval2;
        auto& needleData = needle.m_pair._Myval2;
        return find_first_of(data.GetStorage(), data.MySize, 0, needleData.GetStorage(), needleData.MySize);
    }

    constexpr size_type IndexOfAny(const WuBaseString& needle, const size_type offset) noexcept
    {
        auto& data = m_pair._Myval2;
        auto& needleData = needle.m_pair._Myval2;
        return find_first_of(data.GetStorage(), data.MySize, offset, needleData.GetStorage(), needleData.MySize);
    }

    constexpr size_type IndexOfAny(const _char_type* needle, const size_type needle_size) const noexcept
    {
        auto& data = m_pair._Myval2;
        return find_first_of(data.GetStorage(), data.MySize, 0, needle, needle_size);
    }

    constexpr size_type IndexOfAny(const _char_type* needle, const size_type needle_size, const size_type offset) const noexcept
    {
        auto& data = m_pair._Myval2;
        return find_first_of(data.GetStorage(), data.MySize, offset, needle, needle_size);
    }

    constexpr size_type IndexOfAny(const WuBaseString& needle) const noexcept
    {
        auto& data = m_pair._Myval2;
        auto& needleData = needle.m_pair._Myval2;
        return find_first_of(data.GetStorage(), data.MySize, 0, needleData.GetStorage(), needleData.MySize);
    }

    constexpr size_type IndexOfAny(const WuBaseString& needle, const size_type offset) const noexcept
    {
        auto& data = m_pair._Myval2;
        auto& needleData = needle.m_pair._Myval2;
        return find_first_of(data.GetStorage(), data.MySize, offset, needleData.GetStorage(), needleData.MySize);
    }

    _NODISCARD constexpr WuBaseString Replace(const _char_type to_replace, const _char_type replace_with)
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const pointer& storage = data.GetStorage();

        WuList<_char_type> buffer(size);
        for (size_type i = 0; i < size; i++) {
            if (storage[i] == to_replace)
                buffer.Add(replace_with);
            else
                buffer.Add(storage[i]);
        }

        buffer.Add(_char_type());
        return WuBaseString(buffer.Data());
    }

    _NODISCARD constexpr WuBaseString Replace(const _char_type to_replace, const _char_type* replace_with)
    {
        if (!replace_with)
            return WuBaseString(*this);

        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const pointer& storage = data.GetStorage();

        WuList<_char_type> buffer(size);
        size_type replace_length = _traits::length(replace_with);
        for (size_type i = 0; i < size; i++) {
            if (storage[i] == to_replace) {
                for (size_type i = 0; i < replace_length; i++) {
                    buffer.Add(replace_with[i]);
                }
            }
            else
                buffer.Add(storage[i]);
        }

        buffer.Add(_char_type());

        return WuBaseString(buffer.Data());
    }

    _NODISCARD constexpr WuBaseString Replace(const _char_type to_replace, const WuBaseString& replace_with)
    {
        const size_type& other_size = replace_with.Length();
        if (other_size == 0)
            return WuBaseString(*this);

        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const pointer& storage = data.GetStorage();

        WuList<_char_type> buffer(size);
        for (size_type i = 0; i < size; i++) {
            if (storage[i] == to_replace) {
                for (size_type i = 0; i < other_size; i++) {
                    buffer.Add(replace_with[i]);
                }
            }
            else
                buffer.Add(storage[i]);
        }

        buffer.Add(_char_type());

        return WuBaseString(buffer.Data());
    }

    _NODISCARD constexpr WuBaseString Replace(const _char_type* to_replace, const _char_type* replace_with)
    {
        WuBaseString output(*this);
        if (!to_replace || !replace_with)
            return output;


        auto& out_data = output.m_pair._Myval2;
        const size_type& out_size = out_data.MySize;
        const pointer& out_storage = out_data.GetStorage();

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        WuList<_char_type> buffer(out_size);
        size_type to_replace_length = _traits::length(to_replace);
        size_type replace_with_length = _traits::length(replace_with);

        // Every time the string is found in the buffer we replace it with [replace_with],
        // until _traits::find_str returns null.
        do {
            found_index = _traits::find_str(out_storage, to_replace);
            if (!found_index)
                break;

            has_buffer = true;
            buffer.Clear();

            // Adding each character from the start of the buffer to the [found_index] offset.
            for (size_type i = 0; i < out_size; i++) {
                const pointer current_offset = out_storage + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {

                        // Same principle of before, but for every character from [replace_with].
                        for (size_type j = 0; j < replace_with_length; j++) {
                            buffer.Add(replace_with[j]);
                        }

                        already_inserted = true;
                    }
                }
                else
                    buffer.Add(output[i]);
            }

            buffer.Add(_char_type());
            already_inserted = false;
            output = WuBaseString(buffer.Data());

        } while (found_index != NULL);

        return output;
    }

    _NODISCARD constexpr WuBaseString Replace(const _char_type* to_replace, const WuBaseString& replace_with)
    {
        WuBaseString output(*this);
        size_type replace_with_length = replace_with.Length();
        if (!to_replace || replace_with_length == 0)
            return output;


        auto& out_data = output.m_pair._Myval2;
        const size_type& out_size = out_data.MySize;
        const pointer& out_storage = out_data.GetStorage();

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        WuList<_char_type> buffer(out_size);
        size_type to_replace_length = _traits::length(to_replace);

        // Every time the string is found in the buffer we replace it with [replace_with],
        // until _traits::find_str returns null.
        do {
            found_index = _traits::find_str(out_storage, to_replace);
            if (!found_index)
                break;

            has_buffer = true;
            buffer.Clear();

            // Adding each character from the start of the buffer to the [found_index] offset.
            for (size_type i = 0; i < out_size; i++) {
                const pointer current_offset = out_storage + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {

                        // Same principle of before, but for every character from [replace_with].
                        for (size_type j = 0; j < replace_with_length; j++) {
                            buffer.Add(replace_with[j]);
                        }

                        already_inserted = true;
                    }
                }
                else
                    buffer.Add(output[i]);
            }

            buffer.Add(_char_type());
            already_inserted = false;
            output = WuBaseString(buffer.Data());

        } while (found_index != NULL);

        return output;
    }

    _NODISCARD constexpr WuBaseString Replace(const WuBaseString& to_replace, const _char_type replace_with)
    {
        WuBaseString output(*this);
        size_type to_replace_length = to_replace.Length();
        if (to_replace_length == 0)
            return output;

        const _char_type* to_replace_storage = to_replace.m_pair._Myval2.GetStorage();

        auto& out_data = output.m_pair._Myval2;
        const size_type& out_size = out_data.MySize;
        const _char_type* out_storage = out_data.GetStorage();

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        WuList<_char_type> buffer(out_size);
        do {
            found_index = _traits::find_str(out_storage, to_replace_storage);
            if (!found_index)
                break;

            has_buffer = true;
            buffer.Clear();
            for (size_type i = 0; i < out_size; i++) {
                const _char_type* current_offset = out_storage + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {
                        buffer.Add(replace_with);
                        already_inserted = true;
                    }
                }
                else
                    buffer.Add(output[i]);
            }

            buffer.Add(_char_type());
            already_inserted = false;
            output = WuBaseString(buffer.Data());

        } while (found_index != NULL);

        return output;
    }

    _NODISCARD constexpr WuBaseString Replace(const WuBaseString& to_replace, const _char_type* replace_with)
    {
        WuBaseString output(*this);
        size_type to_replace_length = to_replace.Length();
        if (to_replace_length == 0 || !replace_with)
            return output;

        const _char_type* to_replace_storage = to_replace.m_pair._Myval2.GetStorage();

        auto& out_data = output.m_pair._Myval2;
        const size_type& out_size = out_data.MySize;
        const _char_type* out_storage = out_data.GetStorage();

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        WuList<_char_type> buffer(out_size);
        size_type replace_with_length = _traits::length(replace_with);
        do {
            found_index = _traits::find_str(out_storage, to_replace_storage);
            if (!found_index)
                break;

            has_buffer = true;
            buffer.Clear();
            for (size_t i = 0; i < out_size; i++) {
                const _char_type* current_offset = out_storage + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {
                        for (size_t j = 0; j < replace_with_length; j++) {
                            buffer.Add(replace_with[j]);
                        }
                        already_inserted = true;
                    }
                }
                else
                    buffer.Add(output[i]);
            }

            buffer.Add(_char_type());
            already_inserted = false;
            output = WuBaseString(buffer.Data());

        } while (found_index != NULL);

        return output;
    }

    _NODISCARD constexpr WuBaseString Replace(const WuBaseString& to_replace, const WuBaseString& replace_with)
    {
        WuBaseString output(*this);
        size_type to_replace_length = to_replace.Length();
        size_type replace_with_length = replace_with.Length();
        if (to_replace_length == 0 || replace_with_length == 0)
            return output;

        const _char_type* to_replace_storage = to_replace.m_pair._Myval2.GetStorage();
        const _char_type* replace_with_storage = replace_with.m_pair._Myval2.GetStorage();

        auto& out_data = output.m_pair._Myval2;
        const size_type& out_size = out_data.MySize;
        const _char_type* out_storage = out_data.GetStorage();

        bool has_buffer = false;
        bool already_inserted = false;
        const _char_type* found_index;
        WuList<_char_type> buffer(out_size);
        do {
            found_index = _traits::find_str(out_storage, to_replace_storage);
            if (!found_index)
                break;

            has_buffer = true;
            buffer.Clear();
            for (size_type i = 0; i < out_size; i++) {
                const _char_type* current_offset = out_storage + i + 1;
                if (current_offset > found_index && current_offset <= found_index + to_replace_length) {
                    if (!already_inserted) {
                        for (size_type j = 0; j < replace_with_length; j++) {
                            buffer.Add(replace_with[j]);
                        }
                        already_inserted = true;
                    }
                }
                else
                    buffer.Add(output[i]);
            }

            buffer.Add(_char_type());
            already_inserted = false;
            output = WuBaseString(buffer.Data());

        } while (found_index != NULL);

        return output;
    }

    _NODISCARD constexpr WuList<WuBaseString> Split(const _char_type split_on)
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const pointer& storage = data.GetStorage();

        _char_type null_char = _char_type();
        WuList<WuBaseString> output;
        WuList<_char_type> buffer(size);

        // Adds every character to a buffer until the char is found.
        // Then pushes the string to the vector, and cleans the buffer.
        for (size_type i = 0; i < size; i++) {
            if (storage[i] == split_on) {
                if (buffer.Count()) {
                    buffer.Add(null_char);
                    output.Add(buffer.Data());
                    buffer.Clear();
                }
            }
            else {
                buffer.Add(storage[i]);
            }
        }

        // If there are characters after the last occurrence, they will be in the buffer.
        if (buffer.Count()) {
            buffer.Add(null_char);
            output.Add(buffer.Data());
        }

        return output;
    }

    _NODISCARD constexpr const WuList<WuBaseString> Split(const _char_type split_on) const
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const _char_type* storage = data.GetStorage();

        _char_type null_char = _char_type();
        WuList<WuBaseString> output;
        WuList<_char_type> buffer(size);

        // Adds every character to a buffer until the char is found.
        // Then pushes the string to the vector, and cleans the buffer.
        for (size_type i = 0; i < size; i++) {
            if (storage[i] == split_on) {
                if (buffer.Count()) {
                    buffer.Add(null_char);
                    output.Add(buffer.Data());
                    buffer.Clear();
                }
            }
            else {
                buffer.Add(storage[i]);
            }
        }

        // If there are characters after the last occurrence, they will be in the buffer.
        if (buffer.Count()) {
            buffer.Add(null_char);
            output.Add(buffer.Data());
        }

        return output;
    }

    _NODISCARD constexpr WuList<WuBaseString> Split(const _char_type* split_on)
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const pointer& storage = data.GetStorage();

        WuList<WuBaseString> output;
        _char_type null_char = _char_type();
        if (!split_on) {
            output.Add(*this);
            return output;
        }

        const _char_type* found_offset;
        WuList<_char_type> buffer(size);
        pointer current_offset = storage;
        size_type split_on_length = _traits::length(split_on);

        // Going through each occurrence. For each one we advance the
        // [current_offset] to the occurrence offset, plus its length.
        do {
            found_offset = _traits::find_str(current_offset, split_on);
            if (found_offset) {

                // Pushing every character until we reach [found_offset].
                buffer.Clear();
                while (current_offset < found_offset) {
                    buffer.Add(current_offset[0]);
                    current_offset++;
                }

                // If [found_offset] is at the beginning of the string, and
                // we don't check if the buffer is empty, we create an empty
                // string entry in the output.
                if (buffer.Count()) {
                    buffer.Add(null_char);
                    output.Add(buffer.Data());
                }

                current_offset += split_on_length;
                if (current_offset >= storage + size)
                    break;
            }
            else {
                // Checking if we are at the end of the string.
                if (current_offset >= storage + size)
                    break;

                // Adding each char until the end of the string.
                buffer.Clear();
                while (current_offset < storage + size) {
                    buffer.Add(current_offset[0]);
                    current_offset++;
                }

                buffer.Add(null_char);
                output.Add(buffer.Data());

                break;
            }

        } while (found_offset != NULL);

        return output;
    }

    _NODISCARD constexpr const WuList<WuBaseString> Split(const _char_type* split_on) const
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const pointer& storage = data.GetStorage();

        WuList<WuBaseString> output;
        _char_type null_char = _char_type();
        if (!split_on) {
            output.Add(*this);
            return output;
        }

        const _char_type* found_offset;
        WuList<_char_type> buffer(size);
        pointer current_offset = storage;
        size_type split_on_length = _traits::length(split_on);

        // Going through each occurrence. For each one we advance the
        // [current_offset] to the occurrence offset, plus its length.
        do {
            found_offset = _traits::find_str(current_offset, split_on);
            if (found_offset) {

                // Pushing every character until we reach [found_offset].
                buffer.Clear();
                while (current_offset < found_offset) {
                    buffer.Add(current_offset[0]);
                    current_offset++;
                }

                // If [found_offset] is at the beginning of the string, and
                // we don't check if the buffer is empty, we create an empty
                // string entry in the output.
                if (buffer.Count()) {
                    buffer.Add(null_char);
                    output.Add(buffer.Data());
                }

                current_offset += split_on_length;
                if (current_offset >= storage + size)
                    break;
            }
            else {
                // Checking if we are at the end of the string.
                if (current_offset >= storage + size)
                    break;

                // Adding each char until the end of the string.
                buffer.Clear();
                while (current_offset < storage + size) {
                    buffer.Add(current_offset[0]);
                    current_offset++;
                }

                buffer.Add(null_char);
                output.Add(buffer.Data());

                break;
            }

        } while (found_offset != NULL);

        return output;
    }

    _NODISCARD constexpr WuList<WuBaseString> Split(const WuBaseString& split_on)
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const pointer& storage = data.GetStorage();

        WuList<WuBaseString> output;
        _char_type null_char = _char_type();
        size_type split_on_length = split_on.Length();
        if (!split_on_length) {
            output.Add(*this);
            return output;
        }

        const _char_type* so_storage = split_on.m_pair._Myval2.GetStorage();

        const _char_type* found_offset;
        WuList<_char_type> buffer;
        pointer current_offset = storage;

        // Going through each occurrence. For each one we advance the
        // [current_offset] to the occurrence offset, plus its length.
        do {
            found_offset = _traits::find_str(current_offset, so_storage);
            if (found_offset != NULL) {

                // Pushing every character until we reach [found_offset].
                buffer.Clear();
                while (current_offset < found_offset) {
                    buffer.Add(current_offset[0]);
                    current_offset++;
                }

                // If [found_offset] is at the beginning of the string, and
                // we don't check if the buffer is empty, we create an empty
                // string entry in the output.
                if (buffer.Count()) {
                    buffer.Add(null_char);
                    output.Add(buffer.Data());
                }

                current_offset += split_on_length;
                if (current_offset >= storage + size)
                    break;
            }
            else {
                // Checking if we are at the end of the string.
                if (current_offset >= storage + size)
                    break;

                // Adding each char until the end of the string.
                buffer.Clear();
                while (current_offset < storage + size) {
                    buffer.Add(current_offset[0]);
                    current_offset++;
                }

                buffer.Add(null_char);
                output.Add(buffer.Data());

                break;
            }

        } while (found_offset != NULL);

        return output;
    }

    _NODISCARD constexpr const WuList<WuBaseString> Split(const WuBaseString& split_on) const
    {
        auto& data = m_pair._Myval2;
        const size_type& size = data.MySize;
        const pointer& storage = data.GetStorage();

        WuList<WuBaseString> output;
        _char_type null_char = _char_type();
        size_type split_on_length = split_on.Length();
        if (!split_on_length) {
            output.Add(*this);
            return output;
        }

        const pointer& so_storage = split_on.m_pair._Myval2.GetStorage();

        const _char_type* found_offset;
        WuList<_char_type> buffer;
        pointer current_offset = storage;

        // Going through each occurrence. For each one we advance the
        // [current_offset] to the occurrence offset, plus its length.
        do {
            found_offset = _traits::find_str(current_offset, so_storage);
            if (found_offset != NULL) {

                // Pushing every character until we reach [found_offset].
                buffer.Clear();
                while (current_offset < found_offset) {
                    buffer.Add(current_offset[0]);
                    current_offset++;
                }

                // If [found_offset] is at the beginning of the string, and
                // we don't check if the buffer is empty, we create an empty
                // string entry in the output.
                if (buffer.Count()) {
                    buffer.Add(null_char);
                    output.Add(buffer.Data());
                }

                current_offset += split_on_length;
                if (current_offset >= storage + size)
                    break;
            }
            else {
                // Checking if we are at the end of the string.
                if (current_offset >= storage + size)
                    break;

                // Adding each char until the end of the string.
                buffer.Clear();
                while (current_offset < storage + size) {
                    buffer.Add(current_offset[0]);
                    current_offset++;
                }

                buffer.Add(null_char);
                output.Add(buffer.Data());

                break;
            }

        } while (found_offset != NULL);

        return output;
    }

    constexpr const int CompareTo(const _char_type* other, bool ignoreCase = false)
    {
        if (!other)
            return -1;

        auto& data = m_pair._Myval2;
        const pointer& storage = data.GetStorage();

        const size_type& our_length = Length();
        const size_type other_len = _traits::length(other);
        const size_type biggest = our_length > other_len ? our_length : other_len;

        return ignoreCase ? _traits::compare_no_case(storage, other, biggest)
            : _traits::compare(storage, other, biggest);
    }

    constexpr const int CompareTo(const _char_type* other, bool ignoreCase = false) const
    {
        if (!other)
            return -1;

        auto& data = m_pair._Myval2;
        const pointer& storage = data.GetStorage();

        const size_type& our_length = Length();
        const size_type other_len = _traits::length(other);
        const size_type biggest = our_length > other_len ? our_length : other_len;

        return ignoreCase ? _traits::compare_no_case(storage, other, biggest)
            : _traits::compare(storage, other, biggest);
    }

    constexpr const int CompareTo(const WuBaseString& other, bool ignoreCase = false)
    {
        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        const _char_type* other_storage = other.m_pair._Myval2.GetStorage();

        const size_type& our_length = Length();
        const size_type other_len = other.Length();
        const size_type biggest = our_length > other_len ? our_length : other_len;

        return ignoreCase ? _traits::compare_no_case(storage, other_storage, biggest)
            : _traits::compare(storage, other_storage, biggest);
    }

    /*
    *   !!REPEATED CODE!!FIXDAT!!
    */
    constexpr const int CompareTo(const WuBaseString& other, bool ignoreCase = false) const
    {
        auto& data = m_pair._Myval2;
        const _char_type* storage = data.GetStorage();
        const _char_type* other_storage = other.m_pair._Myval2.GetStorage();

        const size_type& our_length = Length();
        const size_type other_len = other.Length();
        const size_type biggest = our_length > other_len ? our_length : other_len;

        return ignoreCase ? _traits::compare_no_case(storage, other_storage, biggest)
            : _traits::compare(storage, other_storage, biggest);
    }

    constexpr void ToUpper()
    {
        _traits::to_upper(m_pair._Myval2.GetStorage(), Length() + 1);
    }

    constexpr void ToLower()
    {
        _traits::to_lower(m_pair._Myval2.GetStorage(), Length() + 1);
    }

    constexpr WuBaseString& Trim()
    {
        trim();

        return *this;
    }

    constexpr WuBaseString& TrimStart()
    {
        trim(TrimType::Head);

        return *this;
    }

    constexpr WuBaseString& TrimEnd()
    {
        trim(TrimType::Tail);

        return *this;
    }

    _NODISCARD constexpr WuBaseString<wchar_t, char_traits<wchar_t>> ToWide(const uint32_t code_page = CP_ACP)
    {
        return WuBaseString<wchar_t, char_traits<wchar_t>>(_traits::to_wide(get_allocator(), m_pair._Myval2.GetStorage(), Length(), code_page));
    }

    _NODISCARD constexpr const WuBaseString<wchar_t, char_traits<wchar_t>> ToWide(const uint32_t code_page = CP_ACP) const
    {
        return WuBaseString<wchar_t, char_traits<wchar_t>>(_traits::to_wide(get_allocator(), m_pair._Myval2.GetStorage(), Length(), code_page));
    }

    _NODISCARD static constexpr WuBaseString<wchar_t, char_traits<wchar_t>> ToWide(const _char_type* str, const uint32_t code_page = CP_ACP)
    {
        std::allocator<char> allocator;
        const size_type len = strlen(str);
        return WuBaseString<wchar_t, char_traits<wchar_t>>(_traits::to_wide(allocator, str, len, code_page));
    }

    _NODISCARD constexpr WuBaseString<char, char_traits<char>> ToNarrow()
    {
        return WuBaseString<char, char_traits<char>>(_traits::to_narrow(get_allocator(), m_pair._Myval2.GetStorage(), Length()));
    }

    _NODISCARD constexpr const WuBaseString<char, char_traits<char>> ToNarrow() const
    {
        return WuBaseString<char, char_traits<char>>(_traits::to_narrow(get_allocator(), m_pair._Myval2.GetStorage(), Length()));
    }

    _NODISCARD static constexpr WuBaseString<char, char_traits<char>> ToNarrow(const _char_type* str)
    {
        std::allocator<wchar_t> allocator;
        const size_type len = wcslen(str);
        return WuBaseString<char, char_traits<char>>(_traits::to_narrow(allocator, str, len));
    }

    _NODISCARD constexpr WuBaseString<char, char_traits<char>> ToMb(const uint32_t code_page, const char* default_char = nullptr)
    {
        const char* our_def_char = code_page == CP_UTF7 || code_page == CP_UTF8 ? nullptr : default_char;
        return WuBaseString<char, char_traits<char>>(_traits::to_mb(get_allocator(), m_pair._Myval2.GetStorage(), Length(), code_page, our_def_char));
    }

    _NODISCARD constexpr const WuBaseString<char, char_traits<char>> ToMb(const uint32_t code_page, const char* default_char = nullptr) const
    {
        const char* our_def_char = code_page == CP_UTF7 || code_page == CP_UTF8 ? nullptr : default_char;
        return WuBaseString<char, char_traits<char>>(_traits::to_mb(get_allocator(), m_pair._Myval2.GetStorage(), Length(), code_page, our_def_char));
    }

    _NODISCARD static constexpr WuBaseString<char, char_traits<char>> ToMb(const _char_type* str, const uint32_t code_page, const char* default_char = nullptr)
    {
        std::allocator<wchar_t> allocator;
        const size_type len = wcslen(str);
        const char* our_def_char = code_page == CP_UTF7 || code_page == CP_UTF8 ? nullptr : default_char;
        return WuBaseString<char, char_traits<char>>(_traits::to_mb(allocator, str, len, code_page, our_def_char));
    }

    _NODISCARD constexpr friend WuBaseString operator+(const WuBaseString& left, const _char_type* right)
    {
        const auto left_size = left.Length();
        const auto right_size = _traits::length(right);

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (left.max_size() - left_size < right_size)
            throw;

        return { std::_String_constructor_concat_tag{}, left, left.Raw(), left_size, right, right_size };
    }

    _NODISCARD constexpr friend WuBaseString operator+(const WuBaseString& left, const WuBaseString& right)
    {
        const auto left_size = left.Length();
        const auto right_size = right.Length();

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (left.max_size() - left_size < right_size)
            throw;

        return { std::_String_constructor_concat_tag{}, left, left.Raw(), left_size, right.Raw(), right_size };
    }

    template<is_addition_supported T>
    _NODISCARD constexpr friend WuBaseString operator+(const WuBaseString& left, const T& right)
    {
        const _char_type* obj_string = _traits::to_string(left.get_allocator(), right);

        const auto left_size = left.Length();
        const auto right_size = _traits::length(obj_string);

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (left.max_size() - left_size < right_size)
            throw;

        return { std::_String_constructor_concat_tag{}, left, left.Raw(), left_size, obj_string, right_size };
    }

    _NODISCARD constexpr _char_type operator[](const size_t index)
    {
        auto& data = m_pair._Myval2;

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (index < 0 && index > data.MySize)
            throw "Index outside the boundaries of this string.";

        return data.GetStorage()[index];
    }

    _NODISCARD constexpr const _char_type operator[](const size_t index) const
    {
        auto& data = m_pair._Myval2;

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (index < 0 && index > data.MySize)
            throw "Index outside the boundaries of this string.";

        return data.GetStorage()[index];
    }

    _NODISCARD constexpr iterator begin() noexcept
    {
        return iterator(std::_Refancy<const _char_type*>(m_pair._Myval2.GetStorage()), std::addressof(m_pair._Myval2));
    }

    _NODISCARD constexpr const const_iterator begin() const noexcept
    {
        return const_iterator(std::_Refancy<const _char_type*>(m_pair._Myval2.GetStorage()), std::addressof(m_pair._Myval2));
    }

    _NODISCARD constexpr iterator end() noexcept
    {
        return iterator(std::_Refancy<const _char_type*>(m_pair._Myval2.GetStorage()) + static_cast<difference_type>(m_pair._Myval2.MySize),
            std::addressof(m_pair._Myval2));
    }

    _NODISCARD constexpr const const_iterator end() const noexcept
    {
        return const_iterator(std::_Refancy<const _char_type*>(m_pair._Myval2.GetStorage()) + static_cast<difference_type>(m_pair._Myval2.MySize),
            std::addressof(m_pair._Myval2));
    }

    _NODISCARD constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    _NODISCARD constexpr const const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    _NODISCARD constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    _NODISCARD constexpr const const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    _NODISCARD constexpr const_iterator cbegin() const noexcept { return begin(); }
    _NODISCARD constexpr const_iterator cend() const noexcept { return end(); }
    _NODISCARD constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    _NODISCARD constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    constexpr WuBaseString& operator=(const _char_type* other)
    {
        if (!other) {
            construct_empty();
            return *this;
        }

        const size_type other_len = _traits::length(other);
        return assign(other, other_len);
    }

    constexpr WuBaseString& operator=(const WuBaseString& right)
    {
        if (this == std::addressof(right))
            return *this;

        auto& allocator = get_allocator();
        const auto& right_al = right.get_allocator();
        if constexpr (std::_Choose_pocca_v<allocator_type>) {
            if (allocator != right_al) {
                const size_type right_size = right.m_pair._Myval2.MySize;
                const _char_type* const right_ptr = right.m_pair._Myval2.GetStorage();
                if (right_size > SmallStringCapacity) {
                    size_type new_capacity = calculate_growth(right_size, SmallStringCapacity, right.max_size());
                    auto right_al_non_const = right_al;
                    const pointer new_ptr = allocate_for_capacity(right_al_non_const, new_capacity);
                    _traits::copy(std::_Unfancy(new_ptr), right_ptr, right_size + 1);

                    _Tidy_deallocate();
                    construct_in_place(m_pair._Myval2.Storage.Pointer, new_ptr);
                    m_pair._Myval2.MySize = right_size;
                    m_pair._Myval2.MyCapacity = new_capacity;
                }
                else {
                    _Tidy_deallocate();
                    _traits::copy(m_pair._Myval2.Storage.Buffer, right_ptr, right_size + 1);
                    m_pair._Myval2.MySize = right_size;
                    m_pair._Myval2.MyCapacity = SmallStringCapacity;
                }

                std::_Pocca(allocator, right_al);

                return *this;
            }
        }

        std::_Pocca(allocator, right_al);
        assign(right.m_pair._Myval2.GetStorage(), right.m_pair._Myval2.MySize);

        return *this;
    }

    constexpr WuBaseString& operator+=(const _char_type* other)
    {
        *this = *this + other;

        return *this;
    }

    constexpr WuBaseString& operator+=(const WuBaseString& other)
    {
        *this = *this + other;

        return *this;
    }

    template<is_addition_supported T>
    constexpr WuBaseString& operator+=(const T& object)
    {
        *this = *this + object;

        return *this;
    }

    _NODISCARD constexpr bool operator==(const _char_type* right) const
    {
        if (right == nullptr)
            return false;

        const size_type& size = m_pair._Myval2.MySize;
        const size_type right_len = _traits::length(right);
        const size_type biggest = size > right_len ? size : right_len;

        return _traits::compare(m_pair._Myval2.GetStorage(), right, biggest) == 0;
    }

    _NODISCARD constexpr bool operator==(const WuBaseString& right) const
    {
        const size_type& size = m_pair._Myval2.MySize;
        const size_type right_len = right.Length();
        const size_type biggest = size > right_len ? size : right_len;

        return _traits::compare(m_pair._Myval2.GetStorage(), right.m_pair._Myval2.GetStorage(), biggest) == 0;
    }

    _NODISCARD constexpr bool operator!=(const _char_type* right) const
    {
        return !(*this == right);
    }

    _NODISCARD constexpr bool operator!=(const WuBaseString& right) const
    {
        return !(*this == right);
    }

    _NODISCARD constexpr bool operator<(const _char_type* right) const
    {
        if (right == nullptr)
            return false;

        const size_type& size = m_pair._Myval2.MySize;
        const size_type right_len = _traits::length(right);
        const size_type biggest = size > right_len ? size : right_len;

        return _traits::compare(m_pair._Myval2.GetStorage(), right, biggest) < 0;
    }

    _NODISCARD constexpr bool operator<(const WuBaseString& right) const
    {
        const size_type& size = m_pair._Myval2.MySize;
        const size_type right_len = right.Length();
        const size_type biggest = size > right_len ? size : right_len;

        return _traits::compare(m_pair._Myval2.GetStorage(), right.m_pair._Myval2.GetStorage(), biggest) < 0;
    }

    _NODISCARD constexpr bool operator>(const _char_type* right) const
    {
        return !(*this < right);
    }

    _NODISCARD constexpr bool operator>(const WuBaseString& right) const
    {
        return !(*this < right);
    }

    _NODISCARD constexpr bool operator<=(const _char_type* right) const
    {
        if (right == nullptr)
            return false;

        const size_type& size = m_pair._Myval2.MySize;
        const size_type right_len = _traits::length(right);
        const size_type biggest = size > right_len ? size : right_len;

        return _traits::compare(m_pair._Myval2.GetStorage(), right, biggest) <= 0;
    }

    _NODISCARD constexpr bool operator<=(const WuBaseString& right) const
    {
        const size_type& size = m_pair._Myval2.MySize;
        const size_type right_len = right.Length();
        const size_type biggest = size > right_len ? size : right_len;

        return _traits::compare(m_pair._Myval2.GetStorage(), right.m_pair._Myval2.GetStorage(), biggest) <= 0;
    }

    _NODISCARD constexpr bool operator>=(const _char_type* right) const
    {
        if (right == nullptr)
            return false;

        const size_type& size = m_pair._Myval2.MySize;
        const size_type right_len = _traits::length(right);
        const size_type biggest = size > right_len ? size : right_len;

        return _traits::compare(m_pair._Myval2.GetStorage(), right, biggest) >= 0;
    }

    _NODISCARD constexpr bool operator>=(const WuBaseString& right) const
    {
        const size_type& size = m_pair._Myval2.MySize;
        const size_type right_len = right.Length();
        const size_type biggest = size > right_len ? size : right_len;

        return _traits::compare(m_pair._Myval2.GetStorage(), right.m_pair._Myval2.GetStorage(), biggest) >= 0;
    }

private:
    std::_Compressed_pair<_allocator, _scary_val> m_pair;

    constexpr WuBaseString(std::_String_constructor_concat_tag, const WuBaseString& al_source,
        const _char_type* const left_ptr, const size_type left_size, const _char_type* const right_ptr, const size_type right_size)
        : m_pair(std::_One_then_variadic_args_t{}, alty_traits::select_on_container_copy_construction(al_source.get_allocator()))
    {
        const auto new_size = static_cast<size_type>(left_size + right_size);
        size_type new_capacity = SmallStringCapacity;
        auto& my_data = m_pair._Myval2;
        _char_type* ptr = my_data.Storage.Buffer;

        if (new_capacity < new_size) {
            new_capacity = calculate_growth(new_size, SmallStringCapacity, max_size());
            const pointer fancy_ptr = allocate_for_capacity(get_allocator(), new_capacity);
            ptr = std::_Unfancy(fancy_ptr);
            construct_in_place(my_data.Storage.Pointer, fancy_ptr);
        }

        my_data.MySize = new_size;
        my_data.MyCapacity = new_capacity;
        _traits::copy(ptr, left_ptr, left_size);
        _traits::copy(ptr + static_cast<ptrdiff_t>(left_size), right_ptr, right_size);
        _traits::assign(ptr[new_size], _char_type());
    }

    constexpr WuBaseString(std::_String_constructor_concat_tag, WuBaseString& left, WuBaseString& right)
        : m_pair(std::_One_then_variadic_args_t{}, left.get_allocator())
    {
        auto& my_data = m_pair._Myval2;
        auto& left_data = left.m_pair._Myval2;
        auto& right_data = right.m_pair._Myval2;
        left_data._Orphan_all();
        right_data._Orphan_all();
        const auto left_size = left_data.MySize;
        const auto right_size = right_data.MySize;

        const auto left_capacity = left_data.MyCapacity;
        const auto right_capacity = right_data.MyCapacity;
        const auto new_size = static_cast<size_type>(left_size + right_size);
        const bool fits_in_left = right_size <= left_capacity - left_size;
        if (fits_in_left && right_capacity <= left_capacity) {
            take_contents(left);
            const auto ptr = my_data.GetStorage();
            _traits::copy(ptr + left_size, right_data.GetStorage(), right_size + 1);
            my_data.MySize = new_size;

            return;
        }

        const bool fits_in_right = left_size <= right_capacity - right_size;
        if (std::_Allocators_equal(get_allocator(), right.get_allocator()) && fits_in_right) {
            take_contents(right);
            const auto ptr = std::_Unfancy(my_data.Storage.Pointer);
            _traits::move(ptr + left_size, ptr, right_size + 1);
            _traits::copy(ptr, left_data.GetStorage(), left_size);
            my_data.MySize = new_size;

            return;
        }

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        const auto max = max_size();
        if (max - left_size < right_size)
            throw;

        size_type new_capacity = calculate_growth(new_size, SmallStringCapacity, max);
        const pointer fancy_ptr = allocate_for_capacity(get_allocator(), new_capacity);
        construct_in_place(my_data.Storage.Pointer, fancy_ptr);
        my_data.MySize = new_size;
        my_data.MyCapacity = new_capacity;
        const auto ptr = std::_Unfancy(fancy_ptr);
        _traits::copy(ptr, left_data.GetStorage(), left_size);
        _traits::copy(ptr + left_size, right_data.GetStorage(), right_size + 1);
    }

    constexpr WuBaseString& assign(_In_reads_(count) const _char_type* ptr, const size_type count)
    {
        if (count <= m_pair._Myval2.MyCapacity) {
            _char_type* const old_ptr = m_pair._Myval2.GetStorage();
            m_pair._Myval2.MySize = count;
            _traits::move(old_ptr, ptr, count);
            _traits::assign(old_ptr[count], _char_type());

            return *this;
        }

        return reallocate_for(
            count,
            [](_char_type* const new_ptr, const size_type count, const _char_type* const ptr) _STATIC_CALL_OPERATOR{
                _traits::copy(new_ptr, ptr, count);
                _traits::assign(new_ptr[count], _char_type());
            }, ptr
        );
    }

    constexpr void trim(const TrimType type = TrimType::Both)
    {
        auto& allocator = get_allocator();
        auto& data = m_pair._Myval2;
        _char_type* storage = data.GetStorage();
        size_type& size = data.MySize;


        if (size < 1)
            return;

        size_t start_index = 0;
        size_t end_index = size - 1;

        if ((type & TrimType::Head) == TrimType::Head) {
            while (start_index < size && _traits::is_null_or_white_space(storage[start_index]))
                start_index++;
        }

        if ((type & TrimType::Tail) == TrimType::Tail) {
            while (end_index >= start_index && _traits::is_null_or_white_space(storage[end_index]))
                end_index--;
        }

        size_t new_len = end_index - start_index + 1;
        if (new_len != size) {
            if (new_len != 0) {
                _traits::move(storage, storage + start_index, new_len);
                storage[new_len] = _char_type();
                size = new_len;

                return;
            }

            storage[0] = _char_type();
            size = 0;

            return;
        }
    }

    template <bool Special = std::_Is_specialization_v<_traits, char_traits>>
    constexpr size_type find_first_of(const _char_type* haystack, const size_type hay_size, const size_type start_at, const _char_type* needle, const size_type needle_size) noexcept
    {
        if (needle_size != 0 && start_at < hay_size) {
            if constexpr (Special) {
                WuStringBitmap<typename _traits::char_type> matches;
                if (!matches.Mark(needle, needle + needle_size)) {
                    return find_first_of<false>(haystack, hay_size, start_at, needle, needle_size);
                }

                const auto end = haystack + hay_size;
                for (auto match_try = haystack + start_at; match_try < end; ++match_try) {
                    if (matches.Match(*match_try)) {
                        return static_cast<size_type>(match_try - haystack);
                    }
                }
            }
            else {
                const auto end = haystack + hay_size;
                for (auto match_try = haystack + start_at; match_try < end; ++match_try) {
                    if (_traits::find(needle, needle_size, *match_try)) {
                        return static_cast<size_type>(match_try - haystack);
                    }
                }
            }
        }

        return static_cast<size_type>(-1);
    }

    template <bool Special = std::_Is_specialization_v<_traits, char_traits>>
    constexpr size_type find_first_of(const _char_type* haystack, const size_type hay_size, const size_type start_at, const _char_type* needle, const size_type needle_size) const noexcept
    {
        if (needle_size != 0 && start_at < hay_size) {
            if constexpr (Special) {
                WuStringBitmap<typename _traits::char_type> matches;
                if (!matches.Mark(needle, needle + needle_size)) {
                    return find_first_of<false>(haystack, hay_size, start_at, needle, needle_size);
                }

                const auto end = haystack + hay_size;
                for (auto match_try = haystack + start_at; match_try < end; ++match_try) {
                    if (matches.Match(*match_try)) {
                        return static_cast<size_type>(match_try - haystack);
                    }
                }
            }
            else {
                const auto end = haystack + hay_size;
                for (auto match_try = haystack + start_at; match_try < end; ++match_try) {
                    if (_traits::find(needle, needle_size, *match_try)) {
                        return static_cast<size_type>(match_try - haystack);
                    }
                }
            }
        }

        return static_cast<size_type>(-1);
    }

    constexpr void construct_empty()
    {
        auto& data = m_pair._Myval2;

        data.MySize = 0;
        data.MyCapacity = SmallStringCapacity;
        data.ActivateSmallStringBuffer();

        _traits::assign(data.Storage.Buffer[0], _char_type());
    }

    enum class construct_strategy : uint8_t { from_char, from_ptr, from_string };

    template <construct_strategy strategy, class char_or_ptr>
    constexpr void construct(const char_or_ptr arg, const size_type count)
    {
        auto& data = m_pair._Myval2;
        _STL_INTERNAL_CHECK(!data.LargeModeEngaged());

        if constexpr (strategy == construct_strategy::from_char) {
            _STL_INTERNAL_STATIC_ASSERT(std::is_same_v<char_or_ptr, _char_type>);
        }
        else {
            _STL_INTERNAL_STATIC_ASSERT(_is_elem_cptr<char_or_ptr>::value);
        }

        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (count > max_size())
            throw;


        auto& allocator = get_allocator();
        if (count <= SmallStringCapacity) {
            data.MySize = count;
            data.MyCapacity = SmallStringCapacity;

            if constexpr (strategy == construct_strategy::from_char) {
                _traits::assign(data.Storage.Buffer, count, arg);
                _traits::assign(data.Storage.Buffer[count], _char_type());
            }
            else if constexpr (strategy == construct_strategy::from_ptr) {
                _traits::copy(data.Storage.Buffer, arg, count);
                _traits::assign(data.Storage.Buffer[count], _char_type());
            }
            else {
                _traits::copy(data.Storage.Buffer, arg, BUFFER_SIZE);
            }

            return;
        }

        size_type new_capacity = calculate_growth(count, SmallStringCapacity, max_size());
        const pointer new_ptr = allocate_for_capacity(allocator, new_capacity);
        construct_in_place(data.Storage.Pointer, new_ptr);

        data.MySize = count;
        data.MyCapacity = new_capacity;
        if constexpr (strategy == construct_strategy::from_char) {
            _traits::assign(std::_Unfancy(new_ptr), count, arg);
            _traits::assign(std::_Unfancy(new_ptr)[count], _char_type());
        }
        else if constexpr (strategy == construct_strategy::from_ptr) {
            _traits::copy(std::_Unfancy(new_ptr), arg, count);
            _traits::assign(std::_Unfancy(new_ptr)[count], _char_type());
        }
        else {
            _traits::copy(std::_Unfancy(new_ptr), arg, count + 1);
        }
    }

    template <class T, class... TArgs>
    constexpr void construct_in_place(T& obj, TArgs&&... args) noexcept(std::is_nothrow_constructible_v<T, TArgs...>)
    {
        if (std::is_constant_evaluated()) {
            std::construct_at(std::addressof(obj), std::forward<TArgs>(args)...);
        }
        else {
            new (static_cast<void*>(std::addressof(obj))) T(std::forward<TArgs>(args)...);
        }
    }

    _CONSTEXPR20 void take_contents(WuBaseString& right) noexcept
    {
        auto& data = m_pair._Myval2;
        auto& right_data = right.m_pair._Myval2;

        if constexpr (CanMemcpyVal) {
            if (!std::is_constant_evaluated()) {
                const auto my_data_mem = reinterpret_cast<unsigned char*>(std::addressof(m_pair._Myval2)) + MemcpyValOffset;
                const auto right_data_mem = reinterpret_cast<const unsigned char*>(std::addressof(right.m_pair._Myval2)) + MemcpyValOffset;
                std::memcpy(my_data_mem, right_data_mem, MemcpyValSize);

                right_data.MySize = 0;
                right_data.MyCapacity = SmallStringCapacity;
                right_data.ActivateSmallStringBuffer();
                _traits::assign(right_data.Storage.Buffer[0], _char_type());

                return;
            }
        }

        if (right_data.LargeModeEngaged()) {
            data._Swap_proxy_and_iterators(right_data);

            construct_in_place(data.Storage.Pointer, right_data.Storage.Pointer);
            right_data.Storage._Switch_to_buf();
        }
        else {
            right_data._Orphan_all();

            data.ActivateSmallStringBuffer();
            _traits::copy(data.Storage.Buffer, right_data.Storage.Buffer, right_data.MySize + 1);
        }

        data.MySize = right_data.MySize;
        data.MyCapacity = right_data.MyCapacity;

        right_data.MySize = 0;
        right_data.MyCapacity = SmallStringCapacity;
        _traits::assign(right_data.Storage.Buffer[0], _char_type());
    }

    enum class allocation_policy { at_least, exactly };

    template <allocation_policy policy = allocation_policy::at_least>
    _NODISCARD static constexpr pointer allocate_for_capacity(allocator_type& allocator, size_type& capacity)
    {
        _STL_INTERNAL_CHECK(capacity > SmallStringCapacity);
        ++capacity;

        pointer fancy_ptr = nullptr;
        if constexpr (policy == allocation_policy::at_least) {
            fancy_ptr = allocator.allocate(capacity);
        }
        else {
            _STL_INTERNAL_STATIC_ASSERT(policy == allocation_policy::exactly);
            fancy_ptr = allocator.allocate(capacity);
        }

        if (std::is_constant_evaluated()) {
            _char_type* const pointer = std::_Unfancy(fancy_ptr);
            for (size_type i = 0; i < capacity; i++) {
                std::construct_at(pointer + i);
            }
        }

        --capacity;
        return fancy_ptr;
    }

    template <class TFn, class... TArgs>
    constexpr WuBaseString& reallocate_for(const size_type new_size, TFn fn, TArgs... args)
    {
        /*
        *   !!CHANGE FOR EXCEPTION!!
        */
        if (new_size > max_size())
            throw;

        const size_type old_capacity = m_pair._Myval2.MyCapacity;
        size_type new_capacity = calculate_growth(new_size);
        auto& allocator = get_allocator();
        const pointer new_ptr = allocate_for_capacity(allocator, new_capacity);

        m_pair._Myval2._Orphan_all();
        m_pair._Myval2.MySize = new_size;
        m_pair._Myval2.MyCapacity = new_capacity;
        fn(std::_Unfancy(new_ptr), new_size, args...);
        if (old_capacity > SmallStringCapacity) {
            allocator.deallocate(m_pair._Myval2.Storage.Pointer, old_capacity);
            m_pair._Myval2.Storage.Pointer = new_ptr;
        }
        else {
            construct_in_place(m_pair._Myval2.Storage.Pointer, new_ptr);
        }

        return *this;
    }

    _NODISCARD constexpr size_type calculate_growth(const size_type requested) const noexcept
    {
        return calculate_growth(requested, m_pair._Myval2.MyCapacity, max_size());
    }

    _NODISCARD static constexpr size_type calculate_growth(const size_type requested, const size_type old, const size_type max) noexcept
    {
        const size_type masked = requested | AllocMask;
        if (masked > max)
            return max;

        if (old > max - old * 2)
            return max;

        return (std::max)(masked, old + old * 2);
    }

    constexpr allocator_type& get_allocator() noexcept { return m_pair._Get_first(); }
    constexpr const allocator_type& get_allocator() const noexcept { return m_pair._Get_first(); }

    _NODISCARD constexpr size_type max_size() const noexcept
    {
        const size_type alloc_max = alty_traits::max_size(get_allocator());
        const size_type storage_max = (std::max)(alloc_max, static_cast<size_type>(BUFFER_SIZE));

        return (std::min)(static_cast<size_type>(std::_Max_limit<difference_type>()), storage_max - 1);
    }

    constexpr void _Tidy_deallocate() noexcept
    {
        auto& data = m_pair._Myval2;
        if (data.LargeModeEngaged()) {
            auto& allocator = get_allocator();
            allocator.deallocate(data.Storage.Pointer, data.MyCapacity + 1);
            data.Storage._Switch_to_buf();
        }

        data.MySize = 0;
        data.MyCapacity = SmallStringCapacity;
        _traits::assign(data.Storage.Buffer[0], _char_type());
    }
};

using WWuString = WuBaseString<wchar_t, char_traits<wchar_t>>;
using WuString = WuBaseString<char, char_traits<char>>;
using Wuu16String = WuBaseString<char16_t, char_traits<char16_t>>;
using Wuu32String = WuBaseString<char32_t, char_traits<char32_t>>;

#if defined(__cpp_lib_char8_t)
using Wuu8String = WuBaseString<char8_t, char_traits<char8_t>>;
#endif
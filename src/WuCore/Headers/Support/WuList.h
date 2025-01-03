#pragma once
#pragma unmanaged

#include <xmemory>
#include <optional>
#include <functional>

///////////////////////////////////////////////////////////////////////////
//
//  ~ WindowsUtils list.
//
// ------------------------------------------------------------------------

//  Simple template vector-like collection.
//  This implementation is based on C++ std::vector<T>, and
//  .NET System.Collections.Generic.List<T>.
// 
//	The motivation behind this is to have a capacity calculation similar
//	to the .NET List<T>, and to have the 'emplace_back' behavior as
//	standard on the 'Add' function.
//	We also have other conveniences like searching using LINQ-like
//	delegates, or a constructor that accepts a initial capacity.
// 
//	Apart from the list method implementation the iterators and
//	type logic were copied from the std::vector<T>.
// 
//	You might notice that some of the names changes from the vector
//	implementation. I didn't do this to try to differ this implementation
//	from the original, but to force me to type and learn how it works.
//
//  This is probably not a good list implementation nor does
//  it have a reason to exist.

// ------------------------------------------------------------------------
//
//  Copyright (c) Francisco Nabas 2024.
//
//  This code, and the WindowsUtils module are distributed under
//  the MIT license. (are you sure you want to use this?)
//
///////////////////////////////////////////////////////////////////////////

#pragma region Iterators

/// <summary>
/// A const iterator for a WuList<T>.
/// </summary>
/// <typeparam name="T">The iterator type.</typeparam>
template<typename MyList>
class WuListConstIterator : public std::_Iterator_base
{
public:
	using iterator_concept   = std::contiguous_iterator_tag;
	using iterator_category  = std::random_access_iterator_tag;
	using value_type         = typename MyList::value_type;
	using difference_type    = typename MyList::difference_type;
	using pointer            = typename MyList::const_pointer;
	using reference          = const value_type&;

	using TPtr = typename MyList::pointer;

	constexpr WuListConstIterator() noexcept
		: Pointer() { }

	constexpr WuListConstIterator(TPtr pointerArg, const std::_Container_base* pointerList) noexcept
		: Pointer(pointerArg)
	{
		this->_Adopt(pointerList);
	}

	_NODISCARD constexpr reference operator*() const noexcept { return  *Pointer; }
	_NODISCARD constexpr pointer operator->() const { return Pointer; }

	constexpr WuListConstIterator& operator++() noexcept
	{
		++Pointer;
		return *this;
	}

	constexpr WuListConstIterator operator++(int) noexcept
	{
		WuListConstIterator temp = *this;
		++*this;
		return temp;
	}

	constexpr WuListConstIterator& operator--() noexcept
	{
		--Pointer;
		return *this;
	}

	constexpr WuListConstIterator operator--(int) noexcept
	{
		WuListConstIterator temp = *this;
		--*this;
		return temp;
	}

	constexpr WuListConstIterator& operator+=(const difference_type offset)
	{
		Pointer += offset;
		return *this;
	}

	_NODISCARD constexpr WuListConstIterator operator+(const difference_type offset) const noexcept
	{
		WuListConstIterator temp = *this;
		temp += offset;
		return temp;
	}

	_NODISCARD friend constexpr WuListConstIterator operator+(const difference_type offset, WuListConstIterator next) noexcept
	{
		next += offset;
		return next;
	}

	constexpr WuListConstIterator& operator-=(const difference_type offset) noexcept { return *this += -offset; }

	_NODISCARD constexpr WuListConstIterator operator-(const difference_type offset) const noexcept
	{
		WuListConstIterator temp = *this;
		temp -= offset;
		return temp;
	}

	_NODISCARD constexpr difference_type operator-(const WuListConstIterator& other) const noexcept { return static_cast<difference_type>(Pointer - other.Pointer); }
	_NODISCARD constexpr reference operator[](const difference_type offset) const noexcept { return *(*this + offset); }
	_NODISCARD constexpr bool operator==(const WuListConstIterator& other) const noexcept { return Pointer == other.Pointer; }
	_NODISCARD constexpr bool operator!=(const WuListConstIterator& other) const noexcept { return Pointer != other.Pointer; }
	_NODISCARD constexpr std::strong_ordering operator<=>(const WuListConstIterator& other) const noexcept { return std::_Unfancy_maybe_null(Pointer) <=> std::_Unfancy_maybe_null(other.Pointer); }

	using _Prevent_inheriting_unwrap = WuListConstIterator;

	_NODISCARD constexpr const value_type* _Unwrapped() const noexcept { return std::_Unfancy_maybe_null(Pointer); }
	constexpr void _Seek_to(const value_type* it) noexcept { Pointer = std::_Refancy_maybe_null<TPtr>(const_cast<value_type*>(it)); }

	TPtr Pointer;
};

template <class MyList>
struct std::pointer_traits<WuListConstIterator<MyList>>
{
	using pointer          = WuListConstIterator<MyList>;
	using element_type     = const pointer::value_type;
	using difference_type  = pointer::difference_type;

	_NODISCARD static constexpr element_type* to_address(const pointer iterator) noexcept { return std::to_address(iterator.Pointer); }
};

template<class MyList>
class WuListIterator : public WuListConstIterator<MyList>
{
public:
	using MyBase             = WuListConstIterator<MyList>;
	using iterator_concept   = std::contiguous_iterator_tag;
	using iterator_category  = std::random_access_iterator_tag;
	using value_type         = typename MyList::value_type;
	using difference_type    = typename MyList::difference_type;
	using pointer            = typename MyList::pointer;
	using reference          = value_type&;

	using MyBase::MyBase;

	_NODISCARD constexpr reference operator*() const noexcept { return const_cast<reference>(MyBase::operator*()); }
	_NODISCARD constexpr pointer operator->() const noexcept { return this->Pointer; }

	constexpr WuListIterator& operator++() noexcept
	{
		MyBase::operator++();
		return *this;
	}

	constexpr WuListIterator operator++(int) noexcept
	{
		WuListIterator temp = *this;
		MyBase::operator++();
		return temp;
	}

	constexpr WuListIterator& operator--() noexcept
	{
		MyBase::operator--();
		return *this;
	}

	constexpr WuListIterator operator--(int) noexcept
	{
		WuListIterator temp = *this;
		MyBase::operator--();
		return temp;
	}

	constexpr WuListIterator& operator+=(const difference_type offset) noexcept
	{
		MyBase::operator+=(offset);
		return *this;
	}

	_NODISCARD constexpr WuListIterator operator+(const difference_type offset) const noexcept
	{
		WuListIterator temp = *this;
		temp += offset;
		return temp;
	}

	_NODISCARD friend constexpr WuListIterator operator+(
		const difference_type offset, WuListIterator next) noexcept
	{
		next += offset;
		return next;
	}

	constexpr WuListIterator& operator-=(const difference_type offset) noexcept
	{
		MyBase::operator-=(offset);
		return *this;
	}

	using MyBase::operator-;

	_NODISCARD constexpr WuListIterator operator-(const difference_type offset) const noexcept
	{
		WuListIterator temp = *this;
		temp -= offset;
		return temp;
	}

	_NODISCARD constexpr reference operator[](const difference_type offset) const noexcept
	{
		return const_cast<reference>(MyBase::operator[](offset));
	}

	using _Prevent_inheriting_unwrap = WuListIterator;

	_NODISCARD constexpr value_type* _Unwrapped() const noexcept
	{
		return std::_Unfancy_maybe_null(this->Pointer);
	}
};

template <class MyList>
struct std::pointer_traits<WuListIterator<MyList>>
{
	using pointer          = WuListIterator<MyList>;
	using element_type     = pointer::value_type;
	using difference_type  = pointer::difference_type;

	_NODISCARD static constexpr element_type* to_address(const pointer iterator) noexcept { return std::to_address(iterator.Pointer); }
};

#pragma endregion

template<class ValueType, class SizeType, class DifferenceType, class Pointer, class ConstPointer>
struct ListIterTypes
{
	using value_type       = ValueType;
	using size_type        = SizeType;
	using difference_type  = DifferenceType;
	using pointer          = Pointer;
	using const_pointer    = ConstPointer;
};

template<class ValTypes>
class ListValue : public std::_Container_base
{
public:
	using value_type       = typename ValTypes::value_type;
	using size_type        = typename ValTypes::size_type;
	using difference_type  = typename ValTypes::difference_type;
	using pointer          = typename ValTypes::pointer;
	using const_pointer    = typename ValTypes::const_pointer;
	using reference        = value_type&;
	using const_reference  = const value_type&;

	constexpr ListValue() noexcept
		: MyFirst{ nullptr }, MyLast{ nullptr }, MyEnd{ nullptr } { }

	constexpr ListValue(pointer first, pointer last, pointer end) noexcept
		: MyFirst{ first }, MyLast{ last }, MyEnd{ end } { }

	constexpr void _Swap_val(ListValue& right) noexcept
	{
		this->_Swap_proxy_and_iterators(right);
		std::swap(MyFirst, right.MyFirst);
		std::swap(MyLast, right.MyLast);
		std::swap(MyEnd, right.MyEnd);
	}

	constexpr void _Take_contents(ListValue& right) noexcept
	{
		this->_Swap_proxy_and_iterators(right);
		MyFirst  = right.MyFirst;
		MyLast   = right.MyLast;
		MyEnd    = right.MyEnd;

		right.MyFirst  = nullptr;
		right.MyLast   = nullptr;
		right.MyEnd    = nullptr;
	}

	pointer MyFirst;
	pointer MyLast;
	pointer MyEnd;
};

/// <summary>
/// A collection of objects.
/// </summary>
/// <typeparam name="T">The object type.</typeparam>
/// <typeparam name="Allocator">The allocator type.</typeparam>
template<class T, class Allocator = std::allocator<T>>
class WuList
{
private:
	friend std::_Tidy_guard<WuList>;

	using AllocatorType           = std::_Rebind_alloc_t<Allocator, T>;
	using AllocatorTypeTraits     = std::allocator_traits<AllocatorType>;

public:
	using value_type              = T;
	using allocator_type          = Allocator;
	using pointer                 = typename AllocatorTypeTraits::pointer;
	using const_pointer           = typename AllocatorTypeTraits::const_pointer;
	using reference               = T&;
	using const_reference         = const T&;
	using size_type               = typename AllocatorTypeTraits::size_type;
	using difference_type         = typename AllocatorTypeTraits::difference_type;

private:
	using ScaryVal = ListValue<std::conditional_t<std::_Is_simple_alloc_v<AllocatorType>, std::_Simple_types<T>,
		ListIterTypes<T, size_type, difference_type, pointer, const_pointer>>>;

public:
	using iterator                = WuListIterator<ScaryVal>;
	using const_iterator          = WuListConstIterator<ScaryVal>;
	using reverse_iterator        = std::reverse_iterator<iterator>;
	using const_reverse_iterator  = std::reverse_iterator<const_iterator>;

#pragma region Constructors/Destructor

	/// <summary>
	/// Constructs a list with no items and zero capacity.
	/// </summary>
	constexpr WuList()
		: m_version{ }, m_pair(std::_Zero_then_variadic_args_t{ }) { }

	/// <summary>
	/// Constructs a list copying from another list.
	/// </summary>
	/// <param name="other">The list to copy from.</param>
	constexpr WuList(const WuList<value_type>& other)
		: m_version{ other.m_version }, m_pair(std::_One_then_variadic_args_t{ }, AllocatorTypeTraits::select_on_container_copy_construction(other.GetAllocator()))
	{
		const auto& otherData = other.m_pair._Myval2;
		const auto count = static_cast<size_type>(otherData.MyLast - otherData.MyFirst);
		ConstructN(count, otherData.MyFirst, otherData.MyLast);
	}

	/// <summary>
	/// Constructs a list moving another list.
	/// </summary>
	/// <param name="other">The list to move.</param>
	constexpr WuList(WuList<value_type>&& other) noexcept
		: m_version{ other.m_version }, m_pair(std::_One_then_variadic_args_t{ }, std::move(other.GetAllocator()),
			std::exchange(other.m_pair._Myval2.MyFirst, nullptr),
			std::exchange(other.m_pair._Myval2.MyLast, nullptr),
			std::exchange(other.m_pair._Myval2.MyEnd, nullptr))
	{
		m_pair._Myval2._Swap_proxy_and_iterators(other.m_pair._Myval2);
	}

	/// <summary>
	/// Constructs a list with no items and a initial capacity.
	/// </summary>
	/// <param name="capacity">The initial capacity.</param>
	constexpr WuList(_CRT_GUARDOVERFLOW size_type capacity, const Allocator& allocator = Allocator())
		: m_version{ }, m_pair(std::_One_then_variadic_args_t{ }, allocator)
	{
		auto& data = m_pair._Myval2;
		pointer& first = data.MyFirst;

		first = GetAllocator().allocate(capacity);
		data.MyLast = first;
		data.MyEnd = first + capacity;
	}

	/// <summary>
	/// Constructs a list from an initializer list.
	/// </summary>
	/// <param name="items">The initializer list.</param>
	constexpr WuList(const std::initializer_list<value_type> items, const Allocator& allocator = Allocator())
		: m_pair(std::_One_then_variadic_args_t{ }, allocator)
	{
		ConstructN(std::_Convert_size<size_type>(items.size()), items.begin(), items.end());
	}

	/// <summary>
	/// Constructs a list from a range.
	/// </summary>
	/// <param name="first">The starting offset pointer.</param>
	/// <param name="last">The last offset pointer.</param>
	constexpr WuList(const pointer first, const pointer last, const Allocator& allocator = Allocator())
		: m_version{ }, m_pair(std::_One_then_variadic_args_t{ }, allocator)
	{
		if (!first || !last || first == last)
			return;

		if (first > last)
			throw L"[WuList::WuList(first, last, allocator)]: First can't be bigger than last."

		auto& data                = m_pair._Myval2;
		pointer& myFirst          = data.MyFirst;
		pointer& myLast           = data.MyLast;
		pointer& myEnd            = data.MyEnd;

		const size_type count     = static_cast<size_type>(last - first);
		const size_type capacity  = GetNewCapacity(count);

		myFirst = allocator.allocate(capacity);
		myLast  = myFirst + count;
		myEnd   = myFirst + capacity;
	
		std::_Uninitialized_copy_n(first, count, myFirst, allocator);
	}

	/// <summary>
	/// Destructs the list freeing its memory.
	/// </summary>
	constexpr ~WuList() noexcept
	{
		_Tidy();
	}

#pragma endregion

#pragma region Properties

	/// <summary>
	/// Gets a pointer to the data.
	/// </summary>
	/// <returns>A pointer to the start of the data.</returns>
	_NODISCARD constexpr T* Data() noexcept { return std::_Unfancy_maybe_null(m_pair._Myval2.MyFirst); }

	/// <summary>
	/// Gets a pointer to the data.
	/// </summary>
	/// <returns>A pointer to the start of the data.</returns>
	_NODISCARD constexpr const T* Data() const noexcept { return std::_Unfancy_maybe_null(m_pair._Myval2.MyFirst); }

	/// <summary>
	/// Gets the current item count.
	/// </summary>
	/// <returns>The list item count.</returns>
	constexpr size_type Count() const
	{
		auto& data = m_pair._Myval2;
		return static_cast<size_type>(data.MyLast - data.MyFirst);
	}

	/// <summary>
	/// Gets the list capacity. This can be greater or equal to the Count.
	/// </summary>
	/// <returns>The list capacity.</returns>
	constexpr size_type Capacity() const
	{
		auto& data = m_pair._Myval2;
		return static_cast<size_type>(data.MyEnd - data.MyFirst);
	}

	/// <summary>
	/// Sets the list capacity.
	/// </summary>
	/// <param name="value">The new list capacity.</param>
	constexpr void SetCapacity(const size_type value)
	{
		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;
		pointer& end    = data.MyEnd;

		const size_type count = static_cast<size_type>(last - first);
		if (value < count || value < 1)
			throw L"[WuList::SetCapacity(value)]: Value can't be smaller than count or <= 0";

		const size_type capacity = static_cast<size_type>(end - first);

		if (value != capacity) {
			const pointer newItems = GetAllocator().allocate(value);
			if (count > 0) {
				auto& allocator = GetAllocator();
				std::_Uninitialized_copy_n(first, count, newItems, allocator);
			}

			first  = newItems;
			last   = first + count;
			end    = first + value;
		}
	}

#pragma endregion

#pragma region ItemManipulation

	/// <summary>
	/// Adds an item to the list by adding an existing object or constructing it in place.
	/// </summary>
	/// <typeparam name="...TArgs">The variadic template type.</typeparam>
	/// <param name="...args">The arguments.</param>
	template<class... TArgs>
	constexpr void Add(TArgs&&... args)
	{
		m_version++;
		EmplaceOneAtBack(std::forward<TArgs>(args)...);
	}

	/// <summary>
	/// Adds a range of items to the list.
	/// </summary>
	/// <param name="collection">The collection to add the items from.</param>
	constexpr void AddRange(const WuList<value_type>& collection)
	{
		auto& collectionData = collection.m_pair._Myval2;
		AppendRangeAtBack(collectionData.MyFirst, collectionData.MyLast);
	}

	/// <summary>
	/// Adds a range of items to the list.
	/// </summary>
	/// <param name="first">The starting offset pointer.</param>
	/// <param name="last">The last offset pointer.</param>
	constexpr void AddRange(const pointer first, const pointer last)
	{
		AppendRangeAtBack(first, last);
	}

	/// <summary>
	/// Clears the list.
	/// </summary>
	constexpr void Clear() noexcept
	{
		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;

		if (first == last)
			return;

		std::_Destroy_range(first, last, GetAllocator());
		last = first;
	}

	/// <summary>
	/// Inserts an item at the specified index with an existing object or constructing in place.
	/// </summary>
	/// <typeparam name="...TArgs">The variadic template type.</typeparam>
	/// <param name="index">The list index to insert the item at.</param>
	/// <param name="...args">The arguments.</param>
	template<class... TArgs>
	constexpr void Insert(size_type index, TArgs&&... args)
	{
		if (index > Count())
			throw L"[WuList::Insert(index, ... args)]: Index can't be bigger than the list count.";

		m_version++;
		EmplaceOneAt(m_pair._Myval2.MyFirst + index, std::forward<TArgs>(args)...);
	}

	template<class... TArgs>
	constexpr void Insert(iterator iterator, TArgs&&... args)
	{
		m_version++;
		EmplaceOneAt(iterator.Pointer, std::forward<TArgs>(args)...);
	}

	template <class Iter, std::enable_if_t<std::_Is_iterator_v<Iter>, int> = 0>
	constexpr iterator InsertRange(const_iterator where, Iter first, Iter last)
	{
		const pointer wherePtr = where.Pointer;
		auto& data = m_pair._Myval2;
		const pointer oldFirst = data.MyFirst;

		std::_Adl_verify_range(first, last);
		auto uFirst = std::_Get_unwrapped(first);
		auto uLast = std::_Get_unwrapped(last);
		const auto whereOff = static_cast<size_type>(wherePtr - oldFirst);
		if constexpr (std::_Is_cpp17_fwd_iter_v<Iter>) {
			const auto length = static_cast<size_t>(std::distance(uFirst, uLast));
			const auto count = std::_Convert_size<size_type>(length);
			InsertCountedRange(where, uFirst, count);
		}
		else if constexpr (std::forward_iterator<Iter>) {
			const auto length = std::_To_unsigned_like(std::ranges::distance(uFirst, uLast));
			const auto count = std::_Convert_size<size_type>(length);
			InsertCountedRange(where, uFirst, count);
		}
		else {
			InsertCountedRange(where, uFirst, uLast);
		}

		return iterator(data.MyFirst + whereOff, std::addressof(data));
	}

	/// <summary>
	/// Ensures the list capacity to a minimum value.
	/// </summary>
	/// <param name="capacity">The new capacity.</param>
	/// <returns>The new list capacity.</returns>
	constexpr size_type EnsureCapacity(size_type capacity)
	{
		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;
		pointer& end    = data.MyEnd;

		size_type myCapacity = static_cast<size_type>(end - first);
		if (capacity > myCapacity) {
			const size_type count   = static_cast<size_type>(last - first);
			const pointer newItems  = GetAllocator().allocate(capacity);
			if (count > 0) {
				auto& allocator = GetAllocator();
				std::_Uninitialized_copy_n(first, count, newItems, allocator);
			}

			first       = newItems;
			last        = first + count;
			myCapacity  = capacity;
			data.MyEnd  = first + myCapacity;
		}

		return myCapacity;
	}

	/// <summary>
	/// Slices the list into a new list.
	/// </summary>
	/// <param name="start">The starting index.</param>
	/// <param name="length">The number of elements.</param>
	/// <returns>A new list as a slice of the current list.</returns>
	constexpr WuList<value_type> Slice(size_type start, size_type length)
	{
		auto& allocator       = GetAllocator();
		auto& data            = m_pair._Myval2;
		pointer& first        = data.MyFirst;
		pointer& last         = data.MyLast;
		const size_type count = static_cast<size_type>(last - first);
		
		if (count - start < length)
			throw L"[WuList::Slice(start, length)]: Invalid start-length combination.";

		const pointer startOffset  = first + start;
		const pointer endOffset    = startOffset + length;

		WuList<value_type> output;
		const size_type newCapacity = output.GetNewCapacity(length);
		output.SetCapacity(newCapacity);

		std::_Uninitialized_copy_n(startOffset, length, output.m_pair._Myval2.MyFirst, allocator);
		output.m_pair._Myval2.MyLast = output.m_pair._Myval2.MyFirst + length;

		return output;
	}

	/// <summary>
	/// Removes the last item from the list.
	/// </summary>
	constexpr void RemoveBack()
	{
		m_version++;

		pointer& last = m_pair._Myval2.MyLast;
		AllocatorTypeTraits::destroy(GetAllocator(), std::_Unfancy(last - 1));
		--last;
	}

	/// <summary>
	/// Removes all items that matches a predicate.
	/// </summary>
	/// <param name="predicate">The predicate to match items.</param>
	/// <returns>The number of items removed.</returns>
	constexpr size_type RemoveAll(std::function<bool(const reference)> predicate)
	{
		size_type removed = 0;
		bool increaseVersion = false;

		auto& allocator       = GetAllocator();
		auto& data            = m_pair._Myval2;
		pointer& first        = data.MyFirst;
		pointer& last         = data.MyLast;
		const size_type count = static_cast<size_type>(last - first);

		// Going through each item.
		for (size_type i = 0; i < count;) {
			reference item = first[i];
			if (predicate(item)) {

				// It's a match. Remove the item and rearrange the list.
				increaseVersion = true;
				const pointer where = &item;
				std::_Move_unchecked(where + 1, last, where);
				AllocatorTypeTraits::destroy(allocator, std::_Unfancy(last - 1));
				--last;
				++removed;

				// We don't advance the index because an item was removed. The current index is now the next item.
				continue;
			}

			++i;
		}

		if (increaseVersion)
			m_version++;

		return removed;
	}

	/// <summary>
	/// Removes an item at the specified index.
	/// </summary>
	/// <param name="index">The index to remove the item from.</param>
	constexpr void RemoveAt(const size_type index)
	{
		if (index >= Count())
			throw L"[WuList::RemoveAt(index)]: Index can't be bigger or equal to the list count.";

		m_version++;

		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;

		const pointer where = first + index;
		std::_Move_unchecked(where + 1, last, where);
		AllocatorTypeTraits::destroy(GetAllocator(), std::_Unfancy(last - 1));
		--last;
	}

	/// <summary>
	/// Removes an item at the input iterator position.
	/// </summary>
	/// <param name="where">The iterator.</param>
	/// <returns>A new iterator on the position of the next item.</returns>
	constexpr iterator Remove(const const_iterator where) noexcept(std::is_nothrow_move_assignable_v<value_type>)
	{
		m_version++;

		auto& allocator  = GetAllocator();
		auto& data       = m_pair._Myval2;
		pointer& last    = data.MyLast;

		const pointer wherePtr = where.Pointer;
		std::_Move_unchecked(wherePtr + 1, last, wherePtr);
		AllocatorTypeTraits::destroy(allocator, std::_Unfancy(last - 1));
		--last;

		return iterator{ wherePtr, std::addressof(data) };
	}

	/// <summary>
	/// Removes a range of items between the input iterators.
	/// </summary>
	/// <param name="first">The iterator at the first position.</param>
	/// <param name="last">The iterator at the last position.</param>
	/// <returns>A new iterator on the position of the next item.</returns>
	constexpr iterator Remove(const const_iterator first, const const_iterator last) noexcept(std::is_nothrow_move_assignable_v<value_type>)
	{
		m_version++;
		const pointer firstPtr  = first.Pointer;
		const pointer lastPtr   = last.Pointer;

		auto& allocator         = GetAllocator();
		auto& data              = m_pair._Myval2;
		pointer& myFirst        = data.MyFirst;
		pointer& myLast         = data.MyLast;

		if (firstPtr != lastPtr) {
			const pointer newLast = std::_Move_unchecked(lastPtr, myLast, firstPtr);
			std::_Destroy_range(newLast, myLast, allocator);
			myLast = newLast;
		}

		return iterator{ firstPtr, std::addressof(data) };
	}

#pragma endregion

#pragma region Searching/Iteration

	/// <summary>
	/// Gets the last item on the list.
	/// </summary>
	/// <returns>A reference to the last item on the list.</returns>
	constexpr reference Back()
	{
		auto& data = m_pair._Myval2;
		if (data.MyFirst == data.MyLast)
			throw L"[WuList::Back()]: Can't call 'Back()' on an empty list.";

		return data.MyLast[-1];
	}

	/// <summary>
	/// Gets the last item on the list.
	/// </summary>
	/// <returns>A reference to the last item on the list.</returns>
	constexpr const_reference Back() const
	{
		auto& data = m_pair._Myval2;
		if (data.MyFirst == data.MyLast)
			throw L"[WuList::Back()]: Can't call 'Back()' on an empty list.";

		return data.MyLast[-1];
	}

	/// <summary>
	/// Checks if an item exists that matches a predicate.
	/// </summary>
	/// <param name="predicate">The predicate to match items.</param>
	/// <returns>True if the item exists.</returns>
	constexpr bool Exists(std::function<bool(const_reference)> predicate)
	{
		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;

		const size_type count = static_cast<size_type>(last - first);
		for (size_type i = 0; i < count; i++) {
			if (predicate(first[i]))
				return true;
		}

		return false;
	}

	/// <summary>
	/// Finds the first occurrence of an item in the list with a predicate.
	/// </summary>
	/// <param name="predicate">The predicate to match items.</param>
	/// <returns>An optional reference to the found item or nothing.</returns>
	constexpr std::optional<std::reference_wrapper<const value_type>> Find(std::function<bool(const_reference)> predicate)
	{
		for (const_reference item : *this) {
			if (predicate(item))
				return std::reference_wrapper<const value_type>(item);
		}

		return std::nullopt;
	}

	/// <summary>
	/// Finds the last occurrence of an item in the list with a predicate.
	/// </summary>
	/// <param name="predicate">The predicate to match items.</param>
	/// <returns>An optional reference to the found item or nothing.</returns>
	constexpr std::optional<std::reference_wrapper<const value_type>> FindLast(std::function<bool(const_reference)> predicate)
	{
		for (auto iterator = rbegin(); iterator != rend(); ++iterator) {
			reference item = *iterator;
			if (predicate(item))
				return std::reference_wrapper<const value_type>(item);
		}

		return std::nullopt;
	}

	/// <summary>
	/// Finds all items matching a predicate.
	/// </summary>
	/// <param name="predicate">The predicate to match items.</param>
	/// <returns>A list containing all the items that matched, or an empty list.</returns>
	constexpr WuList<value_type> FindAll(std::function<bool(const_reference)> predicate)
	{
		WuList<value_type> output;
		for (const_reference item : *this) {
			if (predicate(item))
				output.Add(item);
		}

		return output;
	}

	/// <summary>
	/// Finds the index of the first occurrence of an item matching a predicate.
	/// </summary>
	/// <param name="predicate">The predicate to match items.</param>
	/// <returns>The index of the first occurrence, or 0xFFFFFFFFFFFFFFFF</returns>
	constexpr size_type FindIndex(std::function<bool(const_reference)> predicate)
	{
		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;

		const size_type count = static_cast<size_type>(last - first);
		for (size_type i = 0; i < count; i++) {
			if (predicate(first[i]))
				return i;
		}

		return static_cast<size_type>(-1);
	}

	/// <summary>
	/// Finds the index of the last occurrence of an item matching a predicate.
	/// </summary>
	/// <param name="predicate">The predicate to match items.</param>
	/// <returns>The index of the first occurrence, or 0xFFFFFFFFFFFFFFFF</returns>
	constexpr size_type FindLastIndex(std::function<bool(const_reference)> predicate)
	{
		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;

		const size_type count = static_cast<size_type>(last - first);
		for (size_type i = (count - 1); i != static_cast<size_type>(-1); i--) {
			if (predicate(first[i]))
				return i;
		}

		return static_cast<size_type>(-1);
	}

	/// <summary>
	/// Executes an action for each item on the list.
	/// </summary>
	/// <param name="action">The action to execute.</param>
	constexpr void ForEach(std::function<void(const_reference)> action)
	{
		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;

		const size_type count    = static_cast<size_type>(last - first);
		const size_type version  = m_version;
		for (size_type i = 0; i < count; i++) {
			if (m_version != version)
				break;

			action(first[i]);
		}

		if (version != m_version)
			throw L"[WuList::ForEach(action)]: The collection was modified during the operation.";
	}

	/// <summary>
	/// Gets an iterator to the first item on the list.
	/// </summary>
	/// <returns>The iterator.</returns>
	constexpr iterator begin()
	{
		auto& data = m_pair._Myval2;
		return iterator(data.MyFirst, std::addressof(data));
	}

	/// <summary>
	/// Gets a const iterator to the first item on the list.
	/// </summary>
	/// <returns>The iterator.</returns>
	constexpr const const_iterator begin() const noexcept
	{
		auto& data = m_pair._Myval2;
		return const_iterator(data.MyFirst, std::addressof(data));
	}

	/// <summary>
	/// Gets an iterator to the last item on the list.
	/// </summary>
	/// <returns>The iterator.</returns>
	constexpr iterator end()
	{
		auto& data = m_pair._Myval2;
		return iterator(data.MyLast, std::addressof(data));
	}

	/// <summary>
	/// Gets a const iterator to the last item on the list.
	/// </summary>
	/// <returns>The iterator.</returns>
	constexpr const const_iterator end() const noexcept
	{
		auto& data = m_pair._Myval2;
		return const_iterator(data.MyLast, std::addressof(data));
	}

	/// <summary>
	/// Gets a reverse-iterator to the last item on the list.
	/// </summary>
	/// <returns>The iterator.</returns>
	constexpr reverse_iterator rbegin() { return reverse_iterator(end()); }

	/// <summary>
	/// Gets a const reverse-iterator to the last item on the list.
	/// </summary>
	/// <returns>The iterator.</returns>
	constexpr const const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

	/// <summary>
	/// Gets a reverse-iterator to the first item on the list.
	/// </summary>
	/// <returns>The iterator.</returns>
	constexpr reverse_iterator rend() { return reverse_iterator(begin()); }

	/// <summary>
	/// Gets a const reverse-iterator to the first item on the list.
	/// </summary>
	/// <returns>The iterator.</returns>
	constexpr const const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

#pragma endregion

#pragma region Operators

	/// <summary>
	/// Gets the item at the specified index.
	/// </summary>
	/// <param name="index">The index.</param>
	/// <returns>A reference to the item at the specified index.</returns>
	constexpr reference operator[](size_type index)
	{
		auto& data      = m_pair._Myval2;
		pointer first	= data.MyFirst;
		pointer last	= data.MyLast;
		const auto size = static_cast<size_type>(last - first);

		if (index >= size)
			throw L"[WuList::operator[](index)]: Index can't be greater or equal to the list size.";

		return first[index];
	}

	/// <summary>
	/// Gets the item at the specified index.
	/// </summary>
	/// <param name="index">The index.</param>
	/// <returns>A reference to the item at the specified index.</returns>
	constexpr const reference operator[](size_type index) const
	{
		auto& data		     = m_pair._Myval2;
		const pointer first  = data.MyFirst;
		const pointer last   = data.MyLast;
		const auto size		 = static_cast<size_type>(last - first);

		if (index >= size)
			throw L"[WuList::operator[](index)]: Index can't be greater or equal to the list size.";

		return first[index];
	}

	/// <summary>
	/// Assigns another list to the current list by moving it.
	/// </summary>
	/// <param name="other">The other list.</param>
	/// <returns>A reference to the current list.</returns>
	constexpr WuList<value_type>& operator=(WuList<value_type>&& other) noexcept
	{
		if (this == std::addressof(other))
			return *this;

		AllocatorType& allocator       = GetAllocator();
		AllocatorType& otherAllocator  = other.GetAllocator();

		constexpr auto pocmaVal = std::_Choose_pocma_v<AllocatorType>;
		if constexpr (pocmaVal == std::_Pocma_values::_No_propagate_allocators) {
			if (allocator != otherAllocator) {
				MoveAssignUnequalAlloc(other);
				return *this;
			}
		}

		_Tidy();
		std::_Pocma(allocator, otherAllocator);
		m_pair._Myval2._Take_contents(other.m_pair._Myval2);

		return *this;
	}

	constexpr WuList<value_type>& operator=(const WuList<value_type>& other)
	{
		if (this == std::addressof(other))
			return *this;

		auto& allocator       = GetAllocator();
		auto& otherAllocator  = other.GetAllocator();
		if constexpr (std::_Choose_pocca_v<AllocatorType>) {
			if (allocator != otherAllocator) {
				_Tidy();
			}
		}

		std::_Pocca(allocator, otherAllocator);
		auto& otherData = other.m_pair._Myval2;
		AssignCountedRange(otherData.MyFirst, static_cast<size_type>(otherData.MyLast - otherData.MyFirst));

		return *this;
	}

#pragma endregion

private:
#pragma region Fields

	size_type m_version;
	std::_Compressed_pair<AllocatorType, ScaryVal> m_pair;

	constexpr static size_type s_defaultCapacity = 4;

#pragma endregion

#pragma region HelperMethods

	/// <summary>
	/// Emplaces an item at the end of the list.
	/// </summary>
	/// <typeparam name="...TArgs">The variadic template type.</typeparam>
	/// <param name="...args">The arguments.</param>
	/// <returns>A reference to the emplaced item.</returns>
	template<class... TArgs>
	constexpr reference EmplaceOneAtBack(TArgs&&... args)
	{
		auto& data     = m_pair._Myval2;
		pointer& last  = data.MyLast;
		pointer& end   = data.MyEnd;

		if (last != end)
			return EmplaceAtWithUnusedCapacity(last, std::forward<TArgs>(args)...);

		return *EmplaceReallocate(last, std::forward<TArgs>(args)...);
	}

	/// <summary>
	/// Emplaces an item at the specified position.
	/// </summary>
	/// <typeparam name="...TArgs">The variadic template type.</typeparam>
	/// <param name="where">The position.</param>
	/// <param name="...args">The arguments.</param>
	/// <returns>A reference to the emplaced item.</returns>
	template<class... TArgs>
	constexpr reference EmplaceOneAt(const pointer where, TArgs&&... args)
	{
		auto& data     = m_pair._Myval2;
		pointer& last  = data.MyLast;
		pointer& end   = data.MyEnd;

		if (last != end)
			return EmplaceAtWithUnusedCapacity(where, std::forward<TArgs>(args)...);

		return *EmplaceReallocate(where, std::forward<TArgs>(args)...);
	}

	/// <summary>
	/// Emplaces an item at the specified position utilizing the remaining capacity.
	/// </summary>
	/// <typeparam name="...TArgs">The variadic template type.</typeparam>
	/// <param name="where">The position.</param>
	/// <param name="...args">The arguments.</param>
	/// <returns>A reference to the emplaced item.</returns>
	template<class... TArgs>
	constexpr reference EmplaceAtWithUnusedCapacity(const pointer where, TArgs&&... args)
	{
		auto& allocator		= GetAllocator();
		auto& data			= m_pair._Myval2;
		const pointer last  = data.MyLast;
		const pointer end   = data.MyEnd;

		if (where == last) {
			//_Emplace_back_with_unused_capacity(forward<>(args)...);
			if constexpr (std::conjunction_v<std::is_nothrow_constructible<T, TArgs...>, std::_Uses_default_construct<Allocator, T*, TArgs...>>) {
				std::_Construct_in_place(*last, std::forward<TArgs>(args)...);
			}
			else {
				AllocatorTypeTraits::construct(allocator, std::_Unfancy(last), std::forward<TArgs>(args)...);
			}

			++data.MyLast;
		}
		else {
			std::_Alloc_temporary2<AllocatorType> obj(allocator, std::forward<TArgs>(args)...);
			AllocatorTypeTraits::construct(allocator, std::_Unfancy(last), std::move(last[-1]));
			++data.MyLast;
			std::_Move_backward_unchecked(where, last - 1, last);
			*where = std::move(obj._Get_value());
		}

		reference result = *data.MyLast;

		return result;
	}

	/// <summary>
	/// Emplaces an item at the specified position while growing the list capacity.
	/// </summary>
	/// <typeparam name="...TArgs">The variadic template type.</typeparam>
	/// <param name="where">The position.</param>
	/// <param name="...args">The arguments.</param>
	/// <returns>A pointer to the emplaced item.</returns>
	template<class... TArgs>
	constexpr pointer EmplaceReallocate(const pointer where, TArgs&&... args)
	{
		auto& allocator			= GetAllocator();
		auto& data				= m_pair._Myval2;
		pointer& first			= data.MyFirst;
		pointer& last			= data.MyLast;

		const auto oldSize      = static_cast<size_type>(last - first);
		const auto whereOffset  = static_cast<size_type>(where - first);

		if (oldSize == MaxSize())
			throw L"[WuList::EmplaceReallocate(where, ... args)]: List is too long.";

		const size_type newSize = oldSize + 1;
		size_type newCapacity   = GetNewCapacity(newSize);

		const pointer newList          = std::_Allocate_at_least_helper(allocator, newCapacity);
		const pointer constructedLast  = newList + whereOffset + 1;
		pointer constructedFirst	   = constructedLast;
		try {
			AllocatorTypeTraits::construct(allocator, std::_Unfancy(newList + whereOffset), std::forward<TArgs>(args)...);
			constructedFirst = newList + whereOffset;
			if (where == last) {
				if constexpr (std::is_nothrow_move_constructible_v<value_type> || !std::is_copy_constructible_v<value_type>) {
					std::_Uninitialized_move(first, last, newList, allocator);
				}
				else {
					std::_Uninitialized_copy(first, last, newList, allocator);
				}
			}
			else {
				std::_Uninitialized_move(first, where, newList, allocator);
				constructedFirst = newList;
				std::_Uninitialized_move(where, last, newList + whereOffset + 1, allocator);
			}
		}
		catch (...) {
			std::_Destroy_range(constructedFirst, constructedLast, allocator);
			allocator.deallocate(newList, newCapacity);

			throw;
		}

		ChangeArray(newList, newSize, newCapacity);

		return newList + whereOffset;
	}

	/// <summary>
	/// Adds a range of items to the back of the list.
	/// </summary>
	/// <param name="first">The starting offset pointer.</param>
	/// <param name="last">The last offset pointer.</param>
	constexpr void AppendRangeAtBack(const pointer first, const pointer last)
	{
		if (!first || !last || first == last)
			return;

		if (first > last)
			throw L"[WuList::AppendRangeAtBack(first, last)]: First can't be bigger than last.";

		m_version++;

		auto& allocator            = GetAllocator();
		auto& data                 = m_pair._Myval2;
		pointer& myFirst           = data.MyFirst;
		pointer& myLast            = data.MyLast;
		pointer& myEnd             = data.MyEnd;
		const size_type count      = static_cast<size_type>(myLast - myFirst);
		const size_type rangeSize  = static_cast<size_type>(last - first);
		const size_type remaining  = static_cast<size_type>(myEnd - myLast);

		if (rangeSize > remaining) {
			if (rangeSize > MaxSize())
				throw;

			const size_type newSize        = count + rangeSize;
			size_type newCapacity		   = GetNewCapacity(newSize);
			const pointer newItems         = std::_Allocate_at_least_helper(allocator, newCapacity);
			const pointer constructedLast  = newItems + count + rangeSize;
			pointer constructedFirst	   = constructedLast;
			try {
				std::_Uninitialized_copy_n(std::move(first), rangeSize, newItems + count, allocator);
				constructedFirst = newItems + count;
				if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
					std::_Uninitialized_move(myFirst, myLast, newItems, allocator);
				}
				else {
					std::_Uninitialized_copy(myFirst, myLast, newItems, allocator);
				}
			}
			catch (...) {
				std::_Destroy_range(constructedFirst, constructedLast, allocator);
				allocator.deallocate(newItems, newCapacity);

				throw;
			}

			myFirst  = newItems;
			myLast   = constructedLast;
			myEnd    = newItems + newCapacity;
		}
		else {
			std::_Uninitialized_copy_n(std::move(first), rangeSize, myLast, allocator);
			myLast += rangeSize;
		}
	}

	/// <summary>
	/// Calculates a new capacity for the list.
	/// </summary>
	/// <param name="capacity">The new required capacity.</param>
	/// <returns>The new capacity.</returns>
	constexpr size_type GetNewCapacity(const size_type capacity) const
	{
		const size_type maxSize     = MaxSize();
		const size_type myCapacity  = Capacity();

		size_type newCapacity = myCapacity == 0 ? s_defaultCapacity : 2 * myCapacity;
		if (newCapacity > maxSize)
			return maxSize;

		if (newCapacity < capacity)
			return capacity;

		return newCapacity;
	}

	/// <summary>
	/// Changes the current items array to a new one.
	/// </summary>
	/// <param name="newList">The new list items pointer.</param>
	/// <param name="newSize">The new list size.</param>
	/// <param name="newCapacity">The new list capacity.</param>
	constexpr void ChangeArray(const pointer newList, const size_type newSize, const size_type newCapacity)
	{
		auto& allocator  = GetAllocator();
		auto& data       = m_pair._Myval2;
		pointer& first   = data.MyFirst;
		pointer& last    = data.MyLast;
		pointer& end     = data.MyEnd;

		if (first) {
			std::_Destroy_range(first, last, allocator);
			allocator.deallocate(first, static_cast<size_type>(end - first));
		}

		first  = newList;
		last   = newList + newSize;
		end    = newList + newCapacity;
	}

	/// <summary>
	/// Constructs a number of items on an empty list.
	/// </summary>
	/// <typeparam name="...ValType">The variadic template arguments type.</typeparam>
	/// <param name="count">The number of items to construct.</param>
	/// <param name="...value">The arguments to be forwarded to the constructors.</param>
	template<class... ValType>
	constexpr void ConstructN(const size_type count, ValType&&... value)
	{
		auto& allocator  = GetAllocator();
		auto& data       = m_pair._Myval2;
		if (count != 0) {
			BuyNonZero(count);
			std::_Tidy_guard<WuList> guard{ this };
			if constexpr (sizeof...(value) == 0) {
				data.MyLast = std::_Uninitialized_value_construct_n(data.MyFirst, count, allocator);
			}
			else if constexpr (sizeof...(value) == 1) {
				data.MyLast = std::_Uninitialized_fill_n(data.MyFirst, count, value..., allocator);
			}
			else if constexpr (sizeof...(value) == 2) {
				data.MyLast = std::_Uninitialized_copy(std::forward<ValType>(value)..., data.MyFirst, allocator);
			}
			else {
				_STL_INTERNAL_STATIC_ASSERT(false);
			}
			guard._Target = nullptr;
		}
	}

	/// <summary>
	/// Allocates a non-zero capacity.
	/// </summary>
	/// <param name="newCapacity">The capacity.</param>
	constexpr void BuyNonZero(size_type newCapacity)
	{
		if (newCapacity > MaxSize())
			throw L"[WuList::BuyNonZero(newCapacity)]: List is too long.";

		BuyRaw(newCapacity);
	}

	/// <summary>
	/// Allocates a capacity for the new list items.
	/// </summary>
	/// <param name="newCapacity">The capacity.</param>
	constexpr void BuyRaw(size_type newCapacity)
	{
		auto& data      = m_pair._Myval2;
		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;
		pointer& end    = data.MyEnd;

		_STL_INTERNAL_CHECK(!first && !last && !end);
		_STL_INTERNAL_CHECK(0 < newCapacity && newCapacity <= MaxSize());

		const pointer newList = std::_Allocate_at_least_helper(GetAllocator(), newCapacity);
		first  = newList;
		last   = newList;
		end    = newList + newCapacity;
	}

	template <class Iter>
	constexpr void InsertCountedRange(const_iterator where, Iter first, const size_type count)
	{
		const pointer wherePtr = where.Pointer;

		auto& allocator = GetAllocator();
		auto& data = m_pair._Myval2;
		pointer& myLast = data.MyLast;

		const pointer oldFirst = data.MyFirst;
		const pointer oldLast = myLast;
		const auto unusedCapacity = static_cast<size_type>(data.MyEnd - oldLast);

		if (count == 0) {
		}
		else if (count > unusedCapacity) {
			const auto oldSize = static_cast<size_type>(oldLast - oldFirst);

			if (count > MaxSize() - oldSize)
				throw L"[WuList::InsertCountedRange(where, first, count)]: List is too big.";

			const size_type newSize = oldSize + count;
			size_type newCapacity = GetNewCapacity(newSize);

			const pointer newList = std::_Allocate_at_least_helper(allocator, newCapacity);
			const auto whereOff = static_cast<size_type>(wherePtr - oldFirst);
			const pointer constructedLast = newList + whereOff + count;
			pointer constructedFirst = constructedLast;
			try {
				std::_Uninitialized_copy_n(std::move(first), count, newList + whereOff, allocator);
				constructedFirst = newList + whereOff;
				if (count == 1 && wherePtr == oldLast) {
					if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
						std::_Uninitialized_move(oldFirst, oldLast, newList, allocator);
					}
					else {
						std::_Uninitialized_copy(oldFirst, oldLast, newList, allocator);
					}
				}
				else {
					std::_Uninitialized_move(oldFirst, wherePtr, newList, allocator);
					constructedFirst = newList;
					std::_Uninitialized_move(wherePtr, oldLast, newList + whereOff + count, allocator);
				}
			}
			catch (...) {
				std::_Destroy_range(constructedFirst, constructedLast, allocator);
				allocator.deallocate(newList, newCapacity);

				throw;
			}

			ChangeArray(newList, newSize, newCapacity);
		}
		else {
			const auto affectedElements = static_cast<size_type>(oldLast - wherePtr);
			if (count < affectedElements) {
				myLast = std::_Uninitialized_move(oldLast - count, oldLast, oldLast, allocator);
				std::_Move_backward_unchecked(wherePtr, oldLast - count, oldLast);
				std::_Destroy_range(wherePtr, wherePtr + count, allocator);
				try {
					std::_Uninitialized_copy_n(std::move(first), count, wherePtr, allocator);
				}
				catch (...) {
					try {
						std::_Uninitialized_move(wherePtr + count, wherePtr + 2 * count, wherePtr, allocator);
					}
					catch (...) {
						// vaporize the detached piece.
						std::_Destroy_range(wherePtr + count, myLast, allocator);
						myLast = wherePtr;

						throw;
					}

					std::_Move_unchecked(wherePtr + 2 * count, myLast, wherePtr + count);
					std::_Destroy_range(oldLast, myLast, allocator);
					myLast = oldLast;

					throw;
				}
			}
			else {
				const pointer relocated = wherePtr + count;
				myLast = std::_Uninitialized_move(wherePtr, oldLast, relocated, allocator);
				std::_Destroy_range(wherePtr, oldLast, allocator);
				try {
					std::_Uninitialized_copy_n(std::move(first), count, wherePtr, allocator);
				}
				catch (...) {
					try {
						std::_Uninitialized_move(relocated, myLast, wherePtr, allocator);
					}
					catch (...) {
						// vaporize the detached piece
						std::_Destroy_range(relocated, myLast, allocator);
						myLast = wherePtr;

						throw;
					}

					std::_Destroy_range(relocated, myLast, allocator);
					myLast = oldLast;

					throw;
				}
			}
		}
	}

	template <class Iter>
	constexpr void AssignCountedRange(Iter first, const size_type newSize)
	{
		auto& allocator    = GetAllocator();
		auto& data         = m_pair._Myval2;
		pointer& myFirst   = data.MyFirst;
		pointer& myLast    = data.MyLast;
		pointer& myEnd     = data.MyEnd;

		constexpr bool noThrowConstruct = std::conjunction_v<std::is_nothrow_constructible<T, std::_Iter_ref_t<Iter>>,
			std::_Uses_default_construct<Allocator, T*, std::_Iter_ref_t<Iter>>>;

		data._Orphan_all();
		const auto oldCapacity = static_cast<size_type>(myEnd - myFirst);
		if (newSize > oldCapacity) {
			ClearAndReserveGeometric(newSize);
			if constexpr (noThrowConstruct) {
				myLast = std::_Uninitialized_copy_n(std::move(first), newSize, myFirst, allocator);
			}
			else {
				myLast = std::_Uninitialized_copy_n(std::move(first), newSize, myFirst, allocator);
			}

			return;
		}

		const auto oldSize = static_cast<size_type>(myLast - myFirst);
		if (newSize > oldSize) {
			bool copied = false;
			if constexpr (std::_Iter_copy_cat<Iter, pointer>::_Bitcopy_assignable) {
				if (!std::is_constant_evaluated()) {
					std::_Copy_memmove_n(first, static_cast<size_t>(oldSize), myFirst);
					first += oldSize;
					copied = true;
				}
			}

			if (!copied) {
				for (auto mid = myFirst; mid != myLast; ++mid, (void) ++first) {
					*mid = *first;
				}
			}

			if constexpr (noThrowConstruct) {
				myLast = std::_Uninitialized_copy_n(std::move(first), newSize - oldSize, myLast, allocator);
			}
			else {
				myLast = std::_Uninitialized_copy_n(std::move(first), newSize - oldSize, myLast, allocator);
			}
		}
		else {
			const pointer newLast = myFirst + newSize;
			std::_Copy_n_unchecked4(std::move(first), newSize, myFirst);
			std::_Destroy_range(newLast, myLast, allocator);
			myLast = newLast;
		}
	}

	/// <summary>
	/// Moves items from another list with different a allocator.
	/// </summary>
	/// <param name="other">The other list.</param>
	constexpr void MoveAssignUnequalAlloc(WuList& other)
	{
		auto& allocator  = GetAllocator();
		auto& data       = m_pair._Myval2;
		auto& otherData  = other.m_pair._Myval2;

		const pointer otherFirst  = otherData.MyFirst;
		const pointer otherLast   = otherData.MyLast;
		const auto newSize        = static_cast<size_type>(otherLast - otherFirst);

		pointer& first  = data.MyFirst;
		pointer& last   = data.MyLast;

		const auto oldCapacity = static_cast<size_type>(data.MyEnd - first);
		if (newSize > oldCapacity) {
			ClearAndReserveGeometric(newSize);
			last = std::_Uninitialized_move(otherFirst, otherLast, first, allocator);

			return;
		}

		const auto oldSize = static_cast<size_type>(last - first);
		if (newSize > oldSize) {
			const pointer mid = otherFirst + oldSize;
			std::_Move_unchecked(otherFirst, mid, first);
			last = std::_Uninitialized_move(mid, otherLast, last, allocator);
		}
		else {
			const pointer newLast = first + newSize;
			std::_Move_unchecked(otherFirst, otherLast, first);
			std::_Destroy_range(newLast, last, allocator);
			last = newLast;
		}
	}

	/// <summary>
	/// Clears the items and allocates a new capacity.
	/// </summary>
	/// <param name="newSize"></param>
	constexpr void ClearAndReserveGeometric(const size_type newSize)
	{
		auto& allocator  = GetAllocator();
		auto& data       = m_pair._Myval2;
		pointer& first   = data.MyFirst;
		pointer& last    = data.MyLast;
		pointer& end     = data.MyEnd;

		if (newSize > MaxSize())
			throw L"[WuList::ClearAndReserveGeometric(newSize)]: List is too long.";

		const size_type newCapacity = GetNewCapacity(newSize);

		if (first) {
			std::_Destroy_range(first, last, allocator);
			allocator.deallocate(first, static_cast<size_type>(end - first));

			first  = nullptr;
			last   = nullptr;
			end    = nullptr;
		}

		BuyRaw(newCapacity);
	}

	/// <summary>
	/// Gets the theoretical maximum size for the list.
	/// </summary>
	/// <returns>The theoretical maximum list size.</returns>
	_NODISCARD constexpr size_type MaxSize() const noexcept
	{
		return (std::min)(static_cast<size_type>(std::_Max_limit<difference_type>()), AllocatorTypeTraits::max_size(GetAllocator()));
	}

	/// <summary>
	/// Gets the current allocator.
	/// </summary>
	/// <returns>A reference to the current allocator.</returns>
	_NODISCARD constexpr AllocatorType& GetAllocator() noexcept { return m_pair._Get_first(); }

	/// <summary>
	/// Gets the current allocator.
	/// </summary>
	/// <returns>A reference to the current allocator.</returns>
	_NODISCARD constexpr const AllocatorType& GetAllocator() const noexcept { return m_pair._Get_first(); }

	/// <summary>
	/// Destroys the items in the list and deallocate the buffer.
	/// </summary>
	constexpr void _Tidy() noexcept
	{
		auto& allocator  = GetAllocator();
		auto& data       = m_pair._Myval2;
		pointer& first   = data.MyFirst;
		pointer& last    = data.MyLast;
		pointer& end     = data.MyEnd;

		if (first) {
			std::_Destroy_range(first, last, allocator);
			allocator.deallocate(first, static_cast<size_type>(end - first));

			first  = nullptr;
			last   = nullptr;
			end    = nullptr;
		}
	}

#pragma endregion
};
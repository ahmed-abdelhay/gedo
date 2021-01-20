/*
 * Author: Ahmed Abdelhay
 * A collection of functions used to make programming easier.
 * contains memory allocation utils, some containers, file IO and some bitmap
 * handling. to use the code include this file and #define GEDO_IMPLEMENTATION
 * before including it. defining GEDO_IMPLEMENTATION in 2 or more cpp files will
 * cause a link error.
 */
#if !defined GEDO_H
#define GEDO_H

#if defined _WIN32
#define UNICODE
#define GEDO_OS_WINDOWS 1
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_MEAN_AND_LEAN
#define VC_EXTRALEAN
#include <windows.h>
#include <Rpc.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN
#endif // _WIN32

#if defined __linux__
#define GEDO_OS_LINUX 1
#include <uuid/uuid.h>
#include <stdio.h>
#endif // __linux__

#include <stdint.h>

#if !defined GEDO_ASSERT
#include <assert.h>
#define GEDO_ASSERT assert
#define GEDO_ASSERT_MSG(msg) GEDO_ASSERT(!msg)
#endif // GEDO_ASSERT

#if !defined GEDO_MALLOC
#include <stdlib.h> // malloc, free
#define GEDO_MALLOC malloc
#endif // GEDO_MALLOC

#if !defined GEDO_FREE
#include <stdlib.h> // malloc, free
#define GEDO_FREE free
#endif // GEDO_FREE

#if !defined GEDO_MEMSET
#include <string.h> // memset
#define GEDO_MEMSET memset
#endif // GEDO_MEMSET

#if !defined GEDO_MEMCPY
#include <string.h> // memcpy
#define GEDO_MEMCPY memcpy
#endif // GEDO_MEMCPY

template<typename F>
struct privDefer
{
    F f;
    privDefer(F f)
        : f(f)
    {
    }
    ~privDefer()
    {
        f();
    }
};

template<typename F>
privDefer<F>
defer_func(F f)
{
    return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code) auto DEFER_3(_defer_) = defer_func([&]() { code; })

//-----------------Algorithms----------------------------//
template <typename T>
size_t
ArrayCount(const T arr[])
{
    return sizeof(arr) / sizeof(T);
}

template<typename T>
T
Max(const T& t0, const T& t1)
{
    return (t0 > t1) ? t0 : t1;
}

template<typename T>
T
Min(const T& t0, const T& t1)
{
    return (t0 < t1) ? t0 : t1;
}

template<typename T>
T
Clamp(const T& t, const T& low, const T& high)
{
    return (t < low) ? low : (t > high) ? high : t;
}

template <typename T>
void
Swap(T& t0, T& t1)
{
    T t = t0;
    t0 = t1;
    t1 = t;
}

template <typename T, typename TPredicate>
static void
QuickSort(T* p, size_t size, TPredicate compare)
{
    /* threshold for transitioning to insertion sort */
    while (size > 12)
    {
        /* compute median of three */
        const size_t m = size >> 1;
        const bool c01 = compare(p[0], p[m]);
        const bool c12 = compare(p[m], p[size - 1]);
        /* if 0 >= mid >= end, or 0 < mid < end, then use mid */
        if (c01 != c12)
        {
            /* otherwise, we'll need to swap something else to middle */
            const bool c = compare(p[0], p[size - 1]);
            /* 0>mid && mid<size:  0>size => size; 0<size => 0    */
            /* 0<mid && mid>size:  0>size => 0;    0<size => size */
            const size_t z = (c == c12) ? 0 : size - 1;
            Swap(p[z], p[m]);
        }
        /* now p[m] is the median-of-three */
        /* swap it to the beginning so it won't move around */
        Swap(p[0], p[m]);

        /* partition loop */
        size_t i = 1;
        size_t j = size - 1;
        for (;;)
        {
            /* handling of equality is crucial here */
            /* for sentinels & efficiency with duplicates */
            for (;; ++i)
            {
                if (!compare(p[i], p[0])) break;
            }
            for (;; --j)
            {
                if (!compare(p[0], p[j])) break;
            }
            /* make sure we haven't crossed */
            if (i >= j) break;
            Swap(p[i], p[j]);

            ++i;
            --j;
        }
        /* recurse on smaller side, iterate on larger */
        if (j < (size - i))
        {
            QuickSort(p, j);
            p = p + i;
            size = size - i;
        }
        else
        {
            QuickSort(p + i, size - i);
            size = j;
        }
    }
}

template <typename T>
static void
QuickSort(T* p, size_t size)
{
    QuickSort(p, size,
              [](const T& a, const T& b)
              {
                  return a < b;
              });
}

template <typename T, typename TCompare, typename TPredicate>
static int64_t
BinarySearch(T* p, size_t size, TCompare compare, TPredicate predicate)
{
    size_t low = 0;
    size_t high = size - 1;
    while (low <= high)
    {
        const size_t mid = low + (high - low) / 2;

        // Check if key is present at mid 
        if (predicate(p[mid], key))
        {
            return mid;
        }

        if (compare(p[mid], key))
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    // if we reach here, then element was not present 
    return -1;
}

template <typename T>
static int64_t
BinarySearch(T* p, size_t size, const T& key)
{
    BinarySearch(p, size,
                 [](const T& a, const T& b)
                 {
                     return a < b;
                 },
                 [key](const T& a)
                 {
                     return a == key;
                 });
}
//--------------------------------------------------//

//-----------------UUID-----------------------------//
struct UUId
{
    uint8_t data[16] = {};
};

static UUId
GenerateUUID();

static bool
CompareUUID(const UUId& a, const UUId& b);

//-----------------Memory --------------------------//
struct MemoryBlock
{
    uint8_t* data = nullptr;
    size_t size = 0;
};

struct Allocator
{
    virtual void ResetAllocator() = 0;
    virtual MemoryBlock AllocateMemoryBlock(size_t bytes) = 0;
    virtual bool FreeMemoryBlock(MemoryBlock& block) = 0;
};

struct LinearAllocator : Allocator
{
    size_t offset = 0;
    MemoryBlock arena;

    void ResetAllocator() override;
    MemoryBlock AllocateMemoryBlock(size_t bytes) override;
    bool FreeMemoryBlock(MemoryBlock& block) override;
};

struct MallocAllocator : Allocator
{
    void ResetAllocator() override;
    MemoryBlock AllocateMemoryBlock(size_t bytes) override;
    bool FreeMemoryBlock(MemoryBlock& block) override;
};

static Allocator&
GetDefaultAllocator();

static void
SetDefaultAllocator(Allocator& allocator);

// memory util functions.
static bool
IsPointerInsideMemoryBlock(const uint8_t* ptr, MemoryBlock block);

static bool
IsMemoryBlockInside(MemoryBlock big, MemoryBlock small);

static void
ZeroMemoryBlock(MemoryBlock block);

static double
BytesToMegaBytes(size_t bytes);

static double
BytesToGigaBytes(size_t bytes);

static size_t
MegaBytesToBytes(size_t megabytes);

static size_t
GigaBytesToBytes(size_t gigabytes);
// end of memory util functions.

static LinearAllocator*
CreateLinearAllocator(size_t bytes);

static void
DestroyLinearAllocator(LinearAllocator* allocator);

static MallocAllocator*
CreateMallocAllocator();

static void
DestroyMallocAllocator(MallocAllocator* allocator);

//------------------------------------------------------------//

//--------------------------------File IO---------------------//
// File names are utf8 encoded.

static MemoryBlock
ReadFile(const char* fileName, Allocator& allocator = GetDefaultAllocator());

static bool
WriteFile(const char* fileName,
          MemoryBlock block,
          Allocator& allocator = GetDefaultAllocator());

static bool
DoesFileExist(const char* fileName,
              Allocator& allocator = GetDefaultAllocator());

static int64_t
GetFileSize(const char* fileName, Allocator& allocator = GetDefaultAllocator());

enum class PathType
{
    FAILURE,
    FILE,
    DIRECTORY
};

static PathType
GetPathType(const char* path, Allocator& allocator = GetDefaultAllocator());

//------------------------------------------------------------//

//------------------------------Containers--------------------//

template<typename T>
struct ArrayView
{
    const T* data = NULL;
    size_t size = 0;

    T* begin()
    {
        return data;
    }
    T* end()
    {
        return data + size;
    }
    const T* begin() const
    {
        return data;
    }
    const T* end() const
    {
        return data + size;
    }
};

template<typename T, size_t N>
struct StaticArray
{
    T vals[N] = {};
    size_t count = 0;

    T* data()
    {
        return vals;
    }
    const T* data() const
    {
        return vals;
    }
    size_t capacity() const
    {
        return N;
    }
    size_t size() const
    {
        return count;
    }
    T* begin()
    {
        return data();
    }
    T* end()
    {
        return data() + size();
    }
    const T* begin() const
    {
        return data();
    }
    const T* end() const
    {
        return data() + size();
    }
    T& operator[](size_t i)
    {
        GEDO_ASSERT(i < size());
        return data()[i];
    }
    const T& operator[](size_t i) const
    {
        GEDO_ASSERT(i < size());
        return data()[i];
    }
    void clear()
    {
        count = 0;
    }
    void push_back(const T& d)
    {
        GEDO_ASSERT(size() < N);
        data()[count++] = d;
    }
    void pop_back()
    {
        count--;
    }
    void resize(size_t s)
    {
        GEDO_ASSERT(s <= N);
        count = s;
    }
};

template<typename T>
struct Array
{
    Allocator* allocator = &GetDefaultAllocator();
    MemoryBlock block;
    size_t count = 0;

    Array() = default;
    ~Array()
    {
        if (block.data)
        {
            allocator->FreeMemoryBlock(block);
        }
    }
    Array(const Array& s)
    {
        allocator = s.allocator;
        block = allocator->AllocateMemoryBlock(s.block.size);
        GEDO_MEMCPY(block.data, s.block.data, block.size);
        count = s.count;
    }
    Array& operator=(const Array& s)
    {
        allocator = s.allocator;
        block = allocator->AllocateMemoryBlock(s.block.size);
        GEDO_MEMCPY(block.data, s.block.data, block.size);
        count = s.count;
        return *this;
    }
    Array(Array&& s) noexcept
    {
        allocator = s.allocator;
        block = s.block;
        count = s.count;
        s.allocator = NULL;
        s.block = MemoryBlock{};
        s.count = 0;
    }
    Array& operator=(Array&& s) noexcept
    {
        allocator = s.allocator;
        block = s.block;
        count = s.count;
        s.allocator = NULL;
        s.block = MemoryBlock{};
        s.count = 0;
        return *this;
    }
    T* data()
    {
        return (T*)block.data;
    }
    const T* data() const
    {
        return (const T*)block.data;
    }
    size_t capacity() const
    {
        return block.size / sizeof(T);
    }
    size_t size() const
    {
        return count;
    }
    T* begin()
    {
        return data();
    }
    T* end()
    {
        return data() + size();
    }
    const T* begin() const
    {
        return data();
    }
    const T* end() const
    {
        return data() + size();
    }
    T& operator[](size_t i)
    {
        GEDO_ASSERT(i < size());
        return data()[i];
    }
    const T& operator[](size_t i) const
    {
        GEDO_ASSERT(i < size());
        return data()[i];
    }
    void clear()
    {
        count = 0;
    }
    void push_back(const T& d)
    {
        if (size() >= capacity())
        {
            const size_t s = size();
            resize(s ? 2 * s : 8);
            count = s;
        }
        data()[count++] = d;
    }
    void pop_back()
    {
        count--;
    }
    void resize(size_t s)
    {
        if (capacity() < s)
        {
            reserve(s);
        }
        count = s;
    }
    void reserve(size_t s)
    {
        if (capacity() < s)
        {
            MemoryBlock newBlock = allocator->AllocateMemoryBlock(s * sizeof(T));
            if (block.data)
            {
                GEDO_MEMCPY(newBlock.data, block.data, block.size);
                allocator->FreeMemoryBlock(block);
            }
            block = newBlock;
        }
    }
};

template<typename TKey, typename TValue>
struct HashTable
{
};

template <typename T>
ArrayView<T> CreateArrayView(const T arr[])
{
    ArrayView<T> result;
    result.data = arr;
    result.size = ArrayCount(arr);
    return result;
}

//-------------------------------------------------------------//

//--------------------------Strings----------------------------//

static size_t
StringLength(const char* string);

struct StringView
{
    const char* data = NULL;
    size_t size = 0;

    const char* begin()
    {
        return data;
    }
    const char* end()
    {
        return data + size;
    }
    const char* begin() const
    {
        return data;
    }
    const char* end() const
    {
        return data + size;
    }
};

struct String
{
    Allocator* allocator = &GetDefaultAllocator();
    MemoryBlock block;
    size_t count = 0;

    String() = default;
    String(MemoryBlock b, Allocator& alloc = GetDefaultAllocator())
    {
        block = b;
        allocator = &alloc;
        count = StringLength((const char*)b.data);
    }
    String(const char* string, Allocator& alloc = GetDefaultAllocator())
    {
        allocator = &alloc;
        append(string);
    }
    String(const String& s)
    {
        allocator = s.allocator;
        block = allocator->AllocateMemoryBlock(s.block.size);
        GEDO_MEMCPY(block.data, s.block.data, block.size);
        count = s.count;
    }
    String(String&& s) noexcept
    {
        allocator = s.allocator;
        block = s.block;
        count = s.count;
        s.allocator = NULL;
        s.block = MemoryBlock{};
        s.count = 0;
    }
    String& operator=(const String& s)
    {
        allocator = s.allocator;
        block = allocator->AllocateMemoryBlock(s.block.size);
        GEDO_MEMCPY(block.data, s.block.data, block.size);
        count = s.count;
        return *this;
    }
    String& operator=(String&& s) noexcept
    {
        allocator = s.allocator;
        block = s.block;
        count = s.count;
        s.allocator = NULL;
        s.block = MemoryBlock{};
        s.count = 0;
        return *this;
    }
    ~String()
    {
        if (block.data)
        {
            allocator->FreeMemoryBlock(block);
        }
    }
    char* data()
    {
        return (char*)block.data;
    }
    const char* data() const
    {
        return (const char*)block.data;
    }
    size_t capacity() const
    {
        return block.size;
    }
    size_t size() const
    {
        return count;
    }
    char* begin()
    {
        return data();
    }
    char* end()
    {
        return data() + size();
    }
    const char* begin() const
    {
        return data();
    }
    const char* end() const
    {
        return data() + size();
    }
    char& operator[](size_t i)
    {
        GEDO_ASSERT(i < size());
        return data()[i];
    }
    const char& operator[](size_t i) const
    {
        GEDO_ASSERT(i < size());
        return data()[i];
    }
    void clear()
    {
        count = 0;
    }
    void resize(size_t s)
    {
        if (capacity() < s)
        {
            reserve(s);
        }
        count = s;
    }
    void reserve(size_t s)
    {
        const size_t c = capacity();
        if (c < s)
        {
            MemoryBlock newBlock = allocator->AllocateMemoryBlock(s + 1);
            ZeroMemoryBlock(newBlock);
            if (block.data)
            {
                GEDO_MEMCPY(newBlock.data, block.data, block.size);
                allocator->FreeMemoryBlock(block);
            }
            block = newBlock;
        }
    }
    void append(const char* string)
    {
        const size_t len = StringLength(string);
        reserve(count + StringLength(string));
        for (size_t i = 0; i < len; ++i)
        {
            data()[count++] = string[i];
        }
    }
    void append(char c)
    {
        if (size() >= capacity())
        {
            const size_t s = size();
            resize(s ? 2 * s : 8);
            count = s;
        }
        data()[count++] = c;
    }
};

static StringView
CreateStringView(const char* string);

static const char*
GetFileExtension(const char* string);

static bool
CompareStrings(const char* str1, const char* str2);

static bool
CompareStrings(const StringView str1, const StringView str2);

// if separator != 0 it will be added in between the strings.
// e.g.
//  String lines [] {"line1", line2};
//  String data = ConcatStrings(lines, 2, '\n');
//  GEDO_ASSERT(CompareStrings(data, "line1\nline2"));
static String
ConcatStrings(const String* strings, size_t stringsCount, char seperator = 0);

static Array<String>
SplitString(const char* string, char delim, Allocator& allocator = GetDefaultAllocator());

static Array<StringView>
SplitStringView(const char* string, char delim, Allocator& allocator = GetDefaultAllocator());

static Array<String>
SplitStringIntoLines(const char* string, Allocator& allocator = GetDefaultAllocator());

static Array<StringView>
SplitStringViewIntoLines(const char* string, char delim, Allocator& allocator = GetDefaultAllocator());

//-------------------------------------------------------------//

//------------------------------Bitmap-------------------------//
// 0xRRGGBBAA
struct Color
{
    uint8_t a = 0;
    uint8_t b = 0;
    uint8_t g = 0;
    uint8_t r = 0;
};

struct Rect
{
    size_t x = 0;
    size_t y = 0;
    size_t width = 0;
    size_t height = 0;
};

struct Bitmap
{
    size_t width = 0;
    size_t height = 0;
    uint8_t* data = NULL;
};

struct ColorBitmap
{
    size_t width = 0;
    size_t height = 0;
    Color* data = NULL;
};

static void
FillRectangle(ColorBitmap& dest, Rect fillArea, const ColorBitmap& src);

static void
FillRectangle(ColorBitmap& dest, Rect fillArea, const Bitmap& mask, Color c);

static void
FillRectangle(ColorBitmap& dest, Rect fillArea, Color color);

static Color
CreateColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

static Bitmap
CreateBitmap(size_t width,
             size_t height,
             Allocator& allocator = GetDefaultAllocator());

static void
DestoryBitmap(Bitmap& bitmap, Allocator& allocator = GetDefaultAllocator());

static ColorBitmap
CreateColorBitmap(size_t width,
                  size_t height,
                  Allocator& allocator = GetDefaultAllocator());

static void
DestoryColorBitmap(ColorBitmap& bitmap,
                   Allocator& allocator = GetDefaultAllocator());

static const Color RED = CreateColor(255, 0, 0, 255);
static const Color GREEN = CreateColor(0, 255, 0, 255);
static const Color GREEN_BLUE = CreateColor(78, 201, 176, 255);
static const Color BLUE = CreateColor(0, 0, 255, 255);
static const Color WHITE = CreateColor(255, 255, 255, 255);
static const Color BLACK = CreateColor(0, 0, 0, 255);
static const Color DARK_GREY = CreateColor(30, 30, 30, 255);
//------------------------------------------------------------//

#if defined GEDO_IMPLEMENTATION

static Allocator* defaultAllocator = CreateMallocAllocator();

static Allocator&
GetDefaultAllocator()
{
    return *defaultAllocator;
}

void
SetDefaultAllocator(Allocator& allocator)
{
    defaultAllocator = &allocator;
}

bool
IsPointerInsideMemoryBlock(const uint8_t* ptr, MemoryBlock block)
{
    const uint8_t* begin = block.data;
    const uint8_t* end = block.data + block.size;
    return (ptr >= begin && ptr < end);
}

bool
IsMemoryBlockInside(MemoryBlock big, MemoryBlock small)
{
    return IsPointerInsideMemoryBlock(small.data, big) &&
        IsPointerInsideMemoryBlock(small.data + small.size, big);
}

void
ZeroMemoryBlock(MemoryBlock block)
{
    GEDO_MEMSET(block.data, 0, block.size);
}

double
BytesToMegaBytes(size_t bytes)
{
    return bytes / double(1024ULL * 1024ULL);
}

double
BytesToGigaBytes(size_t bytes)
{
    return bytes / double(1024ULL * 1024ULL * 1024ULL);
}

size_t
MegaBytesToBytes(size_t megabytes)
{
    return (megabytes * 1024ULL * 1024ULL);
}

size_t
GigaBytesToBytes(size_t gigabytes)
{
    return (gigabytes * 1024ULL * 1024ULL * 1024ULL);
}

LinearAllocator*
CreateLinearAllocator(size_t bytes)
{
    LinearAllocator* allocator = new LinearAllocator();
    allocator->arena.data = (uint8_t*)GEDO_MALLOC(bytes);
    allocator->arena.size = bytes;
    ZeroMemoryBlock(allocator->arena);
    return allocator;
}

void
DestroyLinearAllocator(LinearAllocator* allocator)
{
    GEDO_ASSERT(allocator);
    GEDO_ASSERT(allocator->arena.data);
    allocator->offset = 0;
    GEDO_FREE(allocator->arena.data);
    allocator->arena = MemoryBlock{};
    delete allocator;
}

void
LinearAllocator::ResetAllocator()
{
    offset = 0;
}

MemoryBlock
LinearAllocator::AllocateMemoryBlock(size_t bytes)
{
    MemoryBlock result;
    if ((offset + bytes) <= arena.size)
    {
        result.size = bytes;
        result.data = offset + arena.data;
        offset += bytes;
        ZeroMemoryBlock(result);
        return result;
    }
    GEDO_ASSERT_MSG("don't have enough space.");
    return result;
}

bool
LinearAllocator::FreeMemoryBlock(MemoryBlock& block)
{
    // no-op for the allocator.
    if (IsMemoryBlockInside(arena, block))
    {
        block.size = 0;
        block.data = NULL;
        return true;
    }
    return false;
}

MallocAllocator*
CreateMallocAllocator()
{
    MallocAllocator* allocator = new MallocAllocator();
    return allocator;
}

void
DestroyMallocAllocator(MallocAllocator* allocator)
{
    GEDO_ASSERT(allocator);
    delete allocator;
}

void
MallocAllocator::ResetAllocator()
{
}

MemoryBlock
MallocAllocator::AllocateMemoryBlock(size_t bytes)
{
    MemoryBlock result;
    result.data = (uint8_t*)GEDO_MALLOC(bytes);
    if (!result.data)
    {
        GEDO_ASSERT_MSG("don't have enough space.");
    }
    result.size = bytes;
    ZeroMemoryBlock(result);
    return result;
}

bool
MallocAllocator::FreeMemoryBlock(MemoryBlock& block)
{
    GEDO_ASSERT(block.data);
    GEDO_FREE(block.data);
    block.size = 0;
    block.data = NULL;
    return true;
}

//-----------------------------------------------------------//

//-------------------------Bitmap manipulation---------------//
void
FillRectangle(ColorBitmap& dest, Rect fillArea, const ColorBitmap& src)
{
    uint32_t srcIdx = 0;
    for (uint32_t y = fillArea.y; y < (fillArea.y + fillArea.height); ++y)
    {
        for (uint32_t x = fillArea.x; x < (fillArea.x + fillArea.width); ++x)
        {
            const uint32_t index = y * dest.width + x;
            dest.data[index] = src.data[srcIdx++];
        }
    }
}

void
FillRectangle(ColorBitmap& dest, Rect fillArea, const Bitmap& mask, Color c)
{
    uint32_t srcIdx = 0;
    for (uint32_t y = fillArea.y; y < (fillArea.y + fillArea.height); ++y)
    {
        for (uint32_t x = fillArea.x; x < (fillArea.x + fillArea.width); ++x)
        {
            const uint32_t index = y * dest.width + x;
            if (mask.data[srcIdx++])
            {
                dest.data[index] = c;
            }
        }
    }
}

void
FillRectangle(ColorBitmap& dest, Rect fillArea, Color color)
{
    for (uint32_t y = fillArea.y; y < (fillArea.y + fillArea.height); ++y)
    {
        for (uint32_t x = fillArea.x; x < (fillArea.x + fillArea.width); ++x)
        {
            const uint32_t index = y * dest.width + x;
            dest.data[index] = color;
        }
    }
}

Color
CreateColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    Color c;
    c.r = red;
    c.g = green;
    c.b = blue;
    c.a = alpha;
    return c;
}

ColorBitmap
CreateColorBitmap(size_t width, size_t height, Allocator& allocator)
{
    ColorBitmap result;
    result.width = width;
    result.height = height;
    MemoryBlock block =
        allocator.AllocateMemoryBlock(sizeof(Color) * width * height);
    GEDO_ASSERT(block.data);
    result.data = (Color*)block.data;
    return result;
}

void
DestoryColorBitmap(ColorBitmap& bitmap, Allocator& allocator)
{
    GEDO_ASSERT(bitmap.data);
    MemoryBlock block;
    block.data = (uint8_t*)bitmap.data;
    block.size = bitmap.width * bitmap.height * sizeof(Color);
    allocator.FreeMemoryBlock(block);
    bitmap.data = NULL;
}

Bitmap
CreateBitmap(size_t width, size_t height, Allocator& allocator)
{
    Bitmap result;
    result.width = width;
    result.height = height;
    MemoryBlock block =
        allocator.AllocateMemoryBlock(sizeof(uint8_t) * width * height);
    GEDO_ASSERT(block.data);
    result.data = (uint8_t*)block.data;
    return result;
}

void
DestoryBitmap(Bitmap& bitmap, Allocator& allocator)
{
    GEDO_ASSERT(bitmap.data);
    MemoryBlock block;
    block.data = (uint8_t*)bitmap.data;
    block.size = bitmap.width * bitmap.height * sizeof(uint8_t);
    allocator.FreeMemoryBlock(block);
    bitmap.data = NULL;
}

//-----------------------------------------------------------//

//--------------------------------File IO---------------------//
#if defined GEDO_OS_WINDOWS
static MemoryBlock
UTF8ToUTF16(const char* fileName, Allocator& allocator)
{
    const size_t len = strlen(fileName);
    const size_t wlen =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fileName, len, NULL, 0);
    MemoryBlock data =
        allocator.AllocateMemoryBlock(sizeof(wchar_t) * (wlen + 1));
    wchar_t* utf16 = (wchar_t*)data.data;
    utf16[wlen] = 0;
    MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, fileName, len, utf16, wlen);
    return data;
}

static HANDLE
GetFileHandle(const char* fileName, bool read, Allocator& allocator)
{
    if (!fileName)
    {
        return NULL;
    }
    MemoryBlock utf16Memory = UTF8ToUTF16(fileName, allocator);
    defer(allocator.FreeMemoryBlock(utf16Memory));
    wchar_t* text = (wchar_t*)utf16Memory.data;
    HANDLE handle = NULL;
    if (read)
    {
        handle =
            CreateFile(text,            // file to open
                       GENERIC_READ,    // open for reading
                       FILE_SHARE_READ, // share for reading
                       NULL,            // default security
                       OPEN_EXISTING,   // existing file only
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
                       NULL);
    }
    else
    {
        handle = CreateFile(text,                  // name of the write
                            GENERIC_WRITE,         // open for writing
                            0,                     // do not share
                            NULL,                  // default security
                            CREATE_NEW,            // create new file only
                            FILE_ATTRIBUTE_NORMAL, // normal file
                            NULL);
    }
    return handle;
}

MemoryBlock
ReadFile(const char* fileName, Allocator& allocator)
{
    MemoryBlock result;
    const int64_t fileSize = GetFileSize(fileName);
    if (fileSize < 0)
    {
        return result;
    }
    result = allocator.AllocateMemoryBlock(fileSize + 1);
    HANDLE handle = GetFileHandle(fileName, true, allocator);
    defer(CloseHandle(handle));
    OVERLAPPED overlapped = {};
    const bool success = ReadFileEx(handle,
                                    result.data,
                                    result.size - 1,
                                    &overlapped,
                                    NULL);
    GEDO_ASSERT(success);
    return result;
}

bool
WriteFile(const char* fileName, MemoryBlock block, Allocator& allocator)
{
    HANDLE handle = GetFileHandle(fileName, false, allocator);
    if (handle)
    {
        defer(CloseHandle(handle));
        OVERLAPPED overlapped = {};
        return WriteFileEx(handle, block.data, block.size - 1, &overlapped, NULL);
    }
    return false;
}

bool
DoesFileExist(const char* fileName, Allocator& allocator)
{
    HANDLE handle = GetFileHandle(fileName, true, allocator);
    const bool found = handle != INVALID_HANDLE_VALUE;
    if (found)
    {
        CloseHandle(handle);
    }
    return found;
}

int64_t
GetFileSize(const char* fileName, Allocator& allocator)
{
    HANDLE handle = GetFileHandle(fileName, true, allocator);
    const bool found = handle != INVALID_HANDLE_VALUE;
    if (found)
    {
        LARGE_INTEGER size;
        GetFileSizeEx(handle, &size);
        CloseHandle(handle);
        return size.QuadPart;
    }
    return -1;
}

PathType
GetPathType(const char* path, Allocator& allocator)
{
    HANDLE handle = GetFileHandle(path, true, allocator);
    const bool found = handle != INVALID_HANDLE_VALUE;
    if (found)
    {
        defer(CloseHandle(handle));
        FILE_BASIC_INFO basicInfo;
        GetFileInformationByHandleEx(
            handle, FileBasicInfo, &basicInfo, sizeof(basicInfo));
        return (basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            ? PathType::DIRECTORY
            : PathType::FILE;
    }
    return PathType::FAILURE;
}

#elif defined GEDO_OS_LINUX

MemoryBlock
ReadFile(const char* fileName, Allocator& allocator)
{
    FILE* fp = fopen(filename, 'r');
    MemoryBlock result;
    if (fp)
    {
        defer(fclose(fp));
        fseek(fp, 0, SEEK_END);
        const int64_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        result = allocator.AllocateMemoryBlock(size + 1);
        fread(result.data, 1, size, fp);
        result.data[size] = 0;
    }
    return result;
}

bool
WriteFile(const char* fileName, MemoryBlock block, Allocator& allocator)
{
    FILE* fp = fopen(filename, 'w');
    MemoryBlock result;
    if (fp)
    {
        defer(fclose(fp));
        fwrite(result.data, 1, result.size, fp);
        return true;
    }
    return false;
}

bool
DoesFileExist(const char* fileName, Allocator& allocator)
{
    FILE* fp = fopen(filename, 'r');
    if (fp)
    {
        fclose(fp);
        return true;
    }
    return false;
}

int64_t
GetFileSize(const char* fileName, Allocator& allocator)
{
    FILE* fp = fopen(filename, 'r');
    if (fp)
    {
        defer(fclose(fp));
        fseek(fp, 0, SEEK_END);
        const int64_t size = ftell(fp);
        return size;
    }
    return -1;
}

PathType
GetPathType(const char* path, Allocator& allocator)
{
    struct stat s;
    if (stat(path, &s) == 0)
    {
        if (s.st_mode & S_IFDIR)
        {
            return PathType::DIRECTORY;
        }
        else if (s.st_mode & S_IFREG)
        {
            return PathType::FILE;
        }
    }
    return PathType::ERROR;
}
#endif
//------------------------------------------------------------//

//------------------Strings----------------------------------//
size_t
StringLength(const char* string)
{
    size_t result = 0;
    while (*string++)
    {
        result++;
    }
    return result;
}

const char*
GetFileExtension(const char* string)
{
    const size_t len = StringLength(string);
    if (!len)
    {
        return NULL;
    }

    for (int64_t i = len - 1; i >= 0; --i)
    {
        if (string[i] == '.')
        {
            return string + i;
        }
    }
    return NULL;
}

bool
CompareStrings(const StringView str1, const StringView str2)
{
    if (str1.size != str2.size)
    {
        return false;
    }
    for (size_t i = 0; i < str1.size; ++i)
    {
        if (str1.data[i] != str2.data[i])
        {
            return false;
        }
    }
    return true;
}

bool
CompareStrings(const char* str1, const char* str2)
{
    const size_t len1 = StringLength(str1);
    const size_t len2 = StringLength(str2);
    if (len1 != len2)
    {
        return false;
    }
    for (size_t i = 0; i < len1; ++i)
    {
        if (str1[i] != str2[i])
        {
            return false;
        }
    }
    return true;
}

String
ConcatStrings(const String* strings, size_t stringsCount, char seperator)
{
    const bool addSeperator = seperator != 0;
    size_t resultSize = 0;
    for (size_t i = 0; i < stringsCount; ++i)
    {
        resultSize += strings[i].size();
    }
    if (addSeperator)
    {
        resultSize += (stringsCount - 1);
    }

    String result;
    result.reserve(resultSize);
    for (size_t i = 0; i < stringsCount; ++i)
    {
        result.append(strings[i].data());
        if (addSeperator && i != stringsCount - 1)
        {
            result.append(seperator);
        }
    }
    return result;
}

static String
CopyString(const char* string, size_t from, size_t to, Allocator& allocator)
{
    String s;
    s.allocator = &allocator;
    s.reserve(to - from + 1);
    for (size_t i = from; i < to; ++i)
    {
        s.append(string[i]);
    }
    return s;
}

StringView
CreateStringView(const char* string)
{
    StringView result;
    result.data = string;
    result.size = StringLength(string);
    return result;
}

static Array<StringView>
SplitStringView(const char* string, char delim, Allocator& allocator)
{
    Array<StringView> result;
    result.allocator = &allocator;
    const size_t len = StringLength(string);
    size_t previousOffset = 0;
    size_t i = 0;

    size_t partsCount = 0;
    while (i < len)
    {
        if (string[i] == delim)
        {
            if (i - previousOffset)
            {
                partsCount++;
            }
            while (string[i] == delim && i < len)
            {
                i++;
            }
            previousOffset = i;
        }
        else
        {
            i++;
        }
    }
    if (i - previousOffset)
    {
        partsCount++;
    }
    result.reserve(partsCount);

    previousOffset = 0;
    i = 0;
    while (i < len)
    {
        if (string[i] == delim)
        {
            const size_t slen = i - previousOffset;
            if (slen)
            {
                StringView l;
                l.data = string + previousOffset;
                l.size = slen;
                result.push_back(l);
            }
            while (string[i] == delim && i < len)
            {
                i++;
            }
            previousOffset = i;
        }
        else
        {
            i++;
        }
    }
    const size_t slen = i - previousOffset;
    if (slen)
    {
        StringView l;
        l.data = string + previousOffset;
        l.size = slen;
        result.push_back(l);
    }
    return result;
}

Array<String>
SplitString(const char* string, char delim, Allocator& allocator)
{
    Array<String> result;
    result.allocator = &allocator;
    const size_t len = StringLength(string);
    size_t previousOffset = 0;
    size_t i = 0;
    size_t partsCount = 0;
    while (i < len)
    {
        if (string[i] == delim)
        {
            if (i - previousOffset)
            {
                partsCount++;
            }
            while (string[i] == delim && i < len)
            {
                i++;
            }
            previousOffset = i;
        }
        else
        {
            i++;
        }
    }
    if (i - previousOffset)
    {
        partsCount++;
    }
    result.reserve(partsCount);

    i = 0;
    previousOffset = 0;
    while (i < len)
    {
        if (string[i] == delim)
        {
            const size_t slen = i - previousOffset;
            if (slen)
            {
                String l = CopyString(string, previousOffset, i, allocator);
                result.push_back(l);
            }
            while (string[i] == delim && i < len)
            {
                i++;
            }
            previousOffset = i;
        }
        else
        {
            i++;
        }
    }

    const size_t slen = i - previousOffset;
    if (slen)
    {
        String l = CopyString(string, previousOffset, i, allocator);
        result.push_back(l);
    }
    return result;
}

static Array<String>
SplitStringIntoLines(const char* string, Allocator& allocator)
{
    Array<String> result;
    result.allocator = &allocator;
    const size_t len = StringLength(string);
    size_t partsCount = 0;
    for (size_t i = 0; i < len; ++i)
    {
        if (string[i] == '\n')
        {
            partsCount++;
        }
    }
    result.reserve(partsCount);

    size_t previousOffset = 0;
    for (size_t i = 0; i < len; ++i)
    {
        if (string[i] == '\n')
        {
            if (i - previousOffset)
            {
                String l = CopyString(string, previousOffset, i, allocator);
                result.push_back(l);
            }
            previousOffset = i;
        }
    }
    if (len - previousOffset)
    {
        String l = CopyString(string, previousOffset, len, allocator);
        result.push_back(l);
    }
    return result;
}

static Array<StringView>
SplitStringViewIntoLines(const char* string, char delim, Allocator& allocator)
{
    Array<StringView> result;
    result.allocator = &allocator;
    const size_t len = StringLength(string);
    size_t partsCount = 0;
    for (size_t i = 0; i < len; ++i)
    {
        if (string[i] == '\n')
        {
            partsCount++;
        }
    }
    result.reserve(partsCount);

    size_t previousOffset = 0;
    for (size_t i = 0; i < len; ++i)
    {
        if (string[i] == '\n')
        {
            if (i - previousOffset)
            {
                StringView l;
                l.data = string + previousOffset;
                l.size = i - previousOffset;
                result.push_back(l);
            }
            previousOffset = i;
        }
    }
    if (len - previousOffset)
    {
        StringView l;
        l.data = string + previousOffset;
        l.size = len - previousOffset;
        result.push_back(l);
    }
    return result;
}
//------------------------------------------------------------//

//--------------------UUID------------------------------------//
#if defined (GEDO_OS_WINDOWS)
UUId
GenerateUUID()
{
    GUID uuid;
    UuidCreate(&uuid);

    UUId result
    {
        (uint8_t)((uuid.Data1 >> 24) & 0xFF),
        (uint8_t)((uuid.Data1 >> 16) & 0xFF),
        (uint8_t)((uuid.Data1 >> 8) & 0xFF),
        (uint8_t)((uuid.Data1) & 0xff),

        (uint8_t)((uuid.Data2 >> 8) & 0xFF),
        (uint8_t)((uuid.Data2) & 0xff),

        (uint8_t)((uuid.Data3 >> 8) & 0xFF),
        (uint8_t)((uuid.Data3) & 0xFF),

        (uint8_t)uuid.Data4[0],
        (uint8_t)uuid.Data4[1],
        (uint8_t)uuid.Data4[2],
        (uint8_t)uuid.Data4[3],
        (uint8_t)uuid.Data4[4],
        (uint8_t)uuid.Data4[5],
        (uint8_t)uuid.Data4[6],
        (uint8_t)uuid.Data4[7]
    };
    return result;
}

#elif defined (GEDO_OS_LINUX)
UUId
GenerateUUID()
{
    UUId result;
    uuid_generate(result.data);
}
#endif
bool
CompareUUID(const UUId& a, const UUId& b)
{
    for (size_t i = 0; i < 16; ++i)
    {
        if (a.data[i] != b.data[i])
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------//
#endif // GEDO_IMPLEMENTATION

#endif // GEDO_H

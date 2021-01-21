/*
 * Author: Ahmed Abdelhay
 * A collection of functions used to make programming easier.
 * contains memory allocation utils, some containers, file IO and some bitmap
 * handling. to use the code include this file and #define GEDO_IMPLEMENTATION
 * before including it. defining GEDO_IMPLEMENTATION in 2 or more cpp files will
 * cause a link error.
 * Users can define GEDO_DYNAMIC_LIBRARY to build as a shared library instead of a static library.
 * List of features:
 * - Utils:
 *  - defer macro: similar to the defer keyword in go this can be used to execute some action at
 *      the end of a scope.
 *      example:
 *          void* data = malloc(size);
 *          defer(free(data));  // this code will be executed at the end of the current scope.
 * - General algorithms:
 *      - Min,Max,Clamp
 *      - ArrayCount: get the count of a constant sized c array.
 *      - QuickSort.
 *      - BinarySearch.
 * - Memory utils:
 *      provide Allocator interface that proved Allocate and Free functions,
 *      it also provides some ready implementations allocators:
 *      - Malloc allocator: the default c stdlib allocator.
 *      - Arena allocator:  simple linear allocator that allocates block upfront and keep using it,
 *          this is very useful if the user wants in temp allocations where the user knows upfront what
 *          is the size that they will be using.
 *      it also provides a default allocator where the user can set it and it will be used in
 *      all the functions in this library by default.
 *      e.g.
 *           Allocator* alloc = CreateMyCustomAllocator(...);
 *           SetDefaultAllocator(alloc);
 *           MemoryBlock block = ReadFile(fileName); // this memory block will be allocated from alloc.
 *      it also provides some conversion between units utils:
 *           - BytesToMegaBytes(size_t bytes);
 *           - BytesToGigaBytes(size_t bytes);
 *           - MegaBytesToBytes(size_t megabytes);
 *           - GigaBytesToBytes(size_t gigabytes);
 * - Containers:
 *      - ArrayView<T>      a non owning view of array of type T.
 *      - StaticArray<T,N>  owning stretchy array of type T allocated on the stack with max size N.
 *      - Array<T,N>        owning stretchy array of type T allocated using Allocator*.
 *      - HashTable<T>      TODO.
 * - Maths:
 *      - Math code uses double not float.
 *      - 2D/3D Vector.
 *      - 3x3 Matrix and 4x4 Matrix.
 *      - operator overloading for +,*,- between different types.
 *      - Matrix/vector and Matrix/Matrix multiplication support.
 *      - Common geometrical operation like DotProduct and CrossProduct, Normalise, Transpose, Rotate.
 *      - commonly used functions in computer graphics like Perspective and lookAt.
 *
 *      TODO: Add SIMD support.
 * - UUID:
 *      Provides a cross platform UUID generation function and compare.
 * - File I/O:
 *      Provides a cross platform way of handling files.
 *      - Read whole file           ReadFile(const char* fileName, Allocator& allocator);
 *      - Write whole file          WriteFile(const char* fileName, MemoryBlock block, Allocator& allocator);
 *      - Check if a file exists    DoesFileExist(const char* fileName, Allocator& allocator);
 *      - Get the file size         GetFileSize(const char* fileName, Allocator& allocator);
 *      - Check a path type         GetPathType(const char* path, Allocator& allocator);
 * - Strings:
 *      Provides custom implementation of both String (owning container) and StringView (non owning view).
 *      it uses the Allocator* interface for managing memory
 *      it also provides some useful functions like:
 *          - StringLength(const char* string);
 *          - GetFileExtension(const char* string);
 *          - CompareStrings(const char* str1, const char* str2);
 *          - CompareStrings(const StringView str1, const StringView str2);
 *          - ConcatStrings(const String* strings, size_t stringsCount, char seperator);
 *          - SplitString(const char* string, char delim, Allocator& allocator);
 *          - SplitStringView(const char* string, char delim, Allocator& allocator);
 *          - SplitStringIntoLines(const char* string, Allocator& allocator);
 *          - SplitStringViewIntoLines(const char* string, char delim, Allocator& allocator);
 * - Bitmaps:
 *      Provide a way of creating bitmap (colored and mono) and blit data to the bitmap,
 *      it also provide some util for creating colors, rect, and define some common colors.
 *        Blit functions:
 *            FillRectangle(ColorBitmap& dest, Rect fillArea, const ColorBitmap& src);
 *            FillRectangle(ColorBitmap& dest, Rect fillArea, const Bitmap& mask, Color c);
 *            FillRectangle(ColorBitmap& dest, Rect fillArea, Color color);
 *
 */

#pragma once

#include <stdint.h>
#include <math.h>

#if defined _WIN32
#define UNICODE
#define GEDO_OS_WINDOWS 1
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_MEAN_AND_LEAN
#define VC_EXTRALEAN
#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include <windows.h>
#include <Rpc.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN
#elif defined __linux__
#define GEDO_OS_LINUX 1
#include <uuid/uuid.h> // user will have to link against libuuid.
#include <sys/stat.h>
#include <stdio.h>
#else
#error "Not supported OS"
#endif

#if !defined GEDO_ASSERT
#include <assert.h>
#define GEDO_ASSERT assert
#endif // GEDO_ASSERT

#define GEDO_ASSERT_MSG(msg) GEDO_ASSERT(!msg)

#if !defined GEDO_MALLOC
#include <stdlib.h> // malloc, free
#include <string.h> // memset

#define GEDO_MALLOC malloc
#define GEDO_FREE free
#define GEDO_MEMSET memset
#define GEDO_MEMCPY memcpy
#endif // GEDO_MALLOC

#if defined (GEDO_DYNAMIC_LIBRARY)
 // dynamic library
#if defined (GEDO_OS_WINDOWS)
#if defined (GEDO_IMPLEMENTATION)
#define GEDO_DEF __declspec(dllexport) 
#else
#define GEDO_DEF __declspec(dllimport) 
#endif
#else // ! GEDO_OS_WINDOWS
#define GEDO_DEF 
#endif // GEDO_OS_WINDOWS
#else 
 // static library
#define GEDO_DEF static
#endif

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

namespace gedo
{
    //-----------------Algorithms----------------------------//
    template <typename T>
    size_t ArrayCount(const T arr[])
    {
        return sizeof(arr) / sizeof(T);
    }

    template<typename T>
    T Max(const T& t0, const T& t1)
    {
        return (t0 > t1) ? t0 : t1;
    }

    template<typename T>
    T Min(const T& t0, const T& t1)
    {
        return (t0 < t1) ? t0 : t1;
    }

    template<typename T>
    T Clamp(const T& t, const T& low, const T& high)
    {
        return (t < low) ? low : (t > high) ? high : t;
    }

    template <typename T>
    void Swap(T& t0, T& t1)
    {
        T t = t0;
        t0 = t1;
        t1 = t;
    }

    template <typename T, typename TPredicate>
    void QuickSort(T* p, size_t size, TPredicate compare)
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
    void QuickSort(T* p, size_t size)
    {
        QuickSort(p, size,
                  [](const T& a, const T& b)
                  {
                      return a < b;
                  });
    }

    template <typename T, typename TCompare, typename TPredicate>
    int64_t BinarySearch(T* p, size_t size, const T& key, TCompare compare, TPredicate predicate)
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
    int64_t BinarySearch(T* p, size_t size, const T& key)
    {
        return BinarySearch(p, size, key,
                            [](const T& a, const T& b)
                            {
                                return a < b;
                            },
                            [](const T& a, const T& b)
                            {
                                return a == b;
                            });
    }
    //--------------------------------------------------//

    //------------------Math----------------------------//
    GEDO_DEF const double PI = 3.14159265358979323846264338327950288;

    union Vec2d
    {
        struct
        {
            double x, y;
        };
        double data[2];
    };

    union Vec3d
    {
        struct
        {
            double x, y, z;
        };
        double data[3];
    };

    // the matrices are column major.
    union Mat3
    {
        double elements[3][3];
        double data[9];
    };

    union Mat4
    {
        double elements[4][4];
        double data[16];
    };

    GEDO_DEF double Deg2Rad(double v);
    GEDO_DEF double Rad2Deg(double v);

    GEDO_DEF Vec2d operator+(const Vec2d a, const Vec2d& b);
    GEDO_DEF Vec2d operator-(const Vec2d a, const Vec2d& b);
    GEDO_DEF Vec2d operator*(const Vec2d a, const Vec2d& b);
    GEDO_DEF Vec2d operator*(const Vec2d a, double x);
    GEDO_DEF Vec2d operator*(double x, const Vec2d a);
    GEDO_DEF Vec3d operator+(const Vec3d a, const Vec3d& b);
    GEDO_DEF Vec3d operator-(const Vec3d a, const Vec3d& b);
    GEDO_DEF Vec3d operator*(const Vec3d a, const Vec3d& b);
    GEDO_DEF Vec3d operator*(const Vec3d a, double x);
    GEDO_DEF Vec3d operator*(double x, const Vec3d a);
    GEDO_DEF Vec3d operator*(const Mat3 a, const Vec3d& v);
    GEDO_DEF Mat4 operator*(const Mat4& left, const Mat4& right);
    GEDO_DEF Mat4 operator*(Mat4 left, double x);
    GEDO_DEF Vec3d CrossProduct(const Vec3d a, const Vec3d& b);
    GEDO_DEF double DotProduct(const Vec3d a, const Vec3d& b);
    GEDO_DEF double DotProduct(const Vec2d a, const Vec2d& b);
    GEDO_DEF double DotProduct(const double a[4], const double b[4]);
    GEDO_DEF double Length(const Vec3d& v);
    GEDO_DEF double Length(const Vec2d& v);
    GEDO_DEF void Normalise(Vec3d& v);
    GEDO_DEF void Normalise(Vec2d& v);
    GEDO_DEF Vec3d Normalised(const Vec3d& v);
    GEDO_DEF Vec2d Normalised(const Vec2d& v);
    GEDO_DEF Mat3 Transpose(Mat3 m);
    GEDO_DEF Mat4 Transpose(Mat4 m);
    GEDO_DEF Mat4 Identity();
    GEDO_DEF Mat4 Translate(const Mat4& m, Vec3d translation);
    GEDO_DEF Mat4 Rotate(const Mat4& m, double angle, Vec3d v);

    /*
     * @param[in] fovy    Specifies the field of view angle, in degrees, in the y direction.
     * @param[in] aspect  Specifies the aspect ratio that determines the field of view in the x direction. The aspect ratio is the ratio of x (width) to y (height).
     * @param[in] zNear   Specifies the distance from the viewer to the near clipping plane (always positive).
     * @param[in] zFar    Specifies the distance from the viewer to the far clipping plane (always positive).
    */
    GEDO_DEF Mat4 Perspective(double fovy, double aspect, double zNear, double zFar);

    /*!
     * @brief set up view matrix
     *
     * NOTE: The UP vector must not be parallel to the line of sight from
     *       the eye point to the reference point
     *
     * @param[in]  eye    eye vector
     * @param[in]  center center vector
     * @param[in]  up     up vector
     * @param[out] dest   result matrix
     */
    GEDO_DEF Mat4 LookAt(const Vec3d& eye, const Vec3d& center, const Vec3d& up);
    //--------------------------------------------------//

    //-----------------UUID-----------------------------//
    struct UUId
    {
        uint8_t data[16] = {};
    };

    GEDO_DEF UUId GenerateUUID();
    GEDO_DEF bool CompareUUID(const UUId& a, const UUId& b);
    //--------------------------------------------------//

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

    GEDO_DEF Allocator& GetDefaultAllocator();
    GEDO_DEF void SetDefaultAllocator(Allocator& allocator);

    // memory util functions.
    GEDO_DEF bool IsPointerInsideMemoryBlock(const uint8_t* ptr, MemoryBlock block);
    GEDO_DEF bool IsMemoryBlockInside(MemoryBlock big, MemoryBlock small);
    GEDO_DEF void ZeroMemoryBlock(MemoryBlock block);
    GEDO_DEF double BytesToMegaBytes(size_t bytes);
    GEDO_DEF double BytesToGigaBytes(size_t bytes);
    GEDO_DEF size_t MegaBytesToBytes(size_t megabytes);
    GEDO_DEF size_t GigaBytesToBytes(size_t gigabytes);
    // end of memory util functions.

    GEDO_DEF LinearAllocator* CreateLinearAllocator(size_t bytes);
    GEDO_DEF void DestroyLinearAllocator(LinearAllocator* allocator);

    GEDO_DEF MallocAllocator* CreateMallocAllocator();
    GEDO_DEF void DestroyMallocAllocator(MallocAllocator* allocator);
    //------------------------------------------------------------//

    //--------------------------------File IO---------------------//
    enum class PathType
    {
        FAILURE,
        FILE,
        DIRECTORY
    };

    // File names are utf8 encoded.
    GEDO_DEF MemoryBlock ReadFile(const char* fileName, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF bool WriteFile(const char* fileName, MemoryBlock block, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF bool DoesFileExist(const char* fileName, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF int64_t GetFileSize(const char* fileName, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF PathType GetPathType(const char* path, Allocator& allocator = GetDefaultAllocator());
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
    // Can be used when parsing a file.
    struct StreamBuffer
    {
        const char* data = NULL;
        size_t size = 0;
        size_t cursor = 0;
    };

    GEDO_DEF size_t StringLength(const char* string);

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

    GEDO_DEF StringView CreateStringView(const char* string);
    GEDO_DEF const char* GetFileExtension(const char* string);
    GEDO_DEF bool CompareStrings(const char* str1, const char* str2);
    GEDO_DEF bool CompareStrings(const StringView str1, const StringView str2);

    // if separator != 0 it will be added in between the strings.
    // e.g.
    //  String lines [] {"line1", line2};
    //  String data = ConcatStrings(lines, 2, '\n');
    //  GEDO_ASSERT(CompareStrings(data, "line1\nline2"));
    GEDO_DEF String ConcatStrings(const String* strings, size_t stringsCount, char seperator = 0);
    GEDO_DEF Array<String> SplitString(const char* string, char delim, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF Array<StringView> SplitStringView(const char* string, char delim, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF Array<String> SplitStringIntoLines(const char* string, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF Array<StringView> SplitStringViewIntoLines(const char* string, char delim, Allocator& allocator = GetDefaultAllocator());
    //-------------------------------------------------------------//

    //------------------------------Bitmap-------------------------//
    struct Color
    {
        // 0xRRGGBBAA
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

    GEDO_DEF void FillRectangle(ColorBitmap& dest, Rect fillArea, const ColorBitmap& src);
    GEDO_DEF void FillRectangle(ColorBitmap& dest, Rect fillArea, const Bitmap& mask, Color c);
    GEDO_DEF void FillRectangle(ColorBitmap& dest, Rect fillArea, Color color);

    GEDO_DEF Bitmap CreateBitmap(size_t width, size_t height, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF void DestoryBitmap(Bitmap& bitmap, Allocator& allocator = GetDefaultAllocator());

    GEDO_DEF ColorBitmap CreateColorBitmap(size_t width, size_t height, Allocator& allocator = GetDefaultAllocator());
    GEDO_DEF void DestoryColorBitmap(ColorBitmap& bitmap, Allocator& allocator = GetDefaultAllocator());

    GEDO_DEF Color CreateColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

    GEDO_DEF const Color RED = CreateColor(255, 0, 0, 255);
    GEDO_DEF const Color GREEN = CreateColor(0, 255, 0, 255);
    GEDO_DEF const Color GREEN_BLUE = CreateColor(78, 201, 176, 255);
    GEDO_DEF const Color BLUE = CreateColor(0, 0, 255, 255);
    GEDO_DEF const Color WHITE = CreateColor(255, 255, 255, 255);
    GEDO_DEF const Color BLACK = CreateColor(0, 0, 0, 255);
    GEDO_DEF const Color DARK_GREY = CreateColor(30, 30, 30, 255);
    //------------------------------------------------------------//

#if defined GEDO_IMPLEMENTATION

    //----------------------------Memory-------------------------//
    static Allocator* defaultAllocator = CreateMallocAllocator();

    Allocator& GetDefaultAllocator()
    {
        return *defaultAllocator;
    }

    void SetDefaultAllocator(Allocator& allocator)
    {
        defaultAllocator = &allocator;
    }

    bool IsPointerInsideMemoryBlock(const uint8_t* ptr, MemoryBlock block)
    {
        const uint8_t* begin = block.data;
        const uint8_t* end = block.data + block.size;
        return (ptr >= begin && ptr < end);
    }

    bool IsMemoryBlockInside(MemoryBlock big, MemoryBlock small)
    {
        return IsPointerInsideMemoryBlock(small.data, big) &&
            IsPointerInsideMemoryBlock(small.data + small.size, big);
    }

    void ZeroMemoryBlock(MemoryBlock block)
    {
        GEDO_MEMSET(block.data, 0, block.size);
    }

    double BytesToMegaBytes(size_t bytes)
    {
        return bytes / double(1024ULL * 1024ULL);
    }

    double BytesToGigaBytes(size_t bytes)
    {
        return bytes / double(1024ULL * 1024ULL * 1024ULL);
    }

    size_t MegaBytesToBytes(size_t megabytes)
    {
        return (megabytes * 1024ULL * 1024ULL);
    }

    size_t GigaBytesToBytes(size_t gigabytes)
    {
        return (gigabytes * 1024ULL * 1024ULL * 1024ULL);
    }

    LinearAllocator* CreateLinearAllocator(size_t bytes)
    {
        LinearAllocator* allocator = new LinearAllocator();
        allocator->arena.data = (uint8_t*)GEDO_MALLOC(bytes);
        allocator->arena.size = bytes;
        ZeroMemoryBlock(allocator->arena);
        return allocator;
    }

    void DestroyLinearAllocator(LinearAllocator* allocator)
    {
        GEDO_ASSERT(allocator);
        GEDO_ASSERT(allocator->arena.data);
        allocator->offset = 0;
        GEDO_FREE(allocator->arena.data);
        allocator->arena = MemoryBlock{};
        delete allocator;
    }

    void LinearAllocator::ResetAllocator()
    {
        offset = 0;
    }

    MemoryBlock LinearAllocator::AllocateMemoryBlock(size_t bytes)
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

    bool LinearAllocator::FreeMemoryBlock(MemoryBlock& block)
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

    MallocAllocator* CreateMallocAllocator()
    {
        MallocAllocator* allocator = new MallocAllocator();
        return allocator;
    }

    void DestroyMallocAllocator(MallocAllocator* allocator)
    {
        GEDO_ASSERT(allocator);
        delete allocator;
    }

    void MallocAllocator::ResetAllocator()
    {
    }

    MemoryBlock MallocAllocator::AllocateMemoryBlock(size_t bytes)
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

    bool MallocAllocator::FreeMemoryBlock(MemoryBlock& block)
    {
        GEDO_ASSERT(block.data);
        GEDO_FREE(block.data);
        block.size = 0;
        block.data = NULL;
        return true;
    }
    //-----------------------------------------------------------//

    //-------------------------Bitmap manipulation---------------//
    void FillRectangle(ColorBitmap& dest, Rect fillArea, const ColorBitmap& src)
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

    void FillRectangle(ColorBitmap& dest, Rect fillArea, const Bitmap& mask, Color c)
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

    void FillRectangle(ColorBitmap& dest, Rect fillArea, Color color)
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

    Color CreateColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
    {
        Color c;
        c.r = red;
        c.g = green;
        c.b = blue;
        c.a = alpha;
        return c;
    }

    ColorBitmap CreateColorBitmap(size_t width, size_t height, Allocator& allocator)
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

    void DestoryColorBitmap(ColorBitmap& bitmap, Allocator& allocator)
    {
        GEDO_ASSERT(bitmap.data);
        MemoryBlock block;
        block.data = (uint8_t*)bitmap.data;
        block.size = bitmap.width * bitmap.height * sizeof(Color);
        allocator.FreeMemoryBlock(block);
        bitmap.data = NULL;
    }

    Bitmap CreateBitmap(size_t width, size_t height, Allocator& allocator)
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

    void DestoryBitmap(Bitmap& bitmap, Allocator& allocator)
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
    static MemoryBlock UTF8ToUTF16(const char* fileName, Allocator& allocator)
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

    static HANDLE GetFileHandle(const char* fileName, bool read, Allocator& allocator)
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

    MemoryBlock ReadFile(const char* fileName, Allocator& allocator)
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

    bool WriteFile(const char* fileName, MemoryBlock block, Allocator& allocator)
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

    bool DoesFileExist(const char* fileName, Allocator& allocator)
    {
        HANDLE handle = GetFileHandle(fileName, true, allocator);
        const bool found = handle != INVALID_HANDLE_VALUE;
        if (found)
        {
            CloseHandle(handle);
        }
        return found;
    }

    int64_t GetFileSize(const char* fileName, Allocator& allocator)
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

    PathType GetPathType(const char* path, Allocator& allocator)
    {
        HANDLE handle = GetFileHandle(path, true, allocator);
        const bool found = handle != INVALID_HANDLE_VALUE;
        if (found)
        {
            defer(CloseHandle(handle));
            FILE_BASIC_INFO basicInfo;
            GetFileInformationByHandleEx(handle, FileBasicInfo, &basicInfo, sizeof(basicInfo));
            return (basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                ? PathType::DIRECTORY
                : PathType::FILE;
        }
        return PathType::FAILURE;
    }
#elif defined GEDO_OS_LINUX
    MemoryBlock ReadFile(const char* fileName, Allocator& allocator)
    {
        FILE* fp = fopen(fileName, "r");
        MemoryBlock result;
        if (fp)
        {
            fseek(fp, 0, SEEK_END);
            const int64_t size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            result = allocator.AllocateMemoryBlock(size + 1);
            fread(result.data, 1, size, fp);
            result.data[size] = 0;
            fclose(fp);
        }
        return result;
    }

    bool WriteFile(const char* fileName, MemoryBlock block, Allocator& allocator)
    {
        FILE* fp = fopen(fileName, "w");
        MemoryBlock result;
        if (fp)
        {
            fwrite(result.data, 1, result.size, fp);
            fclose(fp);
            return true;
        }
        return false;
    }

    bool DoesFileExist(const char* fileName, Allocator& allocator)
    {
        FILE* fp = fopen(fileName, "r");
        if (fp)
        {
            fclose(fp);
            return true;
        }
        return false;
    }

    int64_t GetFileSize(const char* fileName, Allocator& allocator)
    {
        FILE* fp = fopen(fileName, "r");
        if (fp)
        {
            fseek(fp, 0, SEEK_END);
            fclose(fp);
            const int64_t size = ftell(fp);
            return size;
        }
        return -1;
    }

    PathType GetPathType(const char* path, Allocator& allocator)
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
        return PathType::FAILURE;
    }
#endif
    //------------------------------------------------------------//

    //------------------Strings----------------------------------//
    static String CopyString(const char* string, size_t from, size_t to, Allocator& allocator)
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

    size_t StringLength(const char* string)
    {
        size_t result = 0;
        while (*string++)
        {
            result++;
        }
        return result;
    }

    const char* GetFileExtension(const char* string)
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

    bool CompareStrings(const StringView str1, const StringView str2)
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

    bool CompareStrings(const char* str1, const char* str2)
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

    String ConcatStrings(const String* strings, size_t stringsCount, char seperator)
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

    StringView CreateStringView(const char* string)
    {
        StringView result;
        result.data = string;
        result.size = StringLength(string);
        return result;
    }

    Array<StringView> SplitStringView(const char* string, char delim, Allocator& allocator)
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

    Array<String> SplitString(const char* string, char delim, Allocator& allocator)
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

    Array<String> SplitStringIntoLines(const char* string, Allocator& allocator)
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

    Array<StringView> SplitStringViewIntoLines(const char* string, char delim, Allocator& allocator)
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
    UUId GenerateUUID()
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
    UUId GenerateUUID()
    {
        UUId result;
        uuid_generate(result.data);
        return result;
    }
#endif
    bool CompareUUID(const UUId& a, const UUId& b)
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

    //--------------------Math-----------------------------------//
    Vec2d operator+(const Vec2d a, const Vec2d& b)
    {
        return Vec2d{ a.x + b.x, a.y + b.y };
    }

    Vec2d operator-(const Vec2d a, const Vec2d& b)
    {
        return Vec2d{ a.x - b.x, a.y - b.y };
    }

    Vec2d operator*(const Vec2d a, const Vec2d& b)
    {
        return Vec2d{ a.x * b.x, a.y * b.y };
    }

    Vec2d operator*(const Vec2d a, double x)
    {
        return Vec2d{ a.x * x, a.y * x };
    }

    Vec2d operator*(double x, const Vec2d a)
    {
        return Vec2d{ a.x * x, a.y * x };
    }

    Vec3d operator+(const Vec3d a, const Vec3d& b)
    {
        return Vec3d{ a.x + b.x, a.y + b.y, a.z + b.z };
    }

    Vec3d operator-(const Vec3d a, const Vec3d& b)
    {
        return Vec3d{ a.x - b.x, a.y - b.y, a.z - b.z };
    }

    Vec3d operator*(const Vec3d a, const Vec3d& b)
    {
        return Vec3d{ a.x * b.x, a.y * b.y, a.z * b.z };
    }

    Vec3d operator*(const Vec3d a, double x)
    {
        return Vec3d{ a.x * x, a.y * x, a.z * x };
    }

    Vec3d operator*(double x, const Vec3d a)
    {
        return Vec3d{ a.x * x, a.y * x, a.z * x };
    }

    double DotProduct(const Vec3d a, const Vec3d& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    double DotProduct(const Vec2d a, const Vec2d& b)
    {
        return a.x * b.x + a.y * b.y;
    }

    double DotProduct(const double a[4], const double b[4])
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    }

    Mat4 operator*(const Mat4& left, const Mat4& right)
    {
        Mat4 result;
        for (int columns = 0; columns < 4; ++columns)
        {
            for (int rows = 0; rows < 4; ++rows)
            {
                double sum = 0;
                for (int currentMatrice = 0; currentMatrice < 4; ++currentMatrice)
                {
                    sum += left.elements[currentMatrice][rows] * right.elements[columns][currentMatrice];
                }
                result.elements[columns][rows] = sum;
            }
        }
        return result;
    }

    Mat4 operator*(Mat4 left, double x)
    {
        for (int columns = 0; columns < 4; ++columns)
        {
            for (int rows = 0; rows < 4; ++rows)
            {
                left.elements[columns][rows] *= x;
            }
        }
        return left;
    }

    Vec3d operator*(const Mat3 a, const Vec3d& v)
    {
        Vec3d r;
        for (int rows = 0; rows < 3; ++rows)
        {
            double sum = 0;
            for (int columns = 0; columns < 3; ++columns)
            {
                sum += a.elements[columns][rows] * v.data[columns];
            }
            r.data[rows] = sum;
        }
        return r;
    }

    Vec3d CrossProduct(const Vec3d a, const Vec3d& b)
    {
        return Vec3d{ a.y * b.z - a.z * b.y,
                      a.z * b.x - a.x * b.z,
                      a.x * b.y - a.y * b.x };
    }

    double Length(const Vec2d& v)
    {
        return sqrtf(v.x * v.x + v.y * v.y);
    }

    double Length(const Vec3d& v)
    {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    void Normalise(Vec2d& v)
    {
        const double length = Length(v);
        v.x /= length;
        v.y /= length;
    }

    Vec2d Normalised(const Vec2d& v)
    {
        const double length = Length(v);
        return Vec2d{ v.x / length, v.y / length };
    }

    void Normalise(Vec3d& v)
    {
        const double length = Length(v);
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }

    Vec3d Normalised(const Vec3d& v)
    {
        const double length = Length(v);
        return Vec3d{ v.x / length, v.y / length, v.z / length };
    }

    Mat4 Transpose(Mat4 m)
    {
        Swap(m.elements[0][1], m.elements[1][0]);
        Swap(m.elements[0][2], m.elements[2][0]);
        Swap(m.elements[0][3], m.elements[3][0]);
        Swap(m.elements[1][2], m.elements[2][1]);
        Swap(m.elements[1][3], m.elements[3][1]);
        Swap(m.elements[2][3], m.elements[3][2]);
        return m;
    }

    Mat3 Transpose(Mat3 m)
    {
        Swap(m.elements[0][1], m.elements[1][0]);
        Swap(m.elements[0][2], m.elements[2][0]);
        Swap(m.elements[1][2], m.elements[2][1]);
        return m;
    }

    Mat4 Translate(const Mat4& m, Vec3d translation)
    {
        Mat4 translate = Identity();
        translate.elements[3][0] = translation.x;
        translate.elements[3][1] = translation.y;
        translate.elements[3][2] = translation.z;
        return m * translate;
    }

    Mat4 Rotate(const Mat4& m, double angle, Vec3d axis)
    {
        const double sinTheta = sinf(angle);
        const double cosTheta = cosf(angle);
        const double cosValue = 1.0f - cosTheta;

        Normalise(axis);
        Mat4 rotate = Identity();

        rotate.elements[0][0] = (axis.x * axis.x * cosValue) + cosTheta;
        rotate.elements[0][1] = (axis.x * axis.y * cosValue) + (axis.z * sinTheta);
        rotate.elements[0][2] = (axis.x * axis.z * cosValue) - (axis.y * sinTheta);

        rotate.elements[1][0] = (axis.y * axis.x * cosValue) - (axis.z * sinTheta);
        rotate.elements[1][1] = (axis.y * axis.y * cosValue) + cosTheta;
        rotate.elements[1][2] = (axis.y * axis.z * cosValue) + (axis.x * sinTheta);

        rotate.elements[2][0] = (axis.z * axis.x * cosValue) + (axis.y * sinTheta);
        rotate.elements[2][1] = (axis.z * axis.y * cosValue) - (axis.x * sinTheta);
        rotate.elements[2][2] = (axis.z * axis.z * cosValue) + cosTheta;

        return m * rotate;
    }

    Mat4 Identity()
    {
        Mat4 m = { 0 };
        m.elements[0][0] = m.elements[1][1] = m.elements[2][2] = m.elements[3][3] = 1.0f;
        return m;
    }

    Mat4 LookAt(const Vec3d& eye, const Vec3d& center, const Vec3d& up)
    {
        const Vec3d f = Normalised(center - eye);
        const Vec3d s = Normalised(CrossProduct(f, up));
        const Vec3d u = CrossProduct(s, f);

        Mat4 dest;
        dest.elements[0][0] = s.x;
        dest.elements[0][1] = u.x;
        dest.elements[0][2] = -f.x;
        dest.elements[0][3] = 0;

        dest.elements[1][0] = s.y;
        dest.elements[1][1] = u.y;
        dest.elements[1][2] = -f.y;
        dest.elements[1][3] = 0;

        dest.elements[2][0] = s.z;
        dest.elements[2][1] = u.z;
        dest.elements[2][2] = -f.z;
        dest.elements[2][3] = 0;

        dest.elements[3][0] = -DotProduct(s, eye);
        dest.elements[3][1] = -DotProduct(u, eye);
        dest.elements[3][2] = DotProduct(f, eye);
        dest.elements[3][3] = 1.0f;
        return dest;
    }

    Mat4 Perspective(double fovy, double aspect, double zNear, double zFar)
    {
        const double f = 1.0f / tanf(fovy * 0.5f);
        const double fn = 1.0f / (zNear - zFar);
        Mat4 dest = { 0 };
        dest.elements[0][0] = f / aspect;
        dest.elements[1][1] = f;
        dest.elements[2][2] = (zNear + zFar) * fn;
        dest.elements[2][3] = -1.0f;
        dest.elements[3][2] = 2.0f * zNear * zFar * fn;
        return dest;
    }

    double Deg2Rad(double v)
    {
        return (PI / 180.0) * v;
    }

    double Rad2Deg(double v)
    {
        return (180.0 / PI) * v;
    }
    //----------------------------------------------------------//
#endif // GEDO_IMPLEMENTATION
}

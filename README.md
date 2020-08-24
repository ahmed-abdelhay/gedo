# gedo
Common programming utilities for fast prototyping with focus on performance and compile time.
The code will work on windows and linux.

TODO:
Common utils:
1- stretchy buffer.
2- allocators.
  - arena allocator.
  - stack allocator.
  - heap allocator.
3- hash table.
4- map.
5- file i/o.
6- thread pool.
7- string operations.
8- dynamic library loading.
9- stop watch to measure code blocks.
10- defer.

Math:
1- 2D and 3D vector operations.
2- matrix operations.

Geometry:
1- triangle mesh data structure.
2- mesh i/o.

The code will compile to a static library, clients can then use cmake to use it.

example:
target_link_library(ClientProject PUBLIC gedo)

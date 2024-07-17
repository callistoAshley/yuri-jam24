#pragma once

#include <stdint.h>
#include <stddef.h>

// rust integer types because c's are a fucking mess
typedef float f32;
typedef double f64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ptrdiff_t isize; // signed version of size_t

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize; // size_t is unsigned, so we use it for usize

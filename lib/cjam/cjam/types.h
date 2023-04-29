#pragma once

#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "macros.h"

// length of compile time known array
#define ARRLEN(_arr) ((sizeof((_arr))) / ((sizeof((_arr)[0]))))

// basic fixed-width numeric types
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t usize;
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intptr_t isize;
typedef float f32;
typedef double f64;

typedef u64 hash_type;

#define U8_MIN UINT8_MIN
#define U16_MIN UINT16_MIN
#define U32_MIN UINT32_MIN
#define U64_MIN UINT64_MIN
#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define USIZE_MIN UINTPTR_MIN
#define USIZE_MAX UINTPTR_MAX

#define I8_MIN INT8_MIN
#define I16_MIN INT16_MIN
#define I32_MIN INT32_MIN
#define I64_MIN INT64_MIN
#define I8_MAX INT8_MAX
#define I16_MAX INT16_MAX
#define I32_MAX INT32_MAX
#define I64_MAX INT64_MAX

#define ISIZE_MIN INTPTR_MIN
#define ISIZE_MAX INTPTR_MAX

#define PRIusize PRIuPTR
#define PRIisize PRIiPTR

#define PRIint "d"

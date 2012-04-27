#pragma once

#define array_size(array) (sizeof(array)/sizeof(array[0]))

#ifndef __cplusplus

typedef enum _bool_e
{
	false = 0,
	true = 1
} bool;

#endif

#define max(a,b) ((a)>(b)?(a):(b))

//struct Closure(

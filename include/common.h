#ifndef COMMON_H
#define COMMON_H

#ifdef BUILD_IMAGE
#define PARALLEL_FOR
#else
#define PARALLEL_FOR _Pragma("omp parallel for")
#endif

#endif
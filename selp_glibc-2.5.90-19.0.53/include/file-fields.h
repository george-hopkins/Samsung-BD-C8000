#ifndef FILE_FIELDS_H
#define FILE_FIELDS_H 1

#include <stdint.h>

#ifndef CROSS_FILE
/* The cross version of this file defines file_FOO structures that have
   the same size, alignment and endianness as the target-machine integers
   used here.  */
typedef unsigned int file_uint;
typedef int file_int;
typedef int16_t file_int16_t;
typedef uint16_t file_uint16_t;
typedef int32_t file_int32_t;
typedef uint32_t file_uint32_t;
typedef int64_t file_int64_t;
typedef uint64_t file_uint64_t;

/* Return the value of FIELD, which has a file_* type.  */
#define READ_FIELD(FIELD) (FIELD)

/* Set the value of FIELD to VALUE, where FIELD has a file_* type.  */
#define WRITE_FIELD(FIELD, VALUE) ((FIELD) = (VALUE))

/* Return the integer type associated with file_* type X.  */
#define NATIVE_FIELD(X) X

/* Return the integer type associated with ElfW(TYPE).  */
#define HostElfW(type)	NATIVE_FIELD(ElfW(type))
#endif

#endif

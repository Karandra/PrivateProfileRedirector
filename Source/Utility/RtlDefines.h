#pragma once
#include "stdafx.h"

//#include <ntdef.h>
//#include <ntstatus.h>

//////////////////////////////////////////////////////////////////////////
// NTSTATUS
//////////////////////////////////////////////////////////////////////////

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

#if _WIN32_WINNT >= 0x0600
typedef CONST NTSTATUS *PCNTSTATUS;
#endif

//////////////////////////////////////////////////////////////////////////
// UNICODE_STRING
//////////////////////////////////////////////////////////////////////////

// Unicode strings are counted 16-bit character strings. If they are
// NULL terminated, Length does not include trailing NULL.
typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	#ifdef MIDL_PASS
	[size_is(MaximumLength / 2), length_is((Length) / 2)] USHORT * Buffer;
	#else // MIDL_PASS
	_Field_size_bytes_part_opt_(MaximumLength, Length) PWCH   Buffer;
	#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

#pragma once

#ifndef _CC_VA_H_
#define _CC_VA_H_


#if VER_EXPORTS
#define EAPI(rettype) __declspec(dllexport) rettype __cdecl
#else
#define EAPI(rettype) __declspec(dllimport) rettype __cdecl
#endif

#include <Windows.h>

EAPI(bool) VerifyAuthorityVasTest();

#endif
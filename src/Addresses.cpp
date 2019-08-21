#include "Addresses.h"
#include <Hooking.Patterns.h>
#include "Hooking.Helper.h"

void* const Addresses::CCoronas_Draw = hook::get_pattern("44 89 4C 24 ? 48 83 EC 28 0F 29 74 24 ?");

void* const Addresses::CCoronas_Instance =
	hook::get_absolute_address(hook::get_pattern("F3 41 0F 59 DD 48 8D 0D ? ? ? ? F3 0F 11 44 24 ?", 8));

void* const Addresses::CScriptIM_DrawLine = hook::get_pattern("48 8B DA 48 8B F9 E8 ? ? ? ? 84 C0 74 3F", -0xF);
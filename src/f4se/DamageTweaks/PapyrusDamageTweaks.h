#pragma once
#include "DamageTweaks.h"

#define SCRIPTNAME_Globals "F4TF:Globals"


struct StaticFunctionTag;
class VirtualMachine;


namespace PapyrusDamageTweaks
{
	void RegisterFuncs(VirtualMachine* vm);
}


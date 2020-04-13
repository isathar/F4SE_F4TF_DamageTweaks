#pragma once
#include "f4se/GameData.h"
#include "f4se/GameObjects.h"
#include "f4se/GameReferences.h"
#include "f4se/GameEvents.h"
#include "f4se/NiObjects.h"
#include "f4se/NiNodes.h"

#include <shlobj.h>
#include <string>
#include <set>
#include <vector>
#include <algorithm>

#include "RNG.h"


class BGSAttackData;

// DamageFrame - credit: kassent (https://github.com/kassent/FloatingDamage)
struct DamageFrame
{
	NiPoint3				hitLocation;				// 00
	UInt32					pad0C;						// 0C
	float					unk10[8];					// 10 - 2 vector[4]s: [0]=direction, [1]=direction for projectiles, distance or velocity for beams?
	bhkNPCollisionObject	* collisionObj;				// 30
	UInt64					unk38;						// 38
	UInt32					attackerHandle;				// 40
	UInt32					victimHandle;				// 44
	UInt64					unk48[1];					// 48 - filled with attackerHandle if source is null + actual dmg source is a creature weapon
	BGSAttackData			* attackData;				// 50 - filled if source is a melee/unarmed weapon, creature attack, or bash with a ranged weapon
	TESForm					* damageSourceForm;			// 58 - source weapon form
	TBO_InstanceData		* instanceData;				// 60 - source weapon instance
	UInt64					unk68[3];					// 68
	TESAmmo					* ammo;						// 80 - null for melee weapons and gun bash attacks
	void					* unk88;					// 88
	float					damage2;					// 90 - final damage
	float					unk94;						// 94 - damage before resistances
	float					damage;						// 98 - unk94 * distanceMult - mitigatedDmg (or 0 if beam projectile)
};
STATIC_ASSERT(sizeof(DamageFrame) == 0xA0);


/** native HasKeyword/GetVirtualFunction
		credit: shavkacagarikia (https://github.com/shavkacagarikia/ExtraItemInfo) **/
typedef bool(*_IKeywordFormBase_HasKeyword)(IKeywordFormBase* keywordFormBase, BGSKeyword* keyword, UInt32 unk3);

template <typename T>
T GetVirtualFunction(void* baseObject, int vtblIndex) {
	uintptr_t* vtbl = reinterpret_cast<uintptr_t**>(baseObject)[0];
	return reinterpret_cast<T>(vtbl[vtblIndex]);
}


// stores a damageType's variables
struct DmgTypeData
{
	UInt32			damageTypeID =			0;			// the damageType's formID
	UInt8			critTableID =			0;			// crit table ID to use
	ActorValueInfo	* damageResist =		nullptr;	// damage resistance ActorValue
	ActorValueInfo	* damageThreshold =		nullptr;	// damage threshold ActorValue
	BGSKeyword		* dmgOverrideKW =		nullptr;	// weapon keyword used to override its damage types
	BGSKeyword		* dmgImmuneKW =			nullptr;
	float			fArmorDmgResistExponent = 0.365;	// exponent to use in the default DR formula
	float			fArmorDmgFactor =		0.15;		// factor to use in the default DR formula
	std::string		dmgTypeName =			"";			// name to show in logs
};


// stores a damageType for use in damage calculations
struct DmgTypeStats
{
	UInt32			damageTypeID =			0;			// formID of the damageType
	int				iCritTableID =			-1;
	float			fDmgAmount =			0.0;		// current dmg
	float			fBaseDmg =				0.0;		// starting dmg
	float			fDTVal =				0.0;		// damage threshold
	float			fDRVal =				0.0;		// damage resistance
	float			fArmorPen =				0.0;		// armor penetration amount
	float			fTargetArmorMult =		1.0;
	float			fDRExponent =			0.365;		// DR exponent
	float			fDmgFactor =			0.15;		// DR factor
};


// Critical Effect Table
class CritEffectTable
{
public:
	// critical effect
	struct CritEffect
	{
		int				iRollMax =				0;			// upper threshold for the crit roll (search key)
		SpellItem		* critEffect =			nullptr;	// critical effect spell
		SpellItem		* weakCritEffect =		nullptr;	// critical effect to use if saving roll passed
		int				iSavingRollAVIndex =	-1;			// ActorValue to check for in the saving roll (-1 for none)
		int				iSavingRollMod =		0;			// amount added to the AV for the saving roll
		std::string		sCritText =				"";			// text to show for critEffect
		std::string		sWeakCritText =			"";			// text to show for weakCritEffect
	};

	struct TypedCritTable
	{
		int				iCritTableID =			-1;
		std::string		sMenuName =				"";
		std::vector<CritEffect>	critEffects;
	};

	BGSKeyword		* requiredKW =			nullptr;	// actor KW required to use this table
	
	std::vector<TypedCritTable>	critEffects_Typed;		// damageType-based crit tables
};


class DTConfig
{
public:
	static ATxoroshiro128p rng;

	static std::vector<DmgTypeData> damageTypes;
	static std::vector<CritEffectTable> critEffectTables;
	static std::vector<ActorValueInfo*> specialAVs;


	static ActorValueInfo * avCritTableCheck;
	static ActorValueInfo * avCritRollMod;
	static ActorValueInfo * avTargetCritRollMod;
	static ActorValueInfo * avCritDmgMult;
	static ActorValueInfo * avCritChanceAdd;

	static ActorValueInfo * avWeaponCND;
	static ActorValueInfo * avArmorPenetration;
	static ActorValueInfo * avArmorPenetrationThrown;
	static ActorValueInfo * avTargetArmorMult;
	static ActorValueInfo * avTargetArmorMultThrown;

	// limb CND:
	static ActorValueInfo * avHeadCND;
	static ActorValueInfo * avTorsoCND;
	static ActorValueInfo * avArmLCND;
	static ActorValueInfo * avArmRCND;
	static ActorValueInfo * avLegLCND;
	static ActorValueInfo * avLegRCND;

	// limb cnd diff
	static ActorValueInfo * avLastHeadCND;
	static ActorValueInfo * avLastTorsoCND;
	static ActorValueInfo * avLastArmLCND;
	static ActorValueInfo * avLastArmRCND;
	static ActorValueInfo * avLastLegLCND;
	static ActorValueInfo * avLastLegRCND;

	// for unarmed creature weapons
	static BGSKeyword * kwAnimsUnarmed;
	static ActorValueInfo * avUnarmedDamage;

	// for ranged creature weapons
	static BGSKeyword * kwCrRanged;
	static ActorValueInfo * avCrRangedDamage;

	// min overall damage to always let through resistances
	static float fMinDmg;

	// Damage Resistance: 0=off, 1=Fallout4 (default), 2=Classic (percentage-based)
	static UInt8 iDRType;
	static float fMaxPercentDR;

	// Damage Threshold: 0=off, 1=Classic (before DR), 2=FNV (after DR)
	static UInt8 iDTType;
	static float fDTMinDmgPercent;

	// Critical Hits - 0=off, 1=dmg only, 2=dmg+effect, 3=dmg+effect+savingroll
	static bool bEnableCriticalHits;
	static bool bCritChanceUsesCND;
	static bool bCritDmgMultRoll;
	static bool bCritsUseEffectTables;
	static bool bCritsUseSavingRoll;

	// armor CND
	static bool bEnableArmorCND;
	static UInt8 iArmorDmgChance;
	static float fArmorDmgMultiplier;
	static ActorValueInfo * avDamageArmorCheck;
	static BGSKeyword * kwCanHaveArmorDamage;

	// config paths
	static std::string sConfigPathSettingsBase;
	static std::string sConfigPathSettingsCur;
	static std::string sConfigPathData;


	static bool LoadConfigData();

	static void UpdateSettings();
};


namespace DamageTweaks
{
	bool HasKeyword_Native(IKeywordFormBase * keywordBase, BGSKeyword * checkKW);

	// critical roll - returns a random critical effect spell
	SpellItem *GetCritSpell(Actor * target = nullptr);

	float CalcDamage(TESObjectREFR * attacker, TESObjectREFR * victim, TESObjectWEAP * weaponForm, TESObjectWEAP::InstanceData * weaponInstance, float fBaseDmg = 0.0, float fFinalDmgMult = 0.0);
	

	void DumpDamageFrame(DamageFrame * pDamageFrame);
}


namespace PapyrusDamageTweaks
{
	bool RegisterPapyrus(VirtualMachine * vm);
}

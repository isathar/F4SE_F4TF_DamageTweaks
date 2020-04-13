#include "PapyrusDamageTweaks.h"
#include "f4se/GameData.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameSettings.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusVM.h"
#include "f4se/GameRTTI.h"



struct FindPred_CritEffectTable_ByActorKW
{
	IKeywordFormBase * keywordBase = nullptr;
	FindPred_CritEffectTable_ByActorKW(IKeywordFormBase * keywordBase = nullptr) : keywordBase(keywordBase) {}
	bool operator () (const CritEffectTable & critTable) const
	{ return (keywordBase && critTable.requiredKW) ? DamageTweaks::HasKeyword_Native(keywordBase, critTable.requiredKW) : false; }
};

struct FindPred_TypedCritTable_ByID
{
	int tableID;
	FindPred_TypedCritTable_ByID(int tableID) : tableID(tableID) {}
	bool operator () (const CritEffectTable::TypedCritTable & critTable) const { return (tableID == critTable.iCritTableID); }
};

struct FindPred_CritEffect_ByRoll
{
	int iRoll;
	FindPred_CritEffect_ByRoll(UInt32 iRoll) : iRoll(iRoll) {}
	bool operator () (const CritEffectTable::CritEffect & critEffect) const { return (iRoll <= critEffect.iRollMax); }
};


// gets the typed crit table for target, performs a crit table roll and optional saving roll to get a crit effect
SpellItem * DamageTweaks::GetCritSpell(Actor * target)
{
	if (!target || DTConfig::critEffectTables.empty()) {
		_MESSAGE("  WARNING: DamageTweaks::GetCritSpell:  No Target or critEffectTables!");
		return nullptr;
	}
	// get the Actor type's tables
	std::vector<CritEffectTable>::iterator critEffectsVarIt = std::find_if(DTConfig::critEffectTables.begin(), DTConfig::critEffectTables.end(), FindPred_CritEffectTable_ByActorKW(&target->keywordFormBase));
	if (critEffectsVarIt == DTConfig::critEffectTables.end()) {
		_MESSAGE("  WARNING: DamageTweaks::GetCritSpell:  no crit effect table for this Actor Type!");
		return nullptr;
	}

	int iCritEffectIndex = (int)(target->actorValueOwner.GetValue(DTConfig::avCritTableCheck) - 1.0);
	
	if (iCritEffectIndex < 0) {
		_MESSAGE("  WARNING: DamageTweaks::GetCritSpell:  invalid iCritEffectIndex!");
		return nullptr;
	}
	_MESSAGE("  Crit effect table variation:  %s, Type Index:  %i", critEffectsVarIt->requiredKW->keyword.c_str(), iCritEffectIndex);

	// get the damage type's table
	std::vector<CritEffectTable::TypedCritTable>::iterator typedCritTableIt = std::find_if(critEffectsVarIt->critEffects_Typed.begin(), critEffectsVarIt->critEffects_Typed.end(), FindPred_TypedCritTable_ByID(iCritEffectIndex));
	if (typedCritTableIt == critEffectsVarIt->critEffects_Typed.end()) {
		_MESSAGE("  WARNING: DamageTweaks::GetCritSpell:  no table for this damageType index!");
		return nullptr;
	}

	std::vector<CritEffectTable::CritEffect> curEffectTable = typedCritTableIt->critEffects;
	if (curEffectTable.empty()) {
		_MESSAGE("  WARNING: DamageTweaks::GetCritSpell:  curEffectTable is empty!");
		return nullptr;
	}

	// get the crit effect for the roll value
	int iRoll = (int)target->actorValueOwner.GetValue(DTConfig::avTargetCritRollMod);
	_MESSAGE("  Critical Effect Roll:  %i", iRoll);
	std::vector<CritEffectTable::CritEffect>::iterator critEffectIt = std::find_if(curEffectTable.begin(), curEffectTable.end(), FindPred_CritEffect_ByRoll(iRoll));
	if (critEffectIt == curEffectTable.end()) {
		_MESSAGE("  WARNING: DamageTweaks::GetCritSpell:  no effect for this roll value!");
		return nullptr;
	}
	if (critEffectIt->critEffect) {
		if (DTConfig::bCritsUseSavingRoll && (critEffectIt->iSavingRollAVIndex > -1)) {
			// saving roll
			if (DTConfig::rng.RandomInt(1, 20) < ((int)target->actorValueOwner.GetValue(DTConfig::specialAVs[critEffectIt->iSavingRollAVIndex]) + critEffectIt->iSavingRollMod)) {
				_MESSAGE("    Saving roll failed - Critical Effect: %s,  target: %s\n", critEffectIt->critEffect->name.name.c_str(), target->GetFullName());
				return critEffectIt->critEffect;
			}
			else {
				// lesser crit effect
				if (critEffectIt->weakCritEffect) {
					_MESSAGE("    Saving roll passed - Critical Effect: %s,  target: %s\n", critEffectIt->weakCritEffect->name.name.c_str(), target->GetFullName());
					return critEffectIt->weakCritEffect;
				}
			}
		}
		else {
			_MESSAGE("    Critical Effect: %s,  target: %s\n", critEffectIt->critEffect->name.name.c_str(), target->GetFullName());
			return critEffectIt->critEffect;
		}
	}
	return nullptr;
}



namespace PapyrusDamageTweaks
{
	// returns a crit effect or null
	SpellItem * GetCritEffect(StaticFunctionTag*, Actor * target)
	{
		SpellItem * critSpell = nullptr;
		if (target) {
			critSpell = DamageTweaks::GetCritSpell(target);
			target->actorValueOwner.SetBase(DTConfig::avCritTableCheck, 0.0);
		}
		return critSpell;
	}


	TESForm * UpdateArmorCND(StaticFunctionTag*, Actor * ownerActor)
	{
		if (!ownerActor || !ownerActor->inventoryList || (ownerActor->inventoryList->items.count == 0)) {
			ownerActor->actorValueOwner.SetBase(DTConfig::avDamageArmorCheck, 0.0);
			return nullptr;
		}
		_MESSAGE("UpdateArmorCND called");
		TESForm * brokenArmor = nullptr;

		float fHeadDmg = ownerActor->actorValueOwner.GetValue(DTConfig::avLastHeadCND) - ownerActor->actorValueOwner.GetValue(DTConfig::avHeadCND);
		float fTorsoDmg = ownerActor->actorValueOwner.GetValue(DTConfig::avLastTorsoCND) - ownerActor->actorValueOwner.GetValue(DTConfig::avTorsoCND);
		float fArmLDmg = ownerActor->actorValueOwner.GetValue(DTConfig::avLastArmLCND) - ownerActor->actorValueOwner.GetValue(DTConfig::avArmLCND);
		float fArmRDmg = ownerActor->actorValueOwner.GetValue(DTConfig::avLastArmRCND) - ownerActor->actorValueOwner.GetValue(DTConfig::avArmRCND);
		float fLegLDmg = ownerActor->actorValueOwner.GetValue(DTConfig::avLastLegLCND) - ownerActor->actorValueOwner.GetValue(DTConfig::avLegLCND);
		float fLegRDmg = ownerActor->actorValueOwner.GetValue(DTConfig::avLastLegRCND) - ownerActor->actorValueOwner.GetValue(DTConfig::avLegRCND);

		bool bHead = fHeadDmg > 0.0;
		bool bTorso = fTorsoDmg > 0.0;
		bool bArmL = fArmLDmg > 0.0;
		bool bArmR = fArmRDmg > 0.0;
		bool bLegL = fLegLDmg > 0.0;
		bool bLegR = fLegRDmg > 0.0;

		// temp flags:
		UInt16 iFlagEquipped = 0x1;
		UInt32 iArmorSlotHairTop = 0x1;
		UInt32 iArmorSlotBody = 0x8;
		UInt32 iArmorSlotTorsoA = 0x800;
		UInt32 iArmorSlotLArmA = 0x1000;
		UInt32 iArmorSlotRArmA = 0x2000;
		UInt32 iArmorSlotLLegA = 0x4000;
		UInt32 iArmorSlotRLegA = 0x8000;

		if (bHead || bTorso || bArmL || bArmR || bLegL || bLegR) {
			UInt32 bipedFlags = 0;
			float fDmgAmount = ownerActor->actorValueOwner.GetValue(DTConfig::avDamageArmorCheck);
			float fActualHealth = 0.0;
			bool bDoDamage = false;

			for (UInt32 i = 0; i < ownerActor->inventoryList->items.count; i++) {
				BGSInventoryItem tempItem;
				if (ownerActor->inventoryList->items.GetNthItem(i, tempItem)) {
					if (tempItem.form && tempItem.form->formType == kFormType_ARMO) {
						tempItem.stack->Visit([&](BGSInventoryItem::Stack * stack)
						{
							ExtraDataList * stackData = stack->extraData;
							if (!stackData || ((stack->flags & iFlagEquipped) != iFlagEquipped)) {
								return true;
							}
							TESObjectARMO * armorForm = (TESObjectARMO*)tempItem.form;
							if (!armorForm) {
								return true;
							}
							
							bipedFlags = armorForm->bipedObject.data.parts;
							bDoDamage = false;
							// hairtop/helmet
							if (bHead && ((bipedFlags & iArmorSlotHairTop) == iArmorSlotHairTop)) {
								bDoDamage = true;
							}
							// outfit/bottom layer
							if (bTorso && ((bipedFlags & iArmorSlotBody) == iArmorSlotBody)) {
								bDoDamage = true;
							}
							// chest armor
							if (bTorso && ((bipedFlags & iArmorSlotTorsoA) == iArmorSlotTorsoA)) {
								bDoDamage = true;
							}
							// left arm armor
							if (bArmL && ((bipedFlags & iArmorSlotLArmA) == iArmorSlotLArmA)) {
								bDoDamage = true;
							}
							// right arm armor
							if (bArmR && ((bipedFlags & iArmorSlotRArmA) == iArmorSlotRArmA)) {
								bDoDamage = true;
							}
							// left leg armor
							if (bLegL && ((bipedFlags & iArmorSlotLLegA) == iArmorSlotLLegA)) {
								bDoDamage = true;
							}
							// right leg armor
							if (bLegR && ((bipedFlags & iArmorSlotRLegA) == iArmorSlotRLegA)) {
								bDoDamage = true;
							}

							// only continue if armor in this slot was damaged
							if (!bDoDamage) {
								return true;
							}

							TESObjectARMO::InstanceData * armorInstance = nullptr;
							BSExtraData * extraDataInst = stackData->GetByType(ExtraDataType::kExtraData_InstanceData);
							if (extraDataInst) {
								ExtraInstanceData * objectModData = DYNAMIC_CAST(extraDataInst, BSExtraData, ExtraInstanceData);
								if (objectModData) {
									armorInstance = (TESObjectARMO::InstanceData*)Runtime_DynamicCast(objectModData->instanceData, RTTI_TBO_InstanceData, RTTI_TESObjectARMO__InstanceData);
								}
							}
							if (!armorInstance) {
								armorInstance = &armorForm->instanceData;
							}
							if (armorInstance && (armorInstance->health != 0)) {
								BSExtraData * extraDataHealth = stackData->GetByType(ExtraDataType::kExtraData_Health);
								if (!extraDataHealth) {
									fActualHealth = (float)(int)armorInstance->health;
									ExtraHealth * newHealthData = ExtraHealth::Create((fActualHealth - fDmgAmount) / fActualHealth);
									stackData->Add(newHealthData->type, (BSExtraData*)newHealthData);
									extraDataHealth = stackData->GetByType(ExtraDataType::kExtraData_Health);
								}
								if (extraDataHealth) {
									ExtraHealth * healthData = DYNAMIC_CAST(extraDataHealth, BSExtraData, ExtraHealth);
									if (healthData) {
										fActualHealth = (float)(int)armorInstance->health * healthData->health;
										if (fActualHealth - fDmgAmount > 0.0) {
											healthData->health = (fActualHealth - fDmgAmount) / (float)(int)armorInstance->health;
											_MESSAGE("Damaged %s by %.4f - new CND: %.4f%%", armorForm->GetFullName(), fDmgAmount, healthData->health);
										}
										else {
											healthData->health = 0.0;
											brokenArmor = tempItem.form;
											_MESSAGE("Damaged %s by %.4f - Broken", armorForm->GetFullName(), fDmgAmount);
											return false;
										}
									}
								}
							}
							return true;
						});
					}
				}
			}
		}
		ownerActor->actorValueOwner.SetBase(DTConfig::avDamageArmorCheck, 0.0);
		return brokenArmor;
	}



	TESForm * DamageRandomArmor(StaticFunctionTag*, Actor * ownerActor)
	{
		if (!ownerActor || !ownerActor->inventoryList || (ownerActor->inventoryList->items.count == 0)) {
			ownerActor->actorValueOwner.SetBase(DTConfig::avDamageArmorCheck, 0.0);
			return nullptr;
		}
		_MESSAGE("UpdateArmorCND called");
		TESForm * brokenArmor = nullptr;

		int iChancePerPiece = 25;
		int iDmgChanceRoll = 0;

		// temp flags:
		UInt16 iFlagEquipped = 0x1;
		UInt32 iArmorSlotHairTop = 0x1;
		UInt32 iArmorSlotBody = 0x8;
		UInt32 iArmorSlotTorsoA = 0x800;
		UInt32 iArmorSlotLArmA = 0x1000;
		UInt32 iArmorSlotRArmA = 0x2000;
		UInt32 iArmorSlotLLegA = 0x4000;
		UInt32 iArmorSlotRLegA = 0x8000;

		UInt32 bipedFlags = 0;
		float fDmgAmount = 10.0;
		float fActualHealth = 0.0;
		bool bDoDamage = false;

		for (UInt32 i = 0; i < ownerActor->inventoryList->items.count; i++) {
			BGSInventoryItem tempItem;
			if (ownerActor->inventoryList->items.GetNthItem(i, tempItem)) {
				if (tempItem.form && tempItem.form->formType == kFormType_ARMO) {
					tempItem.stack->Visit([&](BGSInventoryItem::Stack * stack)
					{
						ExtraDataList * stackData = stack->extraData;
						if (!stackData || ((stack->flags & iFlagEquipped) != iFlagEquipped)) {
							return true;
						}
						TESObjectARMO * armorForm = (TESObjectARMO*)tempItem.form;
						if (!armorForm) {
							return true;
						}

						bipedFlags = armorForm->bipedObject.data.parts;
						bDoDamage = false;
						iDmgChanceRoll = DTConfig::rng.RandomInt(0, iChancePerPiece);

						// hairtop/helmet
						if ((bipedFlags & iArmorSlotHairTop) == iArmorSlotHairTop) {
							bDoDamage = iDmgChanceRoll < iChancePerPiece;
						}
						// outfit/bottom layer
						if ((bipedFlags & iArmorSlotBody) == iArmorSlotBody) {
							bDoDamage = iDmgChanceRoll < iChancePerPiece;
						}
						// chest armor
						if ((bipedFlags & iArmorSlotTorsoA) == iArmorSlotTorsoA) {
							bDoDamage = iDmgChanceRoll < iChancePerPiece;
						}
						// left arm armor
						if ((bipedFlags & iArmorSlotLArmA) == iArmorSlotLArmA) {
							bDoDamage = iDmgChanceRoll < iChancePerPiece;
						}
						// right arm armor
						if ((bipedFlags & iArmorSlotRArmA) == iArmorSlotRArmA) {
							bDoDamage = iDmgChanceRoll < iChancePerPiece;
						}
						// left leg armor
						if ((bipedFlags & iArmorSlotLLegA) == iArmorSlotLLegA) {
							bDoDamage = iDmgChanceRoll < iChancePerPiece;
						}
						// right leg armor
						if ((bipedFlags & iArmorSlotRLegA) == iArmorSlotRLegA) {
							bDoDamage = iDmgChanceRoll < iChancePerPiece;
						}

						// only continue if armor in this slot was damaged
						if (!bDoDamage) {
							return true;
						}

						TESObjectARMO::InstanceData * armorInstance = nullptr;
						BSExtraData * extraDataInst = stackData->GetByType(ExtraDataType::kExtraData_InstanceData);
						if (extraDataInst) {
							ExtraInstanceData * objectModData = DYNAMIC_CAST(extraDataInst, BSExtraData, ExtraInstanceData);
							if (objectModData) {
								armorInstance = (TESObjectARMO::InstanceData*)Runtime_DynamicCast(objectModData->instanceData, RTTI_TBO_InstanceData, RTTI_TESObjectARMO__InstanceData);
							}
						}
						if (!armorInstance) {
							armorInstance = &armorForm->instanceData;
						}
						if (armorInstance && (armorInstance->health != 0)) {
							BSExtraData * extraDataHealth = stackData->GetByType(ExtraDataType::kExtraData_Health);
							if (!extraDataHealth) {
								fActualHealth = (float)(int)armorInstance->health;
								ExtraHealth * newHealthData = ExtraHealth::Create((fActualHealth - fDmgAmount) / fActualHealth);
								stackData->Add(newHealthData->type, (BSExtraData*)newHealthData);
								extraDataHealth = stackData->GetByType(ExtraDataType::kExtraData_Health);
							}
							if (extraDataHealth) {
								ExtraHealth * healthData = DYNAMIC_CAST(extraDataHealth, BSExtraData, ExtraHealth);
								if (healthData) {
									fActualHealth = (float)(int)armorInstance->health * healthData->health;
									if (fActualHealth - fDmgAmount > 0.0) {
										healthData->health = (fActualHealth - fDmgAmount) / (float)(int)armorInstance->health;
										_MESSAGE("Damaged %s by %.4f - new CND: %.4f%%", armorForm->GetFullName(), fDmgAmount, healthData->health);
									}
									else {
										healthData->health = 0.0;
										brokenArmor = tempItem.form;
										_MESSAGE("Damaged %s by %.4f - Broken", armorForm->GetFullName(), fDmgAmount);
										return false;
									}
								}
							}
						}
						return true;
					});
				}
			}
		}
		return brokenArmor;
	}


	// updates settings from config files - used to sync settings after editing with MCM
	void UpdateSettings(StaticFunctionTag*)
	{
		_MESSAGE("Updating setting from MCM ini");
		DTConfig::UpdateSettings();
	}

}


bool PapyrusDamageTweaks::RegisterPapyrus(VirtualMachine* vm)
{
	RegisterFuncs(vm);
	return true;
}


void PapyrusDamageTweaks::RegisterFuncs(VirtualMachine* vm)
{
	vm->RegisterFunction(
		new NativeFunction1 <StaticFunctionTag, SpellItem*, Actor*>("GetCritEffect", SCRIPTNAME_Globals, GetCritEffect, vm));

	vm->RegisterFunction(
		new NativeFunction1 <StaticFunctionTag, TESForm*, Actor*>("UpdateArmorCND", SCRIPTNAME_Globals, UpdateArmorCND, vm));
	vm->RegisterFunction(
		new NativeFunction1 <StaticFunctionTag, TESForm*, Actor*>("DamageRandomArmor", SCRIPTNAME_Globals, DamageRandomArmor, vm));

	vm->RegisterFunction(
		new NativeFunction0 <StaticFunctionTag, void>("UpdateSettings", SCRIPTNAME_Globals, UpdateSettings, vm));

	_MESSAGE("Registered native functions for %s", SCRIPTNAME_Globals);
}

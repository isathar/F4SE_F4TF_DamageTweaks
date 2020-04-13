#include "DamageTweaks.h"


struct FindPred_DmgTypeData_ByID
{
	UInt32 formID;
	FindPred_DmgTypeData_ByID(UInt32 formID) : formID(formID) {}
	bool operator () (const DmgTypeData & dmgType) const { return (formID == dmgType.damageTypeID); }
};

struct FindPred_DmgTypeData_ByWeaponKW
{
	IKeywordFormBase * keywordBase = nullptr;
	FindPred_DmgTypeData_ByWeaponKW(IKeywordFormBase * keywordBase = nullptr) : keywordBase(keywordBase) {}
	bool operator () (const DmgTypeData & dmgType) const
	{ return (keywordBase && dmgType.dmgOverrideKW) ? DamageTweaks::HasKeyword_Native(keywordBase, dmgType.dmgOverrideKW) : false; }
};


bool DamageTweaks::HasKeyword_Native(IKeywordFormBase * keywordBase, BGSKeyword * checkKW)
{
	if (!checkKW || !keywordBase) {
		return false;
	}
	auto HasKeyword_Internal = GetVirtualFunction<_IKeywordFormBase_HasKeyword>(keywordBase, 1);
	if (HasKeyword_Internal(keywordBase, checkKW, 0)) {
		return true;
	}
	return false;
}


namespace DamageTweaks
{
	// -- Damage Threshold
	float CalcDamage_DT(std::vector<DmgTypeStats> & damageTypesList)
	{
		float fFinalDmg = 0.0;
		float fCurDmg = 0.0;
		float fAPDmg = 0.0;
		float fMinDmgPercent = DTConfig::fDTMinDmgPercent;
		_MESSAGE("    Damage Threshold:");
		for (UInt32 i = 0; i < damageTypesList.size(); i++) {
			fAPDmg = (damageTypesList[i].fDmgAmount * max(0.0, damageTypesList[i].fArmorPen));
			fCurDmg = max(damageTypesList[i].fBaseDmg * fMinDmgPercent, damageTypesList[i].fDmgAmount - max(0.0, (damageTypesList[i].fDTVal * damageTypesList[i].fTargetArmorMult) - fAPDmg));
			_MESSAGE("      %i - 0x%08X:  Dmg in: %.4f, Dmg out: %.4f, DT: %.4f, DT Mult: %.4fx, AP: %.4f (%.2f%%)",
				i, damageTypesList[i].damageTypeID, damageTypesList[i].fDmgAmount, fCurDmg,
				damageTypesList[i].fDTVal, damageTypesList[i].fTargetArmorMult, fAPDmg, damageTypesList[i].fArmorPen * 100.0);
			damageTypesList[i].fDmgAmount = fCurDmg;
			fFinalDmg += fCurDmg;
		}
		_MESSAGE("        -->DT Out: %.4f dmg", fFinalDmg);
		return fFinalDmg;
	}

	// -- Damage Resistance (default)
	float CalcDamage_DR_Default(std::vector<DmgTypeStats> & damageTypesList)
	{
		float fMaxDRMult = DTConfig::fMaxPercentDR;
		float fFinalDmg = 0.0;
		float fDmgCoeff = 0.0;
		float fCurDmg = 0.0;
		float fDRVal = 0.0;
		_MESSAGE("    Damage Resistance:");
		for (UInt32 i = 0; i < damageTypesList.size(); i++) {
			fCurDmg = damageTypesList[i].fDmgAmount;
			fDRVal = damageTypesList[i].fDRVal * damageTypesList[i].fTargetArmorMult * (1.0 - damageTypesList[i].fArmorPen);
			fDmgCoeff = (fDRVal > 0.0) ? max(0.0, min(fMaxDRMult, pow((damageTypesList[i].fDmgAmount * damageTypesList[i].fDmgFactor) / fDRVal, damageTypesList[i].fDRExponent))) : 1.0;
			fCurDmg = fCurDmg * fDmgCoeff;
			_MESSAGE("      %i - 0x%08X:  Dmg in: %.4f, Dmg out: %.4f, DR: %.4f, AP: %.2f%%, DR Mult: %.4fx, Dmg Mult: %.4f",
				i, damageTypesList[i].damageTypeID, damageTypesList[i].fDmgAmount, fCurDmg,
				damageTypesList[i].fDRVal, damageTypesList[i].fArmorPen * 100.0, damageTypesList[i].fTargetArmorMult, fDmgCoeff);
			damageTypesList[i].fDmgAmount = fCurDmg;
			fFinalDmg += fCurDmg;
		}
		_MESSAGE("        -->DR Out: %.4f dmg", fFinalDmg);
		return fFinalDmg;
	}

	// -- Damage Resistance (classic)
	float CalcDamage_DR_Percentage(std::vector<DmgTypeStats> & damageTypesList)
	{
		float fFinalDmg = 0.0;
		float fCurDmg = 0.0;
		float fDRFactor = 1.0;
		float fMaxDRPercent = DTConfig::fMaxPercentDR;
		_MESSAGE("    Damage Resistance (Percent):");
		for (UInt32 i = 0; i < damageTypesList.size(); i++) {
			fDRFactor = min(fMaxDRPercent, max(0.0, damageTypesList[i].fDRVal * damageTypesList[i].fTargetArmorMult * (1.0 - damageTypesList[i].fArmorPen) * 0.01));
			fCurDmg = damageTypesList[i].fDmgAmount * fDRFactor;
			_MESSAGE("      %i - 0x%08X:  Dmg in: %.4f, Dmg out: %.4f, DR: %.4f%%, AP: %.2f%%, DR Mult: %.4f, Dmg Mult: %.4f",
				i, damageTypesList[i].damageTypeID, damageTypesList[i].fDmgAmount, fCurDmg,
				damageTypesList[i].fDRVal, damageTypesList[i].fArmorPen * 100.0, damageTypesList[i].fTargetArmorMult, fDRFactor);
			damageTypesList[i].fDmgAmount = fCurDmg;
			fFinalDmg += fCurDmg;
		}
		_MESSAGE("        -->DR Out: %.4f dmg", fFinalDmg);
		return fFinalDmg;
	}

	// fills damageTypesList returns combined base damage
	float GetWeaponDmg(TESObjectREFR * attacker, TESObjectREFR * victim, TESObjectWEAP * weaponForm, TESObjectWEAP::InstanceData * weaponInstance, std::vector<DmgTypeStats> & damageTypesList, float fBaseDamage = 0.0)
	{
		float fWeaponDmg = weaponInstance ? (float)(int)weaponInstance->baseDamage : 0.0;
		float fArmorPen = 0.0;
		float fTargetDRMult = 1.0;
		float fNumProjectiles = (weaponInstance && weaponInstance->firingData) ? (float)(int)(UInt8)((weaponInstance->firingData->numProjectiles << 12) >> 12) : 1.0;
		UInt32 iOverrideDmgType = 0;
		bool bThrown = weaponInstance && weaponInstance->equipSlot && (weaponInstance->equipSlot->formID == 0x46AAC);
		bool bHasPhysDmg = false;
		float fAddVal = 0.0;

		// -- check source type
		if (bThrown) {
			_MESSAGE("  Source: Thrown weapon");
			fWeaponDmg = fBaseDamage;
			fArmorPen = attacker->actorValueOwner.GetValue(DTConfig::avArmorPenetrationThrown) * 0.01;
			fTargetDRMult = attacker->actorValueOwner.GetValue(DTConfig::avTargetArmorMultThrown) * 0.01;
		}
		else {
			if (weaponForm) {
				if (HasKeyword_Native(&weaponForm->keyword.keywordBase, DTConfig::kwCrRanged)) {
					_MESSAGE("  Source: Creature Ranged Weapon");
					fWeaponDmg += attacker->actorValueOwner.GetValue(DTConfig::avCrRangedDamage);
				}
				else if (HasKeyword_Native(&weaponForm->keyword.keywordBase, DTConfig::kwAnimsUnarmed)) {
					_MESSAGE("  Source: Unarmed Weapon");
					fWeaponDmg += attacker->actorValueOwner.GetValue(DTConfig::avUnarmedDamage);
				}
			}
			else {
				// could be creature unarmed or ranged, add both since creatures use one or the other
				fWeaponDmg += attacker->actorValueOwner.GetValue(DTConfig::avCrRangedDamage);
				fWeaponDmg += attacker->actorValueOwner.GetValue(DTConfig::avUnarmedDamage);
			}
			fArmorPen = attacker->actorValueOwner.GetValue(DTConfig::avArmorPenetration) * 0.01;
			fTargetDRMult = attacker->actorValueOwner.GetValue(DTConfig::avTargetArmorMult) * 0.01;
		}

		// -- check for damageType overrides
		// base weapon form keywords
		std::vector<DmgTypeData>::iterator newDmgType = std::find_if(DTConfig::damageTypes.begin(), DTConfig::damageTypes.end(), FindPred_DmgTypeData_ByWeaponKW(&weaponForm->keyword.keywordBase));
		if (newDmgType != DTConfig::damageTypes.end()) {
			iOverrideDmgType = newDmgType->damageTypeID;
		}
		else {
			// weapon instance keywords
			if (weaponInstance && weaponInstance->keywords && (weaponInstance->keywords->numKeywords != 0)) {
				newDmgType = std::find_if(DTConfig::damageTypes.begin(), DTConfig::damageTypes.end(), FindPred_DmgTypeData_ByWeaponKW(&weaponInstance->keywords->keywordBase));
				if (newDmgType != DTConfig::damageTypes.end()) {
					iOverrideDmgType = newDmgType->damageTypeID;
				}
			}
		}

		// base damage (physical unless overridden)
		if (fWeaponDmg > 0.0) {
			fWeaponDmg = fWeaponDmg / fNumProjectiles;
			DmgTypeStats newStats = DmgTypeStats();
			if (iOverrideDmgType == 0) {
				newDmgType = std::find_if(DTConfig::damageTypes.begin(), DTConfig::damageTypes.end(), FindPred_DmgTypeData_ByID(0x60A87));
			}
			if (newDmgType != DTConfig::damageTypes.end()) {
				if (newDmgType->damageThreshold) {
					newStats.fDTVal = victim->actorValueOwner.GetValue(newDmgType->damageThreshold);
				}
				if (newDmgType->damageResist) {
					newStats.fDRVal = victim->actorValueOwner.GetValue(newDmgType->damageResist);
				}
				newStats.fArmorPen = fArmorPen;
				newStats.fTargetArmorMult = fTargetDRMult;
			}
			newStats.damageTypeID = newDmgType->damageTypeID;
			newStats.iCritTableID = newDmgType->critTableID;
			newStats.fDmgAmount = fWeaponDmg;
			newStats.fBaseDmg = fWeaponDmg;
			newStats.fDmgFactor = newDmgType->fArmorDmgFactor;
			newStats.fDRExponent = newDmgType->fArmorDmgResistExponent;
			damageTypesList.push_back(newStats);
			bHasPhysDmg = true;
		}

		// typed damage
		if (weaponInstance && weaponInstance->damageTypes && weaponInstance->damageTypes->count != 0) {
			TBO_InstanceData::DamageTypes tempDT;
			if (bHasPhysDmg && (iOverrideDmgType != 0)) {
				// damage override enabled
				for (UInt32 i = 0; i < weaponInstance->damageTypes->count; i++) {
					if (weaponInstance->damageTypes->GetNthItem(i, tempDT)) {
						if (tempDT.damageType) {
							fAddVal = (float)(int)tempDT.value / fNumProjectiles;
							damageTypesList[0].fBaseDmg += fAddVal;
							damageTypesList[0].fDmgAmount += fAddVal;
							fWeaponDmg += fAddVal;
						}
					}
				}
			}
			else {
				// standard damage types
				for (UInt32 i = 0; i < weaponInstance->damageTypes->count; i++) {
					if (weaponInstance->damageTypes->GetNthItem(i, tempDT)) {
						if (tempDT.damageType) {
							if (bHasPhysDmg && (tempDT.damageType->formID == 0x60A87)) {
								fAddVal = (float)(int)tempDT.value / fNumProjectiles;
								damageTypesList[0].fBaseDmg += fAddVal;
								damageTypesList[0].fDmgAmount += fAddVal;
								fWeaponDmg += fAddVal;
							}
							else {
								DmgTypeStats newStats = DmgTypeStats();
								newDmgType = std::find_if(DTConfig::damageTypes.begin(), DTConfig::damageTypes.end(), FindPred_DmgTypeData_ByID(tempDT.damageType->formID));
								if (newDmgType != DTConfig::damageTypes.end()) {
									if (newDmgType->damageThreshold) {
										newStats.fDTVal = victim->actorValueOwner.GetValue(newDmgType->damageThreshold);
									}
									if (newDmgType->damageResist) {
										newStats.fDRVal = victim->actorValueOwner.GetValue(newDmgType->damageResist);
									}
									newStats.fArmorPen = fArmorPen;
									newStats.fTargetArmorMult = fTargetDRMult;
								}
								if (iOverrideDmgType != 0) {
									newStats.damageTypeID = iOverrideDmgType;
								}
								else {
									newStats.damageTypeID = tempDT.damageType->formID;
								}
								newStats.iCritTableID = newDmgType->critTableID;
								newStats.fDmgAmount = (float)(int)tempDT.value;
								newStats.fBaseDmg = (float)(int)tempDT.value;
								newStats.fDmgFactor = newDmgType->fArmorDmgFactor;
								newStats.fDRExponent = newDmgType->fArmorDmgResistExponent;
								fWeaponDmg += newStats.fDmgAmount;
								damageTypesList.push_back(newStats);
							}
						}
					}
				}
			}
		}
		return fWeaponDmg;
	}



	void DamageArmor(TESObjectREFR * victim, float fDamageAmount)
	{
		float fHead = victim->actorValueOwner.GetValue(DTConfig::avHeadCND);
		victim->actorValueOwner.SetBase(DTConfig::avLastHeadCND, fHead);
		//_MESSAGE("Head CND: %.4f", fHead);

		float fTorso = victim->actorValueOwner.GetValue(DTConfig::avTorsoCND);
		victim->actorValueOwner.SetBase(DTConfig::avLastTorsoCND, fTorso);
		//_MESSAGE("Torso CND: %.4f", fTorso);

		float fArmL = victim->actorValueOwner.GetValue(DTConfig::avArmLCND);
		victim->actorValueOwner.SetBase(DTConfig::avLastArmLCND, fArmL);
		//_MESSAGE("ArmL CND: %.4f", fArmL);

		float fArmR = victim->actorValueOwner.GetValue(DTConfig::avArmRCND);
		victim->actorValueOwner.SetBase(DTConfig::avLastArmRCND, fArmR);
		//_MESSAGE("ArmR CND: %.4f", fArmR);

		float fLegL = victim->actorValueOwner.GetValue(DTConfig::avLegLCND);
		victim->actorValueOwner.SetBase(DTConfig::avLastLegLCND, fLegL);
		//_MESSAGE("LegL CND: %.4f", fLegL);

		float fLegR = victim->actorValueOwner.GetValue(DTConfig::avLegRCND);
		victim->actorValueOwner.SetBase(DTConfig::avLastLegRCND, fLegR);
		//_MESSAGE("LegR CND: %.4f", fLegR);

		float fFinalDmg = fDamageAmount * DTConfig::fArmorDmgMultiplier;
		_MESSAGE("  Armor CND dmg: %.4f", fFinalDmg);
		victim->actorValueOwner.SetBase(DTConfig::avDamageArmorCheck, fFinalDmg);
	}

}


// entry point
float DamageTweaks::CalcDamage(TESObjectREFR * attacker, TESObjectREFR * victim, TESObjectWEAP * weaponForm, TESObjectWEAP::InstanceData * weaponInstance, float fBaseDmg, float fFinalDmgMult)
{
	_MESSAGE("\n\nCalculating Damage - starting dmg:  %.4f", fBaseDmg);
	std::vector<DmgTypeStats> damageTypesList;
	float fFinalDmg = 0.0;
	UInt8 iArmorRoll = 0;
	bool bDoArmorDamage = DTConfig::bEnableArmorCND && HasKeyword_Native(&victim->keywordFormBase, DTConfig::kwCanHaveArmorDamage);
	
	// -- Get Weapon Damage
	fFinalDmg = GetWeaponDmg(attacker, victim, weaponForm, weaponInstance, damageTypesList, fBaseDmg);
	if (damageTypesList.size() == 0) {
		_MESSAGE("  ERROR: Empty damageTypesList!");
		return fBaseDmg * fFinalDmgMult;
	}

	// -- Distance Multiplier
	float fDistDmgMult = (fFinalDmg > 0.0) ? fBaseDmg / fFinalDmg : 1.0;
	for (UInt32 i = 0; i < damageTypesList.size(); i++) {
		damageTypesList[i].fDmgAmount *= fDistDmgMult;
		damageTypesList[i].fBaseDmg *= fDistDmgMult;
	}

	// -- Damage Thresholds (classic)
	if (DTConfig::iDTType == 1) {
		fFinalDmg = CalcDamage_DT(damageTypesList);
	}
	if (fFinalDmg > 0.0) {
		// -- Damage Resistances
		if (DTConfig::iDRType == 1) {
			fFinalDmg = CalcDamage_DR_Default(damageTypesList);
		}
		else if (DTConfig::iDRType == 2) {
			fFinalDmg = CalcDamage_DR_Percentage(damageTypesList);
		}
		if (fFinalDmg > 0.0) {
			// -- Damage Thresholds (FNV)
			if (DTConfig::iDTType == 2) {
				fFinalDmg = CalcDamage_DT(damageTypesList);
			}
		}
		fFinalDmg = max(DTConfig::fMinDmg, fFinalDmg);
	}

	if (fFinalDmg > 0.0) {
		// -- BodyPart, Perk multipliers
		fFinalDmg = fFinalDmg * fFinalDmgMult;

		// -- Critical hit check
		if (DTConfig::bEnableCriticalHits) {
			float fCNDPct = attacker->actorValueOwner.GetValue(DTConfig::avWeaponCND);
			if (fCNDPct < 0.0) {
				fCNDPct = 100.0;
			}
			float fCritChanceMult = weaponInstance ? weaponInstance->critChargeBonus * fCNDPct : fCNDPct;
			float fCritChance = (attacker->actorValueOwner.GetValue(DTConfig::specialAVs[6]) + attacker->actorValueOwner.GetValue(DTConfig::avCritChanceAdd)) * fCritChanceMult;
			float fCritDmgMult = weaponInstance ? attacker->actorValueOwner.GetValue(DTConfig::avCritDmgMult) * weaponInstance->critDamageMult * 0.01 : attacker->actorValueOwner.GetValue(DTConfig::avCritDmgMult) * 0.01;
			int iCritRollMod = (int)attacker->actorValueOwner.GetValue(DTConfig::avCritRollMod);
			int iCritRoll = 0;
			int iCritEffectRoll = 0;
			int iCritTable = -1;
			int iRandIdx = 0;

			if (fCritChance > 0.0) {
				iCritRoll = DTConfig::rng.RandomInt(0, 10000);
				if (iCritRoll < (int)fCritChance) {
					iCritEffectRoll = DTConfig::rng.RandomInt(0, 85 + iCritRollMod);
					if (DTConfig::bCritDmgMultRoll) {
						if (iCritEffectRoll > 66) {
							fCritDmgMult = fCritDmgMult * 2.0;
						}
						else if (iCritEffectRoll > 33) {
							fCritDmgMult = fCritDmgMult * 1.5;
						}
					}
					fFinalDmg = fFinalDmg * fCritDmgMult;

					if (DTConfig::bCritsUseEffectTables) {
						if (damageTypesList.size() != 0) {
							if (damageTypesList.size() > 1) {
								iRandIdx = DTConfig::rng.RandomInt(0, damageTypesList.size() - 1);
							}
							iCritTable = damageTypesList[iRandIdx].iCritTableID;
						}
						if (iCritTable > -1) {
							victim->actorValueOwner.SetBase(DTConfig::avTargetCritRollMod, (float)iCritEffectRoll);
							iCritTable += 1;
							victim->actorValueOwner.SetBase(DTConfig::avCritTableCheck, (float)(iCritTable));
						}
					}
					_MESSAGE("      Critical Hit!  critDmgMult: %.4f, Chance: %.2f%%,  Roll: %.2f", fCritDmgMult, 0.01 * fCritChance, 0.01 * (float)iCritRoll);
				}
				else {
					_MESSAGE("      Regular hit.  crit chance: %.2f%%, Roll: %.2f", fCritChance * 0.01, 0.01 * (float)iCritRoll);
				}
			}
		}

		// -- Armor Damage
		if (bDoArmorDamage) {
			iArmorRoll = DTConfig::rng.RandomInt(0, 100);
			if (iArmorRoll < DTConfig::iArmorDmgChance) {
				DamageArmor(victim, fBaseDmg);
			}
		}

	}
	
	return fFinalDmg;
}



// debug
void DamageTweaks::DumpDamageFrame(DamageFrame * pDamageFrame)
{
	if (pDamageFrame) {
		_MESSAGE("\nDamageFrame Dump:");
		_MESSAGE("  attackerHandle: 0x%08X", pDamageFrame->attackerHandle);
		_MESSAGE("  victimHandle: 0x%08X", pDamageFrame->victimHandle);
		_MESSAGE("  damage: %.4f\n  damage2: %.4f\n  unk94 (base damage): %.4f", pDamageFrame->damage, pDamageFrame->damage2, pDamageFrame->unk94);
		if (pDamageFrame->damageSourceForm) {
			_MESSAGE("  damage source: %s (0x%08X)", pDamageFrame->damageSourceForm->GetFullName(), pDamageFrame->damageSourceForm->formID);
		}
		else {
			_MESSAGE("  no damage source form");
		}
		if (pDamageFrame->ammo) {
			_MESSAGE("  ammo: %s", pDamageFrame->ammo->GetFullName());
		}
		if (pDamageFrame->attackData) {
			if (pDamageFrame->damageSourceForm) {
				TESObjectWEAP * tempWeap = reinterpret_cast<TESObjectWEAP*>(pDamageFrame->damageSourceForm);
				if (tempWeap && tempWeap->weapData.ammo) {
					_MESSAGE("  source is a gun bash attack");
				}
				else {
					_MESSAGE("  source is a melee weapon");
				}
			}
			else {
				_MESSAGE("  source is a melee attack");
			}
		}
		if (pDamageFrame->instanceData) {
			_MESSAGE("  source has instanceData");
		}
		else {
			_MESSAGE("  source has no instanceData");
		}
		_MESSAGE("  unk10: [%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f]",
			pDamageFrame->unk10[0], pDamageFrame->unk10[1], pDamageFrame->unk10[2], pDamageFrame->unk10[3],
			pDamageFrame->unk10[4], pDamageFrame->unk10[5], pDamageFrame->unk10[6], pDamageFrame->unk10[7]);
		_MESSAGE("  unk38: 0x%016X", pDamageFrame->unk38);
		_MESSAGE("  unk48: [0x%016X]", pDamageFrame->unk48[0]);
		_MESSAGE("  unk68: [0x%016X, 0x%016X, 0x%016X]", pDamageFrame->unk68[0], pDamageFrame->unk68[1], pDamageFrame->unk68[2]);
		if (pDamageFrame->unk88) {
			TESForm * testForm = reinterpret_cast<TESForm*>(pDamageFrame->unk88);
			_MESSAGE("  unk88: 0x%08X", testForm ? testForm->formID : 0);
		}
	}
}



std::vector<DmgTypeData> DTConfig::damageTypes = std::vector<DmgTypeData>();
std::vector<CritEffectTable> DTConfig::critEffectTables = std::vector<CritEffectTable>();
std::vector<ActorValueInfo*> DTConfig::specialAVs = std::vector<ActorValueInfo*>();

ATxoroshiro128p DTConfig::rng = ATxoroshiro128p();


ActorValueInfo * DTConfig::avCritTableCheck = nullptr;
ActorValueInfo * DTConfig::avCritRollMod = nullptr;
ActorValueInfo * DTConfig::avTargetCritRollMod = nullptr;

ActorValueInfo * DTConfig::avCritDmgMult = nullptr;
ActorValueInfo * DTConfig::avCritChanceAdd = nullptr;

ActorValueInfo * DTConfig::avWeaponCND = nullptr;
ActorValueInfo * DTConfig::avArmorPenetration = nullptr;
ActorValueInfo * DTConfig::avArmorPenetrationThrown = nullptr;
ActorValueInfo * DTConfig::avTargetArmorMult = nullptr;
ActorValueInfo * DTConfig::avTargetArmorMultThrown = nullptr;

ActorValueInfo * DTConfig::avHeadCND = nullptr;
ActorValueInfo * DTConfig::avTorsoCND = nullptr;
ActorValueInfo * DTConfig::avArmLCND = nullptr;
ActorValueInfo * DTConfig::avArmRCND = nullptr;
ActorValueInfo * DTConfig::avLegLCND = nullptr;
ActorValueInfo * DTConfig::avLegRCND = nullptr;

ActorValueInfo * DTConfig::avLastHeadCND = nullptr;
ActorValueInfo * DTConfig::avLastTorsoCND = nullptr;
ActorValueInfo * DTConfig::avLastArmLCND = nullptr;
ActorValueInfo * DTConfig::avLastArmRCND = nullptr;
ActorValueInfo * DTConfig::avLastLegLCND = nullptr;
ActorValueInfo * DTConfig::avLastLegRCND = nullptr;

BGSKeyword * DTConfig::kwAnimsUnarmed = nullptr;
ActorValueInfo * DTConfig::avUnarmedDamage = nullptr;
BGSKeyword * DTConfig::kwCrRanged = nullptr;
ActorValueInfo * DTConfig::avCrRangedDamage = nullptr;

ActorValueInfo * DTConfig::avDamageArmorCheck = nullptr;
BGSKeyword * DTConfig::kwCanHaveArmorDamage = nullptr;

// -- default settings: mimic standard DR

// min overall damage to always let through resistance calc
float DTConfig::fMinDmg = 1.0;

// Damage Resistance: 0=off, 1=Fallout4 (default), 2=Classic (percentage-based)
UInt8 DTConfig::iDRType = 0;
// Damage Threshold: 0=off, 1=Classic (before DR), 2=FNV (after DR)
UInt8 DTConfig::iDTType = 1;

bool DTConfig::bEnableCriticalHits = false;
bool DTConfig::bCritChanceUsesCND = false;
bool DTConfig::bCritDmgMultRoll = false;
bool DTConfig::bCritsUseEffectTables = false;
bool DTConfig::bCritsUseSavingRoll = false;

// max percentage of damage mitigated
float DTConfig::fMaxPercentDR = 0.85;

// min percentage of damage to let through DT
float DTConfig::fDTMinDmgPercent = 0.2;

bool DTConfig::bEnableArmorCND = true;
// chance of any armor damage per hit
UInt8 DTConfig::iArmorDmgChance = 15;
// multiplier for dmg applied to armor
float DTConfig::fArmorDmgMultiplier = 0.25;


std::string DTConfig::sConfigPathSettingsBase = ".\\Data\\MCM\\Config\\DamageTweaks\\settings.ini";
std::string DTConfig::sConfigPathSettingsCur = ".\\Data\\MCM\\Settings\\DamageTweaks.ini";
std::string DTConfig::sConfigPathData = ".\\Data\\F4SE\\Config\\DamageTweaks\\";

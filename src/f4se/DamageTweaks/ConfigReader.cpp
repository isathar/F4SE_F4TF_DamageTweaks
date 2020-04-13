#include "ConfigReader.h"

#include "SimpleIni/SimpleIni.h"
#include "nlohmann/json.hpp"
#include <Windows.h>
#include <fstream>
#include <iostream>


using json = nlohmann::json;


// returns a formID from a formatted string
inline UInt32 GetFormIDFromIdentifier(const std::string & formIdentifier)
{
	UInt32 formId = 0;
	if (formIdentifier.c_str() != "none") {
		std::size_t pos = formIdentifier.find_first_of("|");
		std::string modName = formIdentifier.substr(0, pos);
		std::string modForm = formIdentifier.substr(pos + 1);
		sscanf_s(modForm.c_str(), "%X", &formId);

		if (formId != 0x0) {
			UInt8 modIndex = (*g_dataHandler)->GetLoadedModIndex(modName.c_str());
			if (modIndex != 0xFF) {
				formId |= ((UInt32)modIndex) << 24;
			}
			else {
				UInt16 lightModIndex = (*g_dataHandler)->GetLoadedLightModIndex(modName.c_str());
				if (lightModIndex != 0xFFFF) {
					formId |= 0xFE000000 | (UInt32(lightModIndex) << 12);
				}
				else {
					_MESSAGE("FormID %s not found!", formIdentifier.c_str());
					formId = 0;
				}
			}
		}
	}
	return formId;
}

// returns a form from a formatted string
inline TESForm * GetFormFromIdentifier(const std::string & formIdentifier)
{
	UInt32 formId = 0;
	if (formIdentifier.c_str() != "none") {
		std::size_t pos = formIdentifier.find_first_of("|");
		std::string modName = formIdentifier.substr(0, pos);
		std::string modForm = formIdentifier.substr(pos + 1);
		sscanf_s(modForm.c_str(), "%X", &formId);
		if (formId != 0x0) {
			UInt8 modIndex = (*g_dataHandler)->GetLoadedModIndex(modName.c_str());
			if (modIndex != 0xFF) {
				formId |= ((UInt32)modIndex) << 24;
			}
			else {
				UInt16 lightModIndex = (*g_dataHandler)->GetLoadedLightModIndex(modName.c_str());
				if (lightModIndex != 0xFFFF) {
					formId |= 0xFE000000 | (UInt32(lightModIndex) << 12);
				}
				else {
					_MESSAGE("FormID %s not found!", formIdentifier.c_str());
					formId = 0;
				}
			}
		}
	}
	return (formId != 0x0) ? LookupFormByID(formId) : nullptr;
}

// returns a list of json file names at the passed path
inline std::vector<std::string> GetFileNames(const std::string & folder)
{
	std::vector<std::string> names;
	std::string search_path = folder + "/*.json";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}


bool BuildData_DamageType(const std::string & dataPath, DmgTypeData & curResist)
{
	json curDTObj;
	std::ifstream loadedFile(dataPath.c_str());
	loadedFile >> curDTObj;
	loadedFile.close();

	if (curDTObj.is_null() || curDTObj["sDmgTypeID"].is_null()) {
		_MESSAGE("      ERROR: failed to load DamageType!");
		return false;
	}

	std::string dmgIDStr = curDTObj["sDmgTypeID"];
	curResist.damageTypeID = GetFormIDFromIdentifier(dmgIDStr);

	if (!curDTObj["sDmgResistAV"].is_null()) {
		std::string drIDStr = curDTObj["sDmgResistAV"];
		curResist.damageResist = (ActorValueInfo*)GetFormFromIdentifier(drIDStr);
	}
	if (!curDTObj["sDmgThresholdAV"].is_null()) {
		std::string dtIDStr = curDTObj["sDmgThresholdAV"];
		curResist.damageThreshold = (ActorValueInfo*)GetFormFromIdentifier(dtIDStr);
	}
	if (!curDTObj["fArmorDmgReductionExp"].is_null()) {
		curResist.fArmorDmgResistExponent = curDTObj["fArmorDmgReductionExp"];
	}
	if (!curDTObj["fArmorDmgFactor"].is_null()) {
		curResist.fArmorDmgFactor = curDTObj["fArmorDmgFactor"];
	}
	if (!curDTObj["iCritTableID"].is_null()) {
		curResist.critTableID = curDTObj["iCritTableID"];
	}
	if (!curDTObj["sDmgOverrideKW"].is_null()) {
		std::string ovrdKWIDStr = curDTObj["sDmgOverrideKW"];
		curResist.dmgOverrideKW = (BGSKeyword*)GetFormFromIdentifier(ovrdKWIDStr);
	}

	_MESSAGE("\n    Created damage type %s:", curResist.dmgTypeName.c_str());
	_MESSAGE("      damageTypeID: 0x%08X", curResist.damageTypeID);
	_MESSAGE("      damageResist: 0x%08X", curResist.damageResist ? curResist.damageResist->formID : 0);
	_MESSAGE("      damageThreshold: 0x%08X", curResist.damageThreshold ? curResist.damageThreshold->formID : 0);
	_MESSAGE("      fArmorDmgResistExponent: %.4f", curResist.fArmorDmgResistExponent);
	_MESSAGE("      fArmorDmgFactor: %.4f", curResist.fArmorDmgFactor);
	_MESSAGE("      critTableID: %i", curResist.critTableID);

	return true;
}


bool BuildData_CriticalEffectTable(const std::string & dataPath, CritEffectTable & newCritTable)
{
	json cetData;
	std::ifstream loadedFile(dataPath.c_str());
	loadedFile >> cetData;
	loadedFile.close();

	if (cetData.is_null() || cetData["sRequiredKW"].is_null()) {
		_MESSAGE("    ERROR: Crit Effect Table sRequiredKW is null!");
		return false;
	}
	if (cetData["critTables"].is_null() || cetData["critTables"].empty()) {
		_MESSAGE("    ERROR: Missing critTables!");
		return false;
	}

	std::string actorTypeKWIDStr = cetData["sRequiredKW"];
	newCritTable.requiredKW = (BGSKeyword*)GetFormFromIdentifier(actorTypeKWIDStr);
	
	_MESSAGE("\n    Building crit effect table for %s...", newCritTable.requiredKW->keyword.c_str());

	json varTableObj;
	for (json::iterator varTableIt = cetData["critTables"].begin(); varTableIt != cetData["critTables"].end(); ++varTableIt) {
		varTableObj = *varTableIt;
		if (varTableObj.is_null() || varTableObj.empty()) {
			_MESSAGE("      ERROR: Empty or null critTable!");
			continue;
		}
		if (varTableObj["iCritTypeID"].is_null()) {
			_MESSAGE("      ERROR: missing iCritTypeID!");
			continue;
		}
		if (varTableObj["effects"].is_null() || varTableObj["effects"].empty()) {
			_MESSAGE("      ERROR: Empty or null effects list!");
			continue;
		}

		CritEffectTable::TypedCritTable newVarTable = CritEffectTable::TypedCritTable();

		newVarTable.iCritTableID = varTableObj["iCritTypeID"];

		if (!varTableObj["sName"].is_null()) {
			std::string critVarNameStr = varTableObj["sName"];
			newVarTable.sMenuName = critVarNameStr.c_str();
		}

		json varCritObj;
		for (json::iterator varCritsIt = varTableObj["effects"].begin(); varCritsIt != varTableObj["effects"].end(); ++varCritsIt) {
			varCritObj = *varCritsIt;
			if (varCritObj.is_null() || varCritObj.empty()) {
				_MESSAGE("      ERROR: Empty or null crit effect data!");
				continue;
			}
			if (varCritObj["iRollMax"].is_null()) {
				_MESSAGE("      ERROR: Missing iRollMax!");
				continue;
			}

			CritEffectTable::CritEffect newCritEffect = CritEffectTable::CritEffect();
			newCritEffect.iRollMax = varCritObj["iRollMax"];

			if (!varCritObj["sSpellID"].is_null()) {
				std::string spellIDStr = varCritObj["sSpellID"];
				newCritEffect.critEffect = (SpellItem*)GetFormFromIdentifier(spellIDStr);
			}

			// saving roll stuff - only read if sSavingRollAV exists and is a valid AV
			if (!varCritObj["iSavingRollAV"].is_null()) {
				newCritEffect.iSavingRollAVIndex = varCritObj["iSavingRollAV"];
				if (newCritEffect.iSavingRollAVIndex > -1) {
					if (!varCritObj["sSpellIDSaved"].is_null()) {
						std::string spellIDSavedStr = varCritObj["sSpellIDSaved"];
						newCritEffect.weakCritEffect = (SpellItem*)GetFormFromIdentifier(spellIDSavedStr);
					}
					if (!varCritObj["iSavingRollMod"].is_null()) {
						newCritEffect.iSavingRollMod = varCritObj["iSavingRollMod"];
					}
				}
			}

			newVarTable.critEffects.push_back(newCritEffect);
		}
		_MESSAGE("      Adding crit effect table var %s", newVarTable.sMenuName);
		newCritTable.critEffects_Typed.push_back(newVarTable);
	}
	return true;
}


void DTConfig::UpdateSettings()
{
	CSimpleIniA iniSettings;

	float fDTMinDmgPercentTmp = fDTMinDmgPercent;
	float fMaxPercentDRTmp = fMaxPercentDR;
	float fArmorDmgMultiplierTmp = fArmorDmgMultiplier;
	UInt8 iDTTypeTmp = iDTType;
	UInt8 iDRTypeTmp = iDRType;
	UInt8 iArmorDmgChanceTmp = iArmorDmgChance;
	bool bEnableArmorCNDTmp = bEnableArmorCND;
	bool bAllowFullMitigation = false, bAllowFullMitigationTmp = false;
	bool bEnableCriticalHitsTmp = bEnableCriticalHits;
	bool bCritDmgMultRollTmp = bCritDmgMultRoll;
	bool bCritChanceUsesCNDTmp = bCritChanceUsesCND;
	bool bCritsUseEffectTablesTmp = bCritsUseEffectTables;
	bool bCritsUseSavingRollTmp = bCritsUseSavingRoll;

	if (iniSettings.LoadFile(sConfigPathSettingsBase.c_str()) > -1) {
		_MESSAGE("\nLoading settings from base ini file...");
		bAllowFullMitigationTmp = iniSettings.GetBoolValue("DamageTweaks", "bAllowFullMitigation", false);
		iDTTypeTmp = iniSettings.GetLongValue("DamageTweaks", "iDTType", iDTType);
		fDTMinDmgPercentTmp = iniSettings.GetDoubleValue("DamageTweaks", "fDTMinDmgPercent", fDTMinDmgPercent);
		iDRTypeTmp = iniSettings.GetLongValue("DamageTweaks", "iDRType", iDRType);
		fMaxPercentDRTmp = iniSettings.GetDoubleValue("DamageTweaks", "fMaxPercentDR", fMaxPercentDR);
		bEnableArmorCNDTmp = iniSettings.GetBoolValue("DamageTweaks", "bEnableArmorCND", bEnableArmorCND);
		iArmorDmgChanceTmp = iniSettings.GetLongValue("DamageTweaks", "iArmorDmgChance", iArmorDmgChance);
		fArmorDmgMultiplierTmp = iniSettings.GetDoubleValue("DamageTweaks", "fArmorDmgMult", fArmorDmgMultiplier);
		bEnableCriticalHitsTmp = iniSettings.GetBoolValue("DamageTweaks", "bEnableCriticalHits", bEnableCriticalHits);
		bCritDmgMultRollTmp = iniSettings.GetBoolValue("DamageTweaks", "bCritDmgMultRoll", bCritDmgMultRoll);
		bCritChanceUsesCNDTmp = iniSettings.GetBoolValue("DamageTweaks", "bCritChanceUsesCND", bCritChanceUsesCND);
		bCritsUseEffectTablesTmp = iniSettings.GetBoolValue("DamageTweaks", "bCritsUseEffectTables", bCritsUseEffectTables);
		bCritsUseSavingRollTmp = iniSettings.GetBoolValue("DamageTweaks", "bCritsUseSavingRoll", bCritsUseSavingRoll);
	}
	else {
		_MESSAGE("\nERROR: Base Settings ini is missing! Using defaults...");
	}

	if (iniSettings.LoadFile(sConfigPathSettingsCur.c_str()) > -1) {
		_MESSAGE("\nLoading settings from current ini file...");
		bAllowFullMitigation = iniSettings.GetBoolValue("DamageTweaks", "bAllowFullMitigation", bAllowFullMitigationTmp);
		iDTType = iniSettings.GetLongValue("DamageTweaks", "iDTType", iDTTypeTmp);
		fDTMinDmgPercent = iniSettings.GetDoubleValue("DamageTweaks", "fDTMinDmgPercent", fDTMinDmgPercentTmp);
		iDRType = iniSettings.GetLongValue("DamageTweaks", "iDRType", iDRTypeTmp);
		fMaxPercentDR = iniSettings.GetDoubleValue("DamageTweaks", "fMaxPercentDR", fMaxPercentDRTmp);
		bEnableArmorCND = iniSettings.GetBoolValue("DamageTweaks", "bEnableArmorCND", bEnableArmorCNDTmp);
		iArmorDmgChance = iniSettings.GetLongValue("DamageTweaks", "iArmorDmgChance", iArmorDmgChanceTmp);
		fArmorDmgMultiplier = iniSettings.GetDoubleValue("DamageTweaks", "fArmorDmgMult", fArmorDmgMultiplierTmp);
		bEnableCriticalHits = iniSettings.GetBoolValue("DamageTweaks", "bEnableCriticalHits", bEnableCriticalHitsTmp);
		bCritDmgMultRoll = iniSettings.GetBoolValue("DamageTweaks", "bCritDmgMultRoll", bCritDmgMultRollTmp);
		bCritChanceUsesCND = iniSettings.GetBoolValue("DamageTweaks", "bCritChanceUsesCND", bCritChanceUsesCNDTmp);
		bCritsUseEffectTables = iniSettings.GetBoolValue("DamageTweaks", "bCritsUseEffectTables", bCritsUseEffectTablesTmp);
		bCritsUseSavingRoll = iniSettings.GetBoolValue("DamageTweaks", "bCritsUseSavingRoll", bCritsUseSavingRollTmp);
	}
	else {
		_MESSAGE("\nWARNING: Current Settings ini is missing! using defaults from base config");
		bAllowFullMitigation = bAllowFullMitigationTmp;
		iDTType = iDTTypeTmp;
		fDTMinDmgPercent = fDTMinDmgPercentTmp;
		iDRType = iDRTypeTmp;
		fMaxPercentDR = fMaxPercentDRTmp;
		bEnableArmorCND = bEnableArmorCNDTmp;
		iArmorDmgChance = iArmorDmgChanceTmp;
		fArmorDmgMultiplier = fArmorDmgMultiplierTmp;
		bEnableCriticalHits = bEnableCriticalHitsTmp;
		bCritDmgMultRoll = bCritDmgMultRollTmp;
		bCritChanceUsesCND = bCritChanceUsesCNDTmp;
		bCritsUseEffectTables = bCritsUseEffectTablesTmp;
		bCritsUseSavingRoll = bCritsUseSavingRollTmp;
	}

	fMinDmg = bAllowFullMitigation ? 0.0 : 1.0;
}


// config data init
bool DTConfig::LoadConfigData()
{
	// ---- Main ini

	CSimpleIniA iniMain;
	iniMain.SetUnicode();
	if (iniMain.LoadFile(".\\Data\\F4SE\\Plugins\\DamageTweaks.ini") > -1) {
		// -- Paths
		_MESSAGE("\nLoading paths from DamageTweaks.ini");
		sConfigPathSettingsBase = iniMain.GetValue("Settings", "sIniPathBase", ".\\Data\\MCM\\Config\\DamageTweaks\\settings.ini");
		sConfigPathSettingsCur = iniMain.GetValue("Settings", "sIniPathCur", ".\\Data\\MCM\\Settings\\DamageTweaks.ini");
		sConfigPathData = iniMain.GetValue("Settings", "sModDataPath", ".\\Data\\F4SE\\Config\\DamageTweaks\\");

		// -- Custom AVs
		std::string formIDStr = iniMain.GetValue("ActorValues", "sCritTargetAV", "F4TweaksFramework.esm|825");
		avCritTableCheck = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sCritRollModAV", "F4TweaksFramework.esm|823");
		avCritRollMod = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sCritTargetRollModAV", "F4TweaksFramework.esm|824");
		avTargetCritRollMod = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sCritDamageMultAV", "F4TweaksFramework.esm|820");
		avCritDmgMult = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sCritChanceAddAV", "F4TweaksFramework.esm|FC9");
		avCritChanceAdd = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sWeaponCNDPctAV", "F4TweaksFramework.esm|3E11");
		avWeaponCND = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sArmorPenThrownAV", "F4TweaksFramework.esm|81E");
		avArmorPenetrationThrown = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sTargetArmorMultAV", "F4TweaksFramework.esm|6457");
		avTargetArmorMult = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sTargetArmorMultThrownAV", "F4TweaksFramework.esm|6BF2");
		avTargetArmorMultThrown = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);

		formIDStr = iniMain.GetValue("ActorValues", "sLastHeadConditionAV", "F4TweaksFramework.esm|FC3");
		avLastHeadCND = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sLastTorsoConditionAV", "F4TweaksFramework.esm|FC4");
		avLastTorsoCND = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sLastArmLConditionAV", "F4TweaksFramework.esm|FC5");
		avLastArmLCND = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sLastArmRConditionAV", "F4TweaksFramework.esm|FC6");
		avLastArmRCND = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sLastLegLConditionAV", "F4TweaksFramework.esm|FC7");
		avLastLegLCND = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sLastLegRConditionAV", "F4TweaksFramework.esm|FC8");
		avLastLegRCND = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("ActorValues", "sDamageArmorCheckAV", "F4TweaksFramework.esm|101C");
		avDamageArmorCheck = (ActorValueInfo*)GetFormFromIdentifier(formIDStr);
		formIDStr = iniMain.GetValue("Keywords", "sCanHaveArmorCNDKW", "F4TweaksFramework.esm|2EF6");
		kwCanHaveArmorDamage = (BGSKeyword*)GetFormFromIdentifier(formIDStr);
	}
	else {
		_MESSAGE("\nERROR: DamageTweaks.ini is missing or corrupted! Using defaults... This will only work if F4TweaksFramework.esm is loaded.");

		// -- Custom AVs
		avCritTableCheck = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|825");
		avCritRollMod = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|823");
		avTargetCritRollMod = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|824");
		avCritDmgMult = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|820");
		avCritChanceAdd = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|FC9");
		avWeaponCND = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|3E11");
		avArmorPenetrationThrown = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|81E");
		avTargetArmorMult = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|6457");
		avTargetArmorMultThrown = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|6BF2");
		avLastHeadCND = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|FC3");
		avLastTorsoCND = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|FC4");
		avLastArmLCND = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|FC5");
		avLastArmRCND = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|FC6");
		avLastLegLCND = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|FC7");
		avLastLegRCND = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|FC8");
		avDamageArmorCheck = (ActorValueInfo*)GetFormFromIdentifier("F4TweaksFramework.esm|101C");
		kwCanHaveArmorDamage = (BGSKeyword*)GetFormFromIdentifier("F4TweaksFramework.esm|2EF6");
	}

	// ---- Settings
	UpdateSettings();

	// ---- Base Game AVs

	avArmorPenetration = (ActorValueInfo*)LookupFormByID(0x97341);

	// limd cnd
	avHeadCND = (ActorValueInfo*)LookupFormByID(0x36C);
	avTorsoCND = (ActorValueInfo*)LookupFormByID(0x36D);
	avArmLCND = (ActorValueInfo*)LookupFormByID(0x36E);
	avArmRCND = (ActorValueInfo*)LookupFormByID(0x36F);
	avLegLCND = (ActorValueInfo*)LookupFormByID(0x370);
	avLegRCND = (ActorValueInfo*)LookupFormByID(0x371);

	// creature attack damage AVs + KWs
	avUnarmedDamage = (ActorValueInfo*)LookupFormByID(0x2DF);
	kwAnimsUnarmed = (BGSKeyword*)LookupFormByID(0x2405E);
	avCrRangedDamage = (ActorValueInfo*)LookupFormByID(0x1504FB);
	kwCrRanged = (BGSKeyword*)LookupFormByID(0x189348);

	// SPECIAL
	specialAVs.push_back((ActorValueInfo*)LookupFormByID(0x2C2));
	specialAVs.push_back((ActorValueInfo*)LookupFormByID(0x2C3));
	specialAVs.push_back((ActorValueInfo*)LookupFormByID(0x2C4));
	specialAVs.push_back((ActorValueInfo*)LookupFormByID(0x2C5));
	specialAVs.push_back((ActorValueInfo*)LookupFormByID(0x2C6));
	specialAVs.push_back((ActorValueInfo*)LookupFormByID(0x2C7));
	specialAVs.push_back((ActorValueInfo*)LookupFormByID(0x2C8));
	
	// ---- JSON data

	// -- damage types
	std::string sDmgTypesFilePath;
	std::vector<std::string> dmgTypesPathsList;
	std::string sDmgTypesFolderPathStr;
	sDmgTypesFolderPathStr.append(sConfigPathData.c_str());
	sDmgTypesFolderPathStr.append("DamageTypes\\");
	dmgTypesPathsList = GetFileNames(sDmgTypesFolderPathStr);
	if (!dmgTypesPathsList.empty()) {
		_MESSAGE("\n  Loading %i DamageTypes from %s", dmgTypesPathsList.size(), sDmgTypesFolderPathStr.c_str());
		for (std::vector<std::string>::iterator itFile = dmgTypesPathsList.begin(); itFile != dmgTypesPathsList.end(); ++itFile) {
			sDmgTypesFilePath.clear();
			sDmgTypesFilePath.append(sDmgTypesFolderPathStr);
			sDmgTypesFilePath.append(*itFile);
			DmgTypeData curResist = DmgTypeData();
			if (BuildData_DamageType(sDmgTypesFilePath, curResist)) {
				damageTypes.push_back(curResist);
			}
			else {
				_MESSAGE("\n    ERROR: Failed to create DamageType from data at %s", sDmgTypesFilePath.c_str());
			}
		}
		dmgTypesPathsList.clear();
	}
	else {
		_MESSAGE("  ERROR: No DamageTypes found at %s", sDmgTypesFolderPathStr.c_str());
	}

	if (damageTypes.empty()) {
		_MESSAGE("  ERROR: final damage types list was empty...");
		return false;
	}

	// -- critical effect tables
	std::string sCurFilePath;
	std::vector<std::string> configPathsList;
	std::string sFolderPathStr;
	sFolderPathStr.append(sConfigPathData.c_str());
	sFolderPathStr.append("CritEffectTables\\");
	configPathsList = GetFileNames(sFolderPathStr);
	if (!configPathsList.empty()) {
		_MESSAGE("\n  Loading %i Critical Effect Tables from %s...", configPathsList.size(), sFolderPathStr.c_str());
		for (std::vector<std::string>::iterator itFile = configPathsList.begin(); itFile != configPathsList.end(); ++itFile) {
			sCurFilePath.clear();
			sCurFilePath.append(sFolderPathStr);
			sCurFilePath.append(*itFile);
			CritEffectTable newCritTable = CritEffectTable();
			if (BuildData_CriticalEffectTable(sCurFilePath, newCritTable)) {
				critEffectTables.push_back(newCritTable);
			}
			else {
				_MESSAGE("\n    ERROR: Failed to create crit effect table from data at %s!", sCurFilePath.c_str());
			}
		}
		configPathsList.clear();
	}
	else {
		_MESSAGE("\n  ERROR: No Crit Effect Tables found at %s", sFolderPathStr.c_str());
	}

	if (critEffectTables.empty()) {
		_MESSAGE("  ERROR: final crit effect tables list was empty...");
		return false;
	}

	// ---- Seed the rng
	rng.Seed();

	_MESSAGE("\nReady.");
	return true;
}


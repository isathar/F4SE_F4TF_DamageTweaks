#include "f4se_common/f4se_version.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se/PluginAPI.h"
#include "f4se/GameThreads.h"

#include <shlobj.h>
#include <memory>
#include <thread>

#include "Config.h"
#include "DamageTweaks.h"


IDebugLog						gLog;
PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
F4SEMessagingInterface			* g_messaging = NULL;
F4SEPapyrusInterface			* g_papyrus = NULL;


/** Actor::ProcessDamageFrame
		sig: 48 8B C4 48 89 50 10 55 56 41 56 41 57
		address:
			v1.10.163: 0xE01630
		credit: kassent (https://github.com/kassent/FloatingDamage) */
using _Process = void(*)(void *, DamageFrame *);
RelocAddr<_Process>	ProcessDamageFrame = 0xE01630;


SimpleLock globalDamageLock;
class ActorEx : public Actor
{
public:
	static void ProcessDamageFrame_Hook(Actor * pObj, DamageFrame * pDamageFrame)
	{
		if (pDamageFrame && (pDamageFrame->unk94 > 0.0)) {
			// lock the thread
			globalDamageLock.Lock();

			TESObjectREFR * victim = nullptr;
			TESObjectREFR * attacker = nullptr;

			TESObjectWEAP * weaponForm = nullptr;
			TESObjectWEAP::InstanceData * weaponInstance = nullptr;

			float fBaseDmg = pDamageFrame->unk94;
			bool bEditedDamage = false;

			UInt8 iSourceType = 0;
			float fFinalDmg = 0.0;
			float fTmpDmg1 = pDamageFrame->damage;
			float fTmpDmg2 = pDamageFrame->damage2;
			// multiplier from hit bodypart, perks, etc.
			float fFinalDmgMult = (fTmpDmg1 > 0.0 && fTmpDmg2 > 0.0) ? fTmpDmg2 / fTmpDmg1 : 1.0;
			
			if ((LookupREFRByHandle(&pDamageFrame->victimHandle, &victim), victim != nullptr) && victim->formType == FormType::kFormType_ACHR) {
				if ((LookupREFRByHandle(&pDamageFrame->attackerHandle, &attacker), attacker != nullptr) && attacker->formType == FormType::kFormType_ACHR)
				{
					if (pDamageFrame->damageSourceForm) {
						if (pDamageFrame->damageSourceForm->formType == kFormType_WEAP) {
							weaponForm = reinterpret_cast<TESObjectWEAP*>(pDamageFrame->damageSourceForm);
						}
					}

					if (pDamageFrame->instanceData) {
						weaponInstance = reinterpret_cast<TESObjectWEAP::InstanceData*>(pDamageFrame->instanceData);
					}
					else if (weaponForm) {
						weaponInstance = &weaponForm->weapData;
					}

					if (weaponInstance) {
						// -- regular weapon
						fFinalDmg = DamageTweaks::CalcDamage(attacker, victim, weaponForm, weaponInstance, fBaseDmg, fFinalDmgMult);
						//DamageTweaks::DumpDamageFrame(pDamageFrame);
						pDamageFrame->damage = (pDamageFrame->damage > 0.0) ? fFinalDmg / fFinalDmgMult : 0.0;
						pDamageFrame->damage2 = fFinalDmg;
						bEditedDamage = true;
						_MESSAGE("\n  %s hit %s for %.4f damage.", attacker->baseForm->GetFullName(), victim->baseForm->GetFullName(), fFinalDmg);
					}
					else {
						if (pDamageFrame->attackData) {
							// -- some creature/unarmed weapons
							fFinalDmg = DamageTweaks::CalcDamage(attacker, victim, weaponForm, nullptr, fBaseDmg, fFinalDmgMult);
							//DamageTweaks::DumpDamageFrame(pDamageFrame);
							pDamageFrame->damage = (pDamageFrame->damage > 0.0) ? fFinalDmg / fFinalDmgMult : 0.0;
							pDamageFrame->damage2 = fFinalDmg;
							bEditedDamage = true;
							_MESSAGE("\n  %s hit %s for %.4f damage.", attacker->baseForm->GetFullName(), victim->baseForm->GetFullName(), fFinalDmg);
						}
						else {
							_MESSAGE("\nDEBUG: Edge case: no weapon instance or attackData");
						}
					}
				}
				else {
					if (pDamageFrame->damageSourceForm) {
						_MESSAGE("\nDEBUG: Edge case: no attacker or attacker is not a character but damageSourceForm exists");
					}
					else {
						_MESSAGE("\nDEBUG: Edge case: no attacker or attacker is not a character");
					}
				}
			}
			else {
				_MESSAGE("\nDEBUG: Edge case: no victim or victim is not a character");
			}
			
			// remove DR from anything missed
			if (!bEditedDamage) {
				DamageTweaks::DumpDamageFrame(pDamageFrame);
				pDamageFrame->damage = (pDamageFrame->damage > 0.0) ? pDamageFrame->unk94 : 0.0;
				pDamageFrame->damage2 = pDamageFrame->unk94;
			}

			// unlock the thread
			globalDamageLock.Release();
			ProcessDamageFrame(pObj, pDamageFrame);

			// clean up victim ref
			if (victim != nullptr) {
				victim->handleRefObject.DecRefHandle();
			}
			// clean up attacker ref
			if (attacker != nullptr) {
				attacker->handleRefObject.DecRefHandle();
			}
		}
		else {
			_MESSAGE("ERROR: no damageFrame or damage <= 0.0");
			ProcessDamageFrame(pObj, pDamageFrame);
		}
	}


	static void InitHooks()
	{
		// updates ProcessDamageFrame's memory address after a game binary update
		//RELOC_GLOBAL_VAL(ProcessDamageFrame, "48 8B C4 48 89 50 10 55 56 41 56 41 57");

		g_branchTrampoline.Write5Call(RELOC_RUNTIME_ADDR("E8 ? ? ? ? 48 85 FF 74 36 48 8B CF"), (uintptr_t)ProcessDamageFrame_Hook);
	}
};


void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	if (msg->type == F4SEMessagingInterface::kMessage_GameDataReady) {
		if (msg->data) {
			DTConfig::LoadConfigData();
		}
	}
}


extern "C"
{
	bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\F4TF_DamageTweaks.log");

		_MESSAGE("%s: %08X", PLUGIN_NAME, PLUGIN_VERSION);

		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = PLUGIN_NAME;
		info->version = PLUGIN_VERSION;
		plugin_info.plugin_name = PLUGIN_NAME;
		plugin_info.runtime_version = f4se->runtimeVersion;
		
		// version check
		if (f4se->runtimeVersion != SUPPORTED_RUNTIME_VERSION) {
			char buf[512];
			sprintf_s(buf, "DamageTweaks does not work with the installed game version!\n\nExpected: %d.%d.%d.%d\nCurrent:  %d.%d.%d.%d",
				GET_EXE_VERSION_MAJOR(SUPPORTED_RUNTIME_VERSION),
				GET_EXE_VERSION_MINOR(SUPPORTED_RUNTIME_VERSION),
				GET_EXE_VERSION_BUILD(SUPPORTED_RUNTIME_VERSION),
				GET_EXE_VERSION_SUB(SUPPORTED_RUNTIME_VERSION),
				GET_EXE_VERSION_MAJOR(f4se->runtimeVersion),
				GET_EXE_VERSION_MINOR(f4se->runtimeVersion),
				GET_EXE_VERSION_BUILD(f4se->runtimeVersion),
				GET_EXE_VERSION_SUB(f4se->runtimeVersion));
			MessageBox(NULL, buf, "Game Version Error", MB_OK | MB_ICONEXCLAMATION);
			_FATALERROR("ERROR: Game version mismatch");
			return false;
		}
		
		g_pluginHandle = f4se->GetPluginHandle();
		g_messaging = (F4SEMessagingInterface *)f4se->QueryInterface(kInterface_Messaging);
		if (!g_messaging) {
			_FATALERROR("ERROR: Messaging query failed");
			return false;
		}
		g_papyrus = (F4SEPapyrusInterface *)f4se->QueryInterface(kInterface_Papyrus);
		if (!g_papyrus) {
			_FATALERROR("ERROR: Papyrus query failed");
			return false;
		}
		return true;
	}

	bool F4SEPlugin_Load(const F4SEInterface * f4se)
	{
		if (!g_branchTrampoline.Create(1024 * 64)) {
			_FATALERROR("ERROR: The trampoline just experienced its last bounce. Wait for a mod update.");
			return false;
		}

		// initialize ProcessDamageFame hook (credit: kassent)
		try
		{
			sig_scan_timer timer;
			ActorEx::InitHooks();
		}
		catch (const no_result_exception & exception)
		{
			_MESSAGE(exception.what());
			MessageBoxA(nullptr, "ERROR: Signature scan failed... DamageTweaks will not work with this game version.", "DamageTweaks", MB_OK);
			return false;
		}

		if (g_messaging) {
			g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);
		}
		if (g_papyrus) {
			g_papyrus->Register(PapyrusDamageTweaks::RegisterPapyrus);
		}
		return true;
	}
};





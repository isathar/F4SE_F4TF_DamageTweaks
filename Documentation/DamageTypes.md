# Damage Types


--------------------------------------------------------------
## Variables

- **sName** *(String - optional)*
	- damage type name for organization and logging
- **sDmgTypeID** *(FormID String - required)*
	- FormID of the DamageType
	- required even if using a damage type as a dummy
- **iCritTableID** *(int - optional)*
	- index of the critical hit table to use
- **sDmgResistAV** *(FormID String - optional)*
	- FormID of the resistance ActorValue to use
- **sDmgThresholdAV** *(FormID String - optional)*
	- FormID of the damage threshold ActorValue to use
- **bUseArmorPen** *(bool - optional)*
	- if true, allow armor penetration for this damage type

--------------------------------------------------------------

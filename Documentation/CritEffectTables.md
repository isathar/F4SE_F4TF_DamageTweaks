# Critical Effect Tables

- The table to use for a target actor is determined by *requiredKW*, a keyword assigned to actors or races.
- The default tables use the game's *ActorTypeX* keywords.
- Each actor type's tables include a set of critical effects to use for each weapon and damage type that can cause a critical roll.
- If a weapon/damageType's effect list doesn't exist, the actor type is immune to critical effects of the specified type.

----------------------------------------------------------------------------------------------------
## Variables

- **sRequiredKW** *(FormID String - required)*
	- actor keyword required to use this set of effect tables
- **critTables** *(list - required)*
	- the critical effect tables for this actor type
	- **sName** *(String - optional)*
		- crit table name used for organization
	- **iCritTypeID** *(int - required)*
		- index of the DamageType that triggers effects from this table
	- **sImmuneKW** *(FormID String - optional)*
		- actor keyword that specifies if a target is immune to this effect type
	- **effects** *(list - required)* 
		- the list of critical effects for this table
		- **iRollMax** *(int - required)* 
			- the maximum roll value for this crit effect - should be < 256
		- **sSpellID** *(FormID String - required)* 
			- the critical effect spell to apply (or none)
		- **sSpellIDSaved** *(FormID String - optional)* 
			- the critical spell to apply if the target passes its saving roll (or none)
		- **iSavingRollAV** *(int - optional)* 
			- the index of the SPECIAL attribute to use in the saving roll (or -1 for none)
		- **iSavingRollMod** *(int - optional)* 
			- the modifier to use for the saving roll (positive=easier, negative=harder)

----------------------------------------------------------------------------------------------------

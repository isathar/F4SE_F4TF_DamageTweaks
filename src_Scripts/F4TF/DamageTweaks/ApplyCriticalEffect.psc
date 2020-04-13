scriptname F4TF:DamageTweaks:ApplyCriticalEffect extends ActiveMagicEffect


Event OnEffectStart(Actor akTarget, Actor akCaster)
	if (akCaster)
		Spell CritSpell = F4TF:Globals.GetCritEffect(akCaster)
		if (CritSpell)
			akCaster.DoCombatSpellApply(critSpell, akCaster)
			debug.notification(critSpell.GetName())
		endIf
	endIf
EndEvent

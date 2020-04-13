scriptname F4TF:DamageTweaks:ApplyArmorDamage extends ActiveMagicEffect


Event OnEffectStart(Actor akTarget, Actor akCaster)
	if (akCaster)
		Form UnequipArmor = F4TF:Globals.UpdateArmorCND(akCaster)
		if (UnequipArmor != none)
			akCaster.UnequipItem(UnequipArmor)
			debug.notification("damaged" + UnequipArmor.GetName())
		endIf
	endIf
EndEvent

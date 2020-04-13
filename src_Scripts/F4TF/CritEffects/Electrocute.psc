Scriptname F4TF:CritEffects:Electrocute extends ActiveMagicEffect 
{critical electrocution effect}


Group CritFX
	EffectShader Property 	pEffectShader 			auto const
	{Effect shader to play}
	float Property 			fEffectDuration = 		4.0 	auto const
	
	EffectShader Property 	pSkelEffectShader 		auto const
	{Effect shader to play if not an ashpile}
	float Property 			fSkelEffectDuration = 	4.0 	auto const
	
	Armor Property 			pCritSkelArmor			auto const
	{critskeleton armor to equip}
	int Property 			iCritSkelChance = 		50 		auto const
	
	
	Race Property 	pHumanRace						auto const
	Race Property 	pGhoulRace						auto const
EndGroup

int Property 			iTimer_Explode = 		13 		AutoReadOnly

bool bCanHaveSkel = false


; make sure the target is dead before starting
Event OnEffectStart(Actor akTarget, Actor akCaster)
	Race tempRace = akTarget.GetRace()
	bCanHaveSkel = ((tempRace == pHumanRace) || (tempRace == pGhoulRace))
	StartCritEffect(akTarget)
EndEvent


Event OnTimer(int aiTimerID)
	if (aiTimerID == iTimer_Explode)
		Disintegrate(GetTargetActor())
	endIf
EndEvent


Function StartCritEffect(Actor Victim)
	if (Victim != none)
		if ((fEffectDuration > 0.0) && (pEffectShader != none))
			pEffectShader.Play(Victim, fEffectDuration)
			StartTimer(fEffectDuration, iTimer_Explode)
		else
			Disintegrate(Victim)
		endIf
	endIf
EndFunction


Function Disintegrate(Actor Victim)
	if (Victim != none)
		Victim.Dismember("Torso", true, true, true)
		Victim.Kill(GetCasterActor())
		
		if (bCanHaveSkel)
			if (pCritSkelArmor != none)
				if (utility.RandomInt(0,100) < iCritSkelChance)
					Victim.EquipItem(pCritSkelArmor)
					if ((fSkelEffectDuration > 0.0) && (pSkelEffectShader != none))
						pSkelEffectShader.Play(Victim, fSkelEffectDuration)
					endIf
				endIf
			endIf
		endIf
	endIf
Endfunction


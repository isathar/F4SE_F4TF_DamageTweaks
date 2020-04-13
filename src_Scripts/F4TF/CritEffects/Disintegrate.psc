Scriptname F4TF:CritEffects:Disintegrate extends ActiveMagicEffect 
{critical disintegration effects}


Group CritFX
	EffectShader Property 	pEffectShader 			auto const
	{Effect shader to play}
	EffectShader Property 	pSkelEffectShader 		auto const
	{Effect shader to play if not an ashpile}
	
	Activator Property 		pAshPile 				auto const
	{ash pile object}
	Armor Property 			pCritSkelArmor			auto const
	{critskeleton armor to equip}
	
	Form Property 	pItemAdd					 	auto const
	{item to add to the target's inventory}
	
	bool Property 	bKillNow = 				false 	auto const
	{kill the target instantly}
	bool Property 	bRemoveItems = 		false	auto const
	{remove target's inventory items}
	
	float Property 	fEffectDuration = 		4.0 	auto const
	float Property 	fSkelEffectDuration = 	4.0 	auto const
	int Property 	iCritSkelChance = 		50 		auto const
	
	int Property 	iCritState_Start = 		3 		auto const
	int Property 	iCritState_End = 		4 		auto const
EndGroup

int Property 	iTimer_AshPile = 		13 		AutoReadOnly

bool bCanHaveSkel = false
bool bStartedEffect = false


; make sure the target is dead before starting
Event OnEffectStart(Actor akTarget, Actor akCaster)
	akTarget.SetCriticalStage(iCritState_Start)
	
	if (akTarget.isDead())
		bStartedEffect = true
		StartCritEffect(akTarget)
	else
		if (bKillNow)
			bStartedEffect = true
			akTarget.Kill(akCaster)
			StartCritEffect(akTarget)
		endIf
	endIf
EndEvent


Event OnTimer(int aiTimerID)
	if (aiTimerID == iTimer_AshPile)
		Disintegrate(GetTargetActor())
	endIf
EndEvent

Event OnDying(Actor akKiller)
	if (!bStartedEffect)
		bStartedEffect = true
		StartCritEffect(GetTargetActor())
	endIf
EndEvent


Function StartCritEffect(Actor Victim)
	if (Victim != none)
		if (pCritSkelArmor != none)
			Victim.EquipItem(pCritSkelArmor, true, true)
		endIf
		
		if ((fEffectDuration > 0.0) && (pEffectShader != none))
			pEffectShader.Play(Victim, fEffectDuration)
			StartTimer(fEffectDuration, iTimer_AshPile)
		else
			Disintegrate(Victim)
		endIf
	endIf
EndFunction


Function Disintegrate(Actor Victim)
	if (Victim != none)
		if (utility.RandomInt(0,100) < iCritSkelChance)
			CreateAshpile(Victim)
		endIf
	endIf
Endfunction


Function CreateAshpile(Actor Victim)
	bool bRemove = false
	
	if (pItemAdd)
		Victim.AddItem(pItemAdd, 1, true)
	endIf
	
	
	utility.WaitMenuMode(0.02)
	Victim.AttachAshPile(pAshPile)
	Victim.SetAlpha (0.0, True)
	Victim.SetCriticalStage(iCritState_End)
EndFunction


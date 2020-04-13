Scriptname F4TF:CritEffects:ImpactSpray extends ActiveMagicEffect 
{modified FXScatterImpactEffectsOnDeath}


Group ImpactFX
	ImpactDataSet Property 	pFXImpactData 					auto const mandatory
	{Impact data set to spray around}
	
	float Property 			fFXimpactDelay_Min = 	0.20 	auto const
	{min. delay between sprays}
	float Property 			fFXimpactDelay_Max = 	0.75 	auto const
	{max. delay between sprays}
	float Property 			fFXimpactDelay_Start = 	0.00 	auto const
	{initial delay before starting effect}
	
	int Property 			iFXimpactCount = 		5 		auto const
	{max. number of impacts to spray}
	
EndGroup


Actor Victim = none
int iImpactCounter = 0

int iTimer_FXImpactSpawn = 3 const



Event OnEffectStart(Actor akTarget, Actor akCaster)
	if (akTarget.isDead())
		StartCriticalEffect(akTarget)
	else
		GoToState("EffectOnDeath")
	endIf
EndEvent

state EffectOnDeath
	Event OnDying(Actor akKiller)
		StartCriticalEffect(GetTargetActor())
	EndEvent
endState


Function StartCriticalEffect(Actor akTarget)
	Victim = akTarget
	
	if fFXimpactDelay_Start <= 0.0
		Victim.PlayImpactEffect(pFXImpactData, "SprayHelperNode", Utility.RandomFloat(-0.8, 0.8), Utility.RandomFloat(-0.75, 0.75), -1.0, 320, false, true)
		iImpactCounter = iFXimpactCount - 1
		if (iImpactCounter > 0)
			if (fFXimpactDelay_Min > 0.0)
				StartTimer(utility.RandomFloat(fFXimpactDelay_Min, fFXimpactDelay_Max), iTimer_FXImpactSpawn)
			endIf
		endIf
	else
		iImpactCounter = iFXimpactCount
		StartTimer(fFXimpactDelay_Start, iTimer_FXImpactSpawn)
	endIf
EndFunction


Event OnTimer(int aiTimerID)
	Victim.PlayImpactEffect(pFXImpactData, "SprayHelperNode", Utility.RandomFloat(-0.8, 0.8), Utility.RandomFloat(-0.75, 0.75), -1.0, 320, false, true)
	iImpactCounter -= 1
	if iImpactCounter > 0
		if (fFXimpactDelay_Min > 0.0)
			StartTimer(utility.RandomFloat(fFXimpactDelay_Min, fFXimpactDelay_Max), iTimer_FXImpactSpawn)
		endIf
	endIf
EndEvent


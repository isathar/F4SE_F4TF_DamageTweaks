Scriptname F4TF:CritEffects:KnockBack extends ActiveMagicEffect  
{knocks back the target}


float property fPushForce = 15.0 Auto Const


Event OnEffectStart(Actor akTarget, Actor akCaster)
	if (akTarget)
		; this may not be the actual source of the crit effect if fighting multiple opponents...
		; an alternative is to drop an activator and use it as the source, which is not great for performance
		Actor combatTarget = akTarget.GetCombatTarget()
		if (combatTarget)
			int finalForce = (fPushForce * (Utility.RandomFloat(0.6,1.4))) as int
			combatTarget.PushActorAway(akTarget, finalForce)
		else
			; no comatTarget, so just kind of fall over, instead
			akTarget.PushActorAway(akTarget, -1)
		endIf
	endIf
EndEvent

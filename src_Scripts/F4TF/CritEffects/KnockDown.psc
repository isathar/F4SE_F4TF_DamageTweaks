Scriptname F4TF:CritEffects:KnockDown extends ActiveMagicEffect  
{knocks the target down for the effect's duration}

ActorValue Property Paralysis auto const mandatory

InputEnableLayer tempInputDisable = none


Event OnEffectStart(Actor akTarget, Actor akCaster)
	if (akTarget.GetValue(Paralysis)) < 1.0
		; disable controls if player
		if akTarget == Game.GetPlayer()
			tempInputDisable = InputEnableLayer.Create()
			tempInputDisable.EnableJournal(false)
			tempInputDisable.EnableVATS(false)
		endIf
		akTarget.SetValue(Paralysis, 1.0)
		akTarget.SetUnconscious(true)
		akTarget.PushActorAway(akTarget, -1)
	endIf
EndEvent

; hopefully this is called on death dispel...
Event OnEffectFinish(Actor akTarget, Actor akCaster)
	if (akTarget.GetValue(Paralysis)) > 0.0
		; re-enable controls if player
		if akTarget == Game.GetPlayer()
			if tempInputDisable != none
				tempInputDisable.Reset()
				tempInputDisable.Delete()
			endIf
		endIf
		akTarget.SetValue(Paralysis, 0.0)
		akTarget.SetUnconscious(false)
	endIf
EndEvent

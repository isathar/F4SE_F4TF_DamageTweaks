scriptname F4TF:CritEffects:ElectricalArcs extends ActiveMagicEffect


Spell property TrapElectricArcSpell auto Const
Form property xmarker auto Const

float property fDistance = 256.0 auto const
int Property iTimesToFire = 5 auto const
float Property fRefireDelay = 1.0 auto const
float Property fChanceToArc = 0.5 auto const


int iTimerID_Arcs = 1 const

Actor victim = none
ObjectReference dummyTarget = none
int iTimesFired = 0


Event OnEffectStart(Actor akTarget, Actor akCaster)
	victim = akTarget
	Spark()
EndEvent


Event OnTimer(int aiTimerID)
	if (victim != none)
		if (aiTimerID == iTimerID_Arcs)
			Spark()
		endIf
	endIf
EndEvent


Function Spark()
	if (fChanceToArc <= Utility.RandomFloat())
		Actor newTarget = Game.FindRandomActorFromRef(victim, fDistance)
		if (newTarget != none)
			TrapElectricArcSpell.Cast(victim, newTarget)
		else
			if (dummyTarget == none)
				dummyTarget = victim.placeAtMe(xmarker)
			endIf
			dummyTarget.moveto(victim, afXOffset = utility.RandomFloat(-fDistance, fDistance),  afYOffset = utility.RandomFloat(-fDistance, fDistance), afZOffset = utility.RandomFloat(-fDistance, fDistance))
			TrapElectricArcSpell.Cast(victim, dummyTarget)
		endIf
	endIf
	
	iTimesFired += 1
	if (iTimesFired < iTimesToFire)
		StartTimer(fRefireDelay, iTimerID_Arcs)
	else
		if (dummyTarget != none)
			dummyTarget.DeleteWhenAble()
			dummyTarget.Disable()
		endIf
	endIf
EndFunction



scriptname F4TF:EventManager extends Quest


Event OnQuestInit()
	RegisterForExternalEvent("OnMCMMenuClose", "OnMCMMenuClose")
EndEvent

; update settings when MCM closes
Function OnMCMMenuClose(string modName)
	debug.notification("MCM menu closed: " + modName)
	if (modName == "F4TweaksFramework")
		F4TF:Globals.UpdateSettings()
	endIf
EndFunction

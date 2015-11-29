/**
 * Wrapper class that hosts the ScopeSync object and its GUI
 * on behalf of the Scope (or Scope SDK) applications, using 
 * the ScopeFX SDK. This requires it to be derived from the Effect
 * class.
 *
 * Also operates as a Timer source for the ScopeSync and ScopeFXGUI
 * objects.
 *
 *  (C) Copyright 2014 bcmodular (http://www.bcmodular.co.uk/)
 *
 * This file is part of ScopeSync.
 *
 * ScopeSync is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * ScopeSync is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ScopeSync.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contributors:
 *  Simon Russell
 *  Will Ellis
 *  Jessica Brandt
 */
#include "ScopeFX.h"
#include "ScopeFXGUI.h"
#include "../../ScopeSyncShared/Resources/ImageLoader.h"
#include "../../ScopeSyncShared/Resources/Icons.h"
#include "../../ScopeSyncShared/Core/ScopeSyncApplication.h"
#include "../../ScopeSyncShared/Windows/UserSettings.h"
#include "../../ScopeSyncShared/Core/BCMParameterController.h"

const int ScopeFX::initPositionX           = 100;
const int ScopeFX::initPositionY           = 100;
const int ScopeFX::windowHandlerDelayMax   = 6;
const int ScopeFX::timerFrequency          = 20;

using namespace ScopeFXParameterDefinitions;

const int ScopeFX::getInputIndexForValue(int index)
{
	switch (index)
	{
	case 0:  return INPAD_PERFORMANCE_MODE;
	case 1:  return INPAD_OSCUID;
	case 2:  return INPAD_DEVICE_TYPE;
	case 3:  return INPAD_SHOW_PATCH_WINDOW;
	case 4:  return INPAD_SHOW_PRESET_WINDOW;
	case 5:  return INPAD_MONO_EFFECT;
	case 6:  return INPAD_BYPASS_EFFECT;
	case 7:  return INPAD_SHOW_SHELL_PRESET_WINDOW;
	case 8:  return INPAD_VOICE_COUNT;
	case 9:  return INPAD_MIDI_ACTIVITY;
	case 10: return INPAD_MIDI_CHANNEL;
	case 11: return INPAD_PERFORMANCE_MODE_GLOBAL_DISABLE;
	case 12: return INPAD_CONFIGUID;
	default: return -1;
	}
}

const int ScopeFX::getOutputIndexForValue(int index)
{
	switch (index)
	{
	case 0:  return OUTPAD_PERFORMANCE_MODE;
	case 1:  return OUTPAD_OSCUID;
	case 2:  return -1;
	case 3:  return OUTPAD_SHOW_PATCH_WINDOW;
	case 4:  return OUTPAD_SHOW_PRESET_WINDOW;
	case 5:  return OUTPAD_MONO_EFFECT;
	case 6:  return OUTPAD_BYPASS_EFFECT;
	case 7:  return OUTPAD_SHOW_SHELL_PRESET_WINDOW;
	case 8:  return OUTPAD_VOICE_COUNT;
	case 9:  return -1;
	case 10: return OUTPAD_MIDI_CHANNEL;
	case 11: return -1;
	case 12: return OUTPAD_CONFIGUID;
	default: return -1;
	}
}

#ifdef _WIN32
#include <Windows.h>
#endif
#include <float.h>

#ifdef _WIN32
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)  
HWND scopeWindow;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM /* lParam */)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandle(NULL);

    if((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE) == hinst && IsWindowVisible(hwnd))
    {
        scopeWindow = hwnd;
        return FALSE;
    }
    else
        return TRUE;
}
#endif

using namespace ScopeFXParameterDefinitions;

ScopeFX::ScopeFX() : Effect(&effectDescription)
{
    initValues();
    
    if (ScopeSync::getNumScopeSyncInstances() == 0)
    {
#ifdef _WIN32
        Process::setCurrentModuleInstanceHandle(HINST_THISCOMPONENT);
#endif
        initialiseJuce_GUI();
    }

    scopeSync = new ScopeSync(this);

    DBG("ScopeFX::ScopeFX - Number of module instances: " + String(ScopeSync::getNumScopeSyncInstances()));

    startTimer(timerFrequency);
}

ScopeFX::~ScopeFX()
{
    stopTimer();
    
    scopeFXGUI = nullptr;
    scopeSync->unload();
    scopeSync = nullptr;

    DBG("ScopeFX::~ScopeFX - Number of module instances: " + String(ScopeSync::getNumScopeSyncInstances()));
    
    ScopeSync::shutDownIfLastInstance();
}
   
void ScopeFX::initValues()
{
	positionX          = initPositionX;
	positionY          = initPositionY;
    requestWindowShow  = false;
    windowShown        = false;
    windowHandlerDelay = 0;
    
	for (int i = 0; i < numManagedValues; i++)
	{
		currentValues[i]      = 0;
		newScopeSyncValues[i] = 0;
		newAsyncValues[i]     = 0;
	}
}

void ScopeFX::timerCallback()
{
    if (scopeFXGUI)
        scopeFXGUI->refreshWindow();

    if (windowHandlerDelay == 0)
    {   
		if ((requestWindowShow || scopeSync->getSystemError().toString().isNotEmpty()) && !windowShown)
        {
            DBG("ScopeFX::timerCallback - Request to show window");
            showWindow();
        }
        else if (!requestWindowShow && windowShown)
        {
            DBG("ScopeFX::timerCallback - Request to hide window");
            hideWindow();
        }

        //DBG("ScopeFX::timerCallback: " + String(positionX) + ", " + String(positionY) + ", " + String(scopeFXGUI->getScreenPosition().getX()) + ", " + String(scopeFXGUI->getScreenPosition().getY()));
        if (scopeFXGUI != nullptr
            && (positionX != scopeFXGUI->getScreenPosition().getX()
            ||  positionY != scopeFXGUI->getScreenPosition().getY()))
        {
            scopeFXGUI->setTopLeftPosition(positionX, positionY);
        }
    }
    else
    {
        DBG("ScopeFX::timerCallback - Ignoring values: " + String(windowHandlerDelay));
        windowHandlerDelay--;
    }
    
    if (!(scopeSync->processConfigurationChange()))
    {
        scopeSync->getParameterController()->receiveUpdatesFromScopeAsync();
		manageValuesForScopeSync();
    }
}

void ScopeFX::manageValuesForScopeSync()
{
	if (scopeSync == nullptr)
		return;

	newScopeSyncValues[performanceMode]       = scopeSync->getPerformanceMode();
	newScopeSyncValues[oscUID]                = scopeSync->getOSCUID();
	// newScopeSyncValues[deviceType]		      = scopeSync->getDeviceType();
	newScopeSyncValues[showPatchWindow]       = scopeSync->getShowPatchWindow();
	newScopeSyncValues[showPresetWindow]      = scopeSync->getShowPresetWindow();
	newScopeSyncValues[monoEffect]            = scopeSync->getMonoEffect();
	newScopeSyncValues[bypassEffect]          = scopeSync->getBypassEffect();
	newScopeSyncValues[showShellPresetWindow] = scopeSync->getShowShellPresetWindow();
	newScopeSyncValues[voiceCount]			  = scopeSync->getVoiceCount();
	// newScopeSyncValues[midiActivity]          = scopeSync->getMidiActivity();
	newScopeSyncValues[midiChannel]           = scopeSync->getMidiChannel();
	
	
	handleAsyncValue(&ScopeSync::setPerformanceMode, performanceMode);
	handleAsyncValue(&ScopeSync::setOSCUID, oscUID);
	handleAsyncValue(&ScopeSync::setDeviceType, deviceType);
	handleAsyncValue(&ScopeSync::setShowPatchWindow, showPatchWindow);
	handleAsyncValue(&ScopeSync::setShowPresetWindow, showPresetWindow);
	handleAsyncValue(&ScopeSync::setMonoEffect, monoEffect);
	handleAsyncValue(&ScopeSync::setBypassEffect, bypassEffect);
	handleAsyncValue(&ScopeSync::setShowShellPresetWindow, showShellPresetWindow);
	handleAsyncValue(&ScopeSync::setVoiceCount, voiceCount);
	handleAsyncValue(&ScopeSync::setMidiActivity, midiActivity);
	handleAsyncValue(&ScopeSync::setMidiChannel, midiChannel);
	
	if (newAsyncValues[performanceModeGlobalDisable] != currentValues[performanceModeGlobalDisable])
	{
		currentValues[performanceModeGlobalDisable]      = newAsyncValues[performanceModeGlobalDisable];
		newScopeSyncValues[performanceModeGlobalDisable] = newAsyncValues[performanceModeGlobalDisable];
		
		scopeSync->setPerformanceModeGlobalDisable(currentValues[performanceModeGlobalDisable]);
	}
}

void ScopeFX::handleAsyncValue(void (ScopeSync::*f)(int input), int managedValueId)
{
	if (newAsyncValues[managedValueId] != currentValues[managedValueId])
	{
		currentValues[managedValueId]      = newAsyncValues[managedValueId];
		newScopeSyncValues[managedValueId] = newAsyncValues[managedValueId];
		
		(scopeSync->*f)(currentValues[managedValueId]);
	}
}

void ScopeFX::showWindow()
{
    DBG("ScopeFX::showWindow");
    scopeFXGUI = new ScopeFXGUI(this);

#ifdef _WIN32
    if (scopeWindow == nullptr)
    {
        EnumWindows(EnumWindowsProc, 0);
    }
#else
    // If Scope ever ends up on non-Windows, we'll
    // probably want to implement something here
    void* scopeWindow = nullptr;
#endif
    scopeFXGUI->setOpaque(true);
    scopeFXGUI->setVisible(true);
    scopeFXGUI->setName("ScopeSync");

    scopeFXGUI->setTopLeftPosition(positionX, positionY);
    scopeFXGUI->addToDesktop(ComponentPeer::windowHasTitleBar | ComponentPeer::windowHasCloseButton | ComponentPeer::windowHasDropShadow, scopeWindow);
    windowShown = true;
    windowHandlerDelay = windowHandlerDelayMax;
}

void ScopeFX::positionChanged(int newPosX, int newPosY)
{
    DBG("ScopeFX::positionChanged - moving window to: " + String(newPosX) + "," + String(newPosY));
    positionX = newPosX;
    positionY = newPosY;
    windowHandlerDelay = windowHandlerDelayMax;
}

void ScopeFX::setGUIEnabled(bool shouldBeEnabled)
{
    if (scopeFXGUI != nullptr)
        scopeFXGUI->setEnabled(shouldBeEnabled);
}

void ScopeFX::hideWindow()
{
    scopeFXGUI = nullptr;
    windowShown = false;
    windowHandlerDelay = windowHandlerDelayMax;
}

int ScopeFX::async(PadData** asyncIn,  PadData* /*syncIn*/,
                   PadData*  asyncOut, PadData* /*syncOut*/)
{
	int* parameterArray = (int*)asyncIn[INPAD_PARAMS]->itg;
    int* localArray     = (int*)asyncIn[INPAD_LOCALS]->itg;

	int asyncValues[numParameters + numLocals];

	// Grab ScopeSync values from input
	if (parameterArray != nullptr)
	{
		for (int i = 0; i < numParameters; i++)
		{
			// DBG("ScopeFX::async - input value for param " + String(i) + " is: " + String(parameterArray[i]));
			asyncValues[i] = parameterArray[i];
		}
	}
	else
	{
		for (int i = 0; i < numParameters; i++)
            asyncValues[i] = 0;
	}

	// Grab ScopeLocal values from input
	if (localArray != nullptr)
	{
		for (int i = numParameters; i < numParameters + numLocals; i++)
			asyncValues[i] = localArray[i - numParameters];
	}
	else
	{
        for (int i = numParameters; i < numParameters + numLocals; i++)
            asyncValues[i] = 0;
	}

	// Get ScopeSync to process the inputs and pass on changes from the SS system
	if (scopeSync != nullptr)
		scopeSync->getParameterController()->handleScopeSyncAsyncUpdate(asyncValues);

	// Write to the async outputs for the ScopeSync and ScopeLocal parameters
	for (int i = 0; i < numParameters + numLocals; i++)
	{
		// DBG("ScopeFX::async - output value for param " + String(i) + " is: " + String(asyncValues[i]));
		asyncOut[i].itg = asyncValues[i];
	}

	// Deal with showing/hiding the Control Panel window
    requestWindowShow = (asyncIn[INPAD_SHOW]->itg > 0);
    asyncOut[OUTPAD_SHOW].itg = windowShown ? 1 : 0;
    
	// Handle window position updates
	positionX = asyncIn[INPAD_X]->itg;
    positionY = asyncIn[INPAD_Y]->itg;
    
    asyncOut[OUTPAD_X].itg = (scopeFXGUI != nullptr) ? scopeFXGUI->getScreenPosition().getX() : positionX;
    asyncOut[OUTPAD_Y].itg = (scopeFXGUI != nullptr) ? scopeFXGUI->getScreenPosition().getY() : positionY;
    
	// Handle configuration changes
	newAsyncValues[configurationUID] = asyncIn[INPAD_CONFIGUID]->itg;

	if (scopeSync != nullptr)
	{
		if (newAsyncValues[configurationUID] != currentValues[configurationUID])
		{
			scopeSync->changeConfiguration(newAsyncValues[configurationUID]);
			currentValues[configurationUID] = newAsyncValues[configurationUID];
		}

		newScopeSyncValues[configurationUID] = scopeSync->getConfigurationUID();
	}
	
	asyncOut[OUTPAD_CONFIGUID].itg = newScopeSyncValues[configurationUID];
    
	// Tell Scope when the DLL has been loaded
	asyncOut[OUTPAD_LOADED].itg = (scopeSync != nullptr && scopeSync->isInitialised()) ? FRAC_MAX : 0;
  
	// Handle "managed values"
	for (int i = 0; i < configurationUID; i++)
	{
		// Firstly grab the value coming in from the async input
		newAsyncValues[i] = asyncIn[getInputIndexForValue(i)]->itg;
		
		// Deal with values that can be updated by ScopeSync
		int outIdx = getOutputIndexForValue(i);

		if (outIdx >= 0)
		{
			// If we've had a change from ScopeSync, override
			// using that
			if (newScopeSyncValues[i] != currentValues[i])
			{
				// We have a change from ScopeSync, so let's process it
				newAsyncValues[i] = newScopeSyncValues[i];
				currentValues[i]  = newScopeSyncValues[i];
			}

			asyncOut[outIdx].itg = newAsyncValues[i];
		}
	}

    return 0;
}

int ScopeFX::syncBlock(PadData** /*asyncIn*/, PadData* /*syncIn*/,
                       PadData* /*asyncOut*/, PadData* /*syncOut*/, 
                       int /*off*/,
                       int /*cnt*/)
{
	return -1;
}

Effect *newEffect()
{
	return new ScopeFX();
};

void deleteEffect (Effect *eff)
{
   delete eff;
}

EffectDescription *descEffect ()
{
    return &effectDescription;
}

extern "C"
::uint32 ioctlEffect (
					  ::uint32  /* dwService */, ::uint32  /* dwDDB */, ::uint32  /* hDevice */,
					  void * /* lpvInBuffer */, ::uint32 /* cbInBuffer */, 
					  void * /* lpvOutBuffer */, ::uint32 /* cbOutBuffer */,
					  ::uint32 * lpcbBytesReturned )
{
   *lpcbBytesReturned = 0;
   return ERROR_NOT_SUPPORTED;
}

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
 * the Free Software Foundation, either version 3 of the License, or
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

#ifndef SCOPEFX_H_INCLUDED
#define SCOPEFX_H_INCLUDED

//#include <vld.h>
#include <JuceHeader.h>
#include "../Core/ScopeSync.h"
class ScopeFXGUI;
#include "../../../External/SonicCore/effclass.h"
#include "ScopeFXParameterDefinitions.h"

using namespace ScopeFXParameterDefinitions;

class ScopeFX : public Effect, public Value::Listener, public Timer
{
public:
    ScopeFX();
    ~ScopeFX();

    // Process a set of Sync data coming in from Scope
    // and fill in outgoing streams as appropriate
    int  syncBlock (PadData** asyncIn,  PadData* syncIn,
                    PadData*  asyncOut, PadData* syncOut, 
                    int       off,      int      cnt);

    // Process new Async values coming in from Scope and pass on
    // updates from within ScopeSync
    int  async (PadData** asyncIn,  PadData* syncIn,
                PadData*  asyncOut, PadData* syncOut);
    
    ScopeSync& getScopeSync() { return *scopeSync; };
    void setGUIEnabled(bool shouldBeEnabled);

	void syncScope();
	void snapshot();
	void setPluginHostIP(StringRef address);
	void setPluginHostIP(int oct1, int oct2, int oct3, int oct4);
	void setPluginListenerPort(int port);
	void setScopeSyncListenerPort(int port);

    void valueChanged(Value& valueThatChanged) override;

private:	
	
	// Initialise member variables
    void initValues();
   
    // Show/hides the ScopeFX GUI window
	void toggleWindow(bool show);

	void timerCallback() override;
	
    Value shouldShowWindow;
	Value pluginHost;
	Value pluginPort;
	Value scopeSyncPort;

    ScopedPointer<ScopeSync> scopeSync;	
    ScopedPointer<ScopeFXGUI> scopeFXGUI;

	int deviceInstance;
	std::atomic<int> snapshotValue;
	std::atomic<int> syncScopeValue;
	std::atomic<int> pluginHostOctet1;
	std::atomic<int> pluginHostOctet2;
	std::atomic<int> pluginHostOctet3;
	std::atomic<int> pluginHostOctet4;
	std::atomic<int> pluginListenerPort;
	std::atomic<int> scopeSyncListenerPort;
};


#endif  // SCOPEFX_H_INCLUDED

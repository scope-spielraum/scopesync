/*
  ==============================================================================

    BCMParameterController.cpp
    Created: 29 Nov 2015 8:33:20pm
    Author:  giles

  ==============================================================================
*/

#include "BCMParameterController.h"
#include "ScopeSyncApplication.h"
#include "../Utils/BCMXml.h"
#include "Global.h"

#ifndef __DLL_EFFECT__
    #include "../Plugin/PluginProcessor.h"
#endif // __DLL_EFFECT__

const int BCMParameterController::minHostParameters = 128;
const int BCMParameterController::maxHostParameters = 128;
const int BCMParameterController::timerFrequency    = 100;

BCMParameterController::BCMParameterController(ScopeSync* owner) :
parameterValueStore("parametervalues"), scopeSync(owner)
{
	const StringArray scopeFixedParameters = StringArray::fromTokens("DUMMY,X,Y,Show,Config ID,Show Preset Window,Show Patch Window,Mono Effect,BypassEffect,Show Shell Preset Window,Voice Count,MIDI Channel,Device Type,MIDI Activity,Device Instance",",", "");
    
    for (int i = 1; i < scopeFixedParameters.size(); i++)
        addFixedScopeParameter(scopeFixedParameters[i], i);
    
	scopeSync->initOSCUID();
}

void BCMParameterController::addFixedScopeParameter(const String& scopeCode, const int scopeCodeId)
{
	DBG("BCMParameterController::addFixedScopeParameter - adding scopeCode: " + scopeCode + ", scopeCodeId: " + String(scopeCodeId));

    ValueTree tmpParameter = Configuration::getDefaultFixedParameter();
    tmpParameter.setProperty(Ids::scopeCode, scopeCode, nullptr);
    tmpParameter.setProperty(Ids::name, scopeCode, nullptr);
    tmpParameter.setProperty(Ids::shortDescription, scopeCode, nullptr);
    tmpParameter.setProperty(Ids::fullDescription, scopeCode, nullptr);
    tmpParameter.setProperty(Ids::scopeParamId, scopeCodeId, nullptr);
	tmpParameter.setProperty(Ids::scopeParamGroup, 0, nullptr);

    addParameter(tmpParameter, true, (scopeCode != "Device Instance"));
}

void BCMParameterController::addParameter(ValueTree parameterDefinition, bool fixedParameter, bool oscAble)
{
	ScopeOSCParamID scopeOSCParamID(int(parameterDefinition.getProperty(Ids::scopeParamGroup, -1)), 
									int(parameterDefinition.getProperty(Ids::scopeParamId, -1)));

	String name               = parameterDefinition.getProperty(Ids::name);
	
	BCMParameter* parameter = new BCMParameter(parameterDefinition, *this, oscAble, scopeOSCParamID);

    if (fixedParameter)
        fixedParameters.add(parameter);
    else
        dynamicParameters.add(parameter);

    parameters.add(parameter);
	parametersByName.set(name, parameter);
}

void BCMParameterController::setupHostParameters()
{
	int hostIdx = 0;
    
    for (auto parameter : dynamicParameters)
    {
        parameter->setHostIdx(hostIdx);
        hostParameters.add(parameter);

        hostIdx++;
    }
}

void BCMParameterController::reset()
{
	DBG("BCMParameterController::reset - clearing parameters array");
    parameters.clear();

    for (auto fixedParameter : fixedParameters)
	{
		DBG("BCMParameterController::reset - Added parameter: " + fixedParameter->getName());
		parameters.add(fixedParameter);
	}

    hostParameters.clear();
    dynamicParameters.clear();
}

void BCMParameterController::snapshot() const
{
	for (auto parameter : parameters)
		parameter->sendOSCParameterUpdate();
}

int BCMParameterController::getNumParametersForHost()
{
    return minHostParameters;
}

BCMParameter* BCMParameterController::getParameterByName(StringRef name) const
{
	if (parametersByName.contains(name))
		return parametersByName[name];
	
	return nullptr;
}

float BCMParameterController::getParameterHostValue(int hostIdx) const
{
    if (isPositiveAndBelow(hostIdx, hostParameters.size()))
        return hostParameters[hostIdx]->getHostValue();
    else
        return 0.0f;
}

void BCMParameterController::getParameterNameForHost(int hostIdx, String& parameterName) const
{
    if (isPositiveAndBelow(hostIdx, hostParameters.size()))
    {
        String shortDesc;
        hostParameters[hostIdx]->getDescriptions(shortDesc, parameterName);
    }
    else
    {
        parameterName = "Dummy Param";
    }
}

void BCMParameterController::getParameterText(int hostIdx, String& parameterText) const
{
    if (isPositiveAndBelow(hostIdx, hostParameters.size()))
        hostParameters[hostIdx]->getUITextValue(parameterText);
}

void BCMParameterController::setParameterFromHost(int hostIdx, float newValue) const
{
    if (isPositiveAndBelow(hostIdx, hostParameters.size()))
        hostParameters[hostIdx]->setHostValue(newValue);
}

void BCMParameterController::setParameterFromGUI(BCMParameter& parameter, float newValue)
{
    parameter.setUIValue(newValue);
}

void BCMParameterController::endAllParameterChangeGestures()
{
    for (int i = 0; i < hostParameters.size(); i++)
    {
        BCMParameter* parameter = hostParameters[i];
#ifndef __DLL_EFFECT__
        int hostIdx = parameter->getHostIdx();

        if (changingParams[hostIdx])
        {
            scopeSync->getPluginProcessor()->endParameterChangeGesture(hostIdx); 
            changingParams.clearBit(hostIdx);
        }
#else
        parameter->getScopeOSCParameter().startListening();
#endif // __DLL_EFFECT__
    }
}

void BCMParameterController::updateHost(int hostIdx, float newValue) const
{
#ifndef __DLL_EFFECT__
	scopeSync->getPluginProcessor()->updateListeners(hostIdx, newValue);
#else
	(void)hostIdx;
	(void)newValue;
#endif
}

void BCMParameterController::beginParameterChangeGesture(int hostIdx)
{
#ifndef __DLL_EFFECT__
    if (hostIdx >= 0 && !changingParams[hostIdx])
    {
        scopeSync->getPluginProcessor()->beginParameterChangeGesture(hostIdx);
        changingParams.setBit(hostIdx);
    }
#else
    (void)hostIdx;
#endif
}

void BCMParameterController::endParameterChangeGesture(int hostIdx)
{
#ifndef __DLL_EFFECT__
    if (hostIdx >= 0 && changingParams[hostIdx])
    {
        scopeSync->getPluginProcessor()->endParameterChangeGesture(hostIdx); 
        changingParams.clearBit(hostIdx);
    }
#else
    (void)hostIdx;
#endif
}

void BCMParameterController::storeParameterValues()
{
    int numHostParameters = hostParameters.size();
    
    if (numHostParameters > 0)
    {
        Array<float> currentParameterValues;
    
        for (int i = 0; i < numHostParameters; i++)
            currentParameterValues.set(i, getParameterHostValue(i));

        parameterValueStore = XmlElement("parametervalues");
        parameterValueStore.addTextElement(String(floatArrayToString(currentParameterValues, numHostParameters)));

        //DBG("ScopeSync::storeParameterValues - Storing XML: " + parameterValueStore.createDocument(""));
    }
    else
    {
        //DBG("ScopeSync::storeParameterValues - leaving storage alone, as we don't have any host parameters");
    }
}

void BCMParameterController::storeParameterValues(XmlElement& parameterValues)
{
    parameterValueStore = XmlElement(parameterValues);
    
    //DBG("ScopeSync::storeParameterValues - Storing XML: " + parameterValueStore.createDocument(""));
}

void BCMParameterController::restoreParameterValues() const
{
    Array<float> parameterValues;
    int numHostParameters = getNumParametersForHost();

    String floatCSV = parameterValueStore.getAllSubText();
    int numParametersToRead = jmin(numHostParameters, stringToFloatArray(floatCSV, parameterValues, numHostParameters));

    for (int i = 0; i < numParametersToRead; i++)
    {
        setParameterFromHost(i, parameterValues[i]);
    }

    //DBG("ScopeSync::restoreParameterValues - Restoring XML: " + parameterValueStore.createDocument(""));
} 


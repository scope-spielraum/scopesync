/**
 * The BCModular version of Juce's Slider, which adds the ability
 * to be created from an XML definition, as well as being tied into
 * the ScopeSync parameter system
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

#include "BCMSlider.h"
#include "../Utils/BCMMath.h"
#include "../Components/BCMLookAndFeel.h"
#include "../Components/BCMTabbedComponent.h"
#include "../Core/ScopeSync.h"
#include "../Core/ScopeSyncGUI.h"
#include "../Properties/SliderProperties.h"
#include "../Core/Global.h"
#include "../Windows/UserSettings.h"
#include "../Configuration/ConfigurationManager.h"

BCMSlider::BCMSlider(const String& name, ScopeSyncGUI& owner)
    : Slider(name), BCMParameterWidget(owner)
{
    setParentWidget(this);
    //setWantsKeyboardFocus(true);
}

BCMSlider::~BCMSlider() {}

void BCMSlider::applyProperties(SliderProperties& properties)
{
	mapsToParameter = false;

    overrideSliderStyle(properties.style);
    
	fontHeight         = properties.fontHeight;
    fontStyleFlags     = properties.fontStyleFlags;
    justificationFlags = properties.justificationFlags;
    
    applyWidgetProperties(properties);
    
    if (properties.style == Slider::IncDecButtons)
        overrideIncDecButtonMode(properties.incDecButtonMode);

    overridePopupEnabled(properties.popupEnabled);
    overrideVelocityBasedMode(properties.velocityBasedMode);

    setTextBoxStyle(properties.textBoxPosition,
                    properties.textBoxReadOnly,
                    properties.textBoxWidth,
                    properties.textBoxHeight);
    
    double rangeMin = properties.rangeMin;
    double rangeMax = properties.rangeMax;
    double rangeInt = properties.rangeInt;
    String tooltip  = properties.name;
    
	// Set up a dedicated OSC UID slider
	if (getName().equalsIgnoreCase("oscuid"))
	{
		scopeSync.referToOSCUID(getValueObject());

		setRange(rangeMin, rangeMax, 1);
		setTooltip(tooltip);

		return;
	}

    // Set up any mapping to TabbedComponent Tabs
    mapsToTabs = false;
    
    for (int i = 0; i < properties.tabbedComponents.size(); i++)
    {
        String tabbedComponentName = properties.tabbedComponents[i];
        String tabName = properties.tabNames[i];

        tabbedComponentNames.add(tabbedComponentName);
        tabNames.add(tabName);

        mapsToTabs = true;
            
        // DBG("BCMSlider::applyProperties - mapped Tab: " + tabbedComponentName + ", " + tabName);
    }

    setupMapping(Ids::slider, getName(), properties.mappingParentType, properties.mappingParent);

    if (mapsToParameter)
    {
        String shortDesc;
        parameter->getDescriptions(shortDesc, tooltip);

        String uiSuffix = String::empty;
        parameter->getUIRanges(rangeMin, rangeMax, rangeInt, uiSuffix);

        if (parameter->isDiscrete())
        {
            // We have discrete settings for the mapped parameter, so configure these
            ValueTree parameterSettings;
            parameter->getSettings(parameterSettings);

            settingsNames.clear();

            for (int i = 0; i < parameterSettings.getNumChildren(); i++)
            {
                ValueTree parameterSetting = parameterSettings.getChild(i);
                String    settingName      = parameterSetting.getProperty(Ids::name, "__NO_NAME__");

                settingsNames.add(settingName);
            }
            
            if (getEncoderSnap(properties.encoderSnap))
                rangeInt = 1;
        }

        setRange(rangeMin, rangeMax, rangeInt);
        setTextValueSuffix(uiSuffix);

        setDoubleClickReturnValue(true, parameter->getUIResetValue());
        setSkewFactor(parameter->getUISkewFactor());
        
        // DBG("BCMSlider::applyProperties - " + getName() + " mapping to parameter: " + parameter->getName());
        parameter->mapToUIValue(getValueObject());
    }
    else
    {
        setRange(rangeMin, rangeMax, rangeInt);
    }

    setTooltip(tooltip);
    setPopupMenuEnabled(true);
}

const Identifier BCMSlider::getComponentType() const { return Ids::slider; };

String BCMSlider::getTextFromValue(double v)
{
    if (settingsNames.size() == 0)
    {
		return Slider::getTextFromValue(v);
    }
    else
    {
        return settingsNames[roundDoubleToInt(v)];
    }
}

double BCMSlider::getValueFromText(const String& text)
{
    if (settingsNames.size() == 0)
    {
        return Slider::getValueFromText(text);
    }
    else
    {
        for (int i = 0; i < settingsNames.size(); i++)
        {
            if (settingsNames[i].containsIgnoreCase(text))
            {
                return i;
            }
        }

        return text.getIntValue();
    }
}

void BCMSlider::mouseDown(const MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        showPopupMenu();
    }
    else
    {
        switchToTabs();
        Slider::mouseDown(event);
    }
}

double BCMSlider::valueToProportionOfLength(double value)
{
    const double n = (value - getMinimum()) / (getMaximum() - getMinimum());
    const double skew = getSkewFactor();

    return (skew == 1.0 || n < FLT_EPSILON) ? n : pow (n, skew);
}

void BCMSlider::switchToTabs()
{
    for (int i = 0; i < tabbedComponentNames.size(); i++)
    {
        String tabbedComponentName = tabbedComponentNames[i];
        String tabName             = tabNames[i];

        Array<BCMTabbedComponent*> tabbedComponents;
            
        scopeSyncGUI.getTabbedComponentsByName(tabbedComponentName, tabbedComponents);
        int numTabbedComponents = tabbedComponents.size();

        for (int j = 0; j < numTabbedComponents; j++)
        {
            StringArray tabNames = tabbedComponents[j]->getTabNames();
            int tabIndex = tabNames.indexOf(tabName, true);

            // DBG("BCMSlider::switchToTabs - " + tabbedComponentName + ", " + tabName + ", " + String(tabIndex)); 
            tabbedComponents[j]->setCurrentTabIndex(tabIndex, true);
        }
    }
}

void BCMSlider::overrideSliderStyle(Slider::SliderStyle& style)
{
    RotaryMovement rotaryMovementUserSetting = RotaryMovement(UserSettings::getInstance()->getAppProperties()->getIntValue("rotarymovement", 0));

    if (rotaryMovementUserSetting != noOverride_RM &&
        (  style == Rotary 
        || style == RotaryHorizontalDrag 
        || style == RotaryVerticalDrag 
        || style == RotaryHorizontalVerticalDrag))
    {
             if (rotaryMovementUserSetting == rotary)             style = Rotary;
        else if (rotaryMovementUserSetting == vertical)           style = RotaryVerticalDrag;
        else if (rotaryMovementUserSetting == horizontal)         style = RotaryHorizontalDrag;
        else if (rotaryMovementUserSetting == horizontalVertical) style = RotaryHorizontalVerticalDrag; 
    }
    
    setSliderStyle(style);
}

void BCMSlider::overrideIncDecButtonMode(Slider::IncDecButtonMode& incDecButtonMode)
{
    IDBMode incDecButtonsUserSetting = IDBMode(UserSettings::getInstance()->getAppProperties()->getIntValue("incdecbuttonmode", 0));

    if (incDecButtonsUserSetting != noOverride_IDB)
    {
             if (incDecButtonsUserSetting == idbNotDraggable)  incDecButtonMode = incDecButtonsNotDraggable;
        else if (incDecButtonsUserSetting == idbAutoDirection) incDecButtonMode = incDecButtonsDraggable_AutoDirection;
        else if (incDecButtonsUserSetting == idbHorizontal)    incDecButtonMode = incDecButtonsDraggable_Horizontal;
        else if (incDecButtonsUserSetting == idbVertical)      incDecButtonMode = incDecButtonsDraggable_Vertical;
    }
    
    setIncDecButtonsMode(incDecButtonMode);
}

void BCMSlider::overridePopupEnabled(bool popupEnabledFlag)
{
    PopupEnabled popupEnabledUserSetting = PopupEnabled(UserSettings::getInstance()->getAppProperties()->getIntValue("popupenabled", 0));

    if (popupEnabledUserSetting != noOverride_PE)
        popupEnabledFlag = (popupEnabledUserSetting == popupEnabled);
    
    setPopupDisplayEnabled(popupEnabledFlag, nullptr);
}

void BCMSlider::overrideVelocityBasedMode(bool velocityBasedMode)
{
    VelocityBasedMode velocityBasedModeUserSetting = VelocityBasedMode(UserSettings::getInstance()->getAppProperties()->getIntValue("velocitybasedmode", 0));

    if (velocityBasedModeUserSetting != noOverride_VBM)
        velocityBasedMode = (velocityBasedModeUserSetting == velocityBasedModeOn);
    
    setVelocityBasedMode(velocityBasedMode);
}

bool BCMSlider::getEncoderSnap(bool encoderSnap)
{
    EncoderSnap encoderSnapUserSetting = EncoderSnap(UserSettings::getInstance()->getAppProperties()->getIntValue("encodersnap", 0));
    
    if (encoderSnapUserSetting != noOverride_ES)
        encoderSnap = (encoderSnapUserSetting == snap);
    
    return encoderSnap;
}

bool BCMSlider::isRotary() const
{
    //DBG("BCMSlider::isRotary - slider style: " + String(getSliderStyle()));

    return getSliderStyle() == Rotary
        || getSliderStyle() == RotaryHorizontalDrag
        || getSliderStyle() == RotaryVerticalDrag
        || getSliderStyle() == RotaryHorizontalVerticalDrag;
}

bool BCMSlider::isLinearBar() const
{
    //DBG("BCMSlider::isLinearBar - slider style: " + String(getSliderStyle()));

    return getSliderStyle() == LinearBar
        || getSliderStyle() == LinearBarVertical;
}

void BCMSlider::overrideStyle()
{
    String fillColourString;
    String lineColourString;
    String fillColour2String;
    String lineColour2String;

    if (getSliderStyle() == IncDecButtons)
    {
        fillColourString = findColour(TextButton::buttonColourId).toString();
        lineColourString = findColour(TextButton::textColourOffId).toString();
    }
    else if (isRotary())
    {
        fillColourString = findColour(Slider::rotarySliderFillColourId).toString();
        lineColourString = findColour(Slider::rotarySliderOutlineColourId).toString();
    }
    else if (isLinearBar())
    {
        fillColourString = findColour(Slider::thumbColourId).toString();
        lineColourString = findColour(Slider::backgroundColourId).toString();
    }
    else
    {
        fillColourString = findColour(Slider::thumbColourId).toString();
        lineColourString = findColour(Slider::trackColourId).toString();
    }
    
    fillColour2String = findColour(Slider::textBoxBackgroundColourId).toString();
    lineColour2String = findColour(Slider::textBoxTextColourId).toString();

    ConfigurationManagerCalloutWindow* configurationManagerCalloutWindow = new ConfigurationManagerCalloutWindow(scopeSyncGUI.getScopeSync(), 550, 165);
    configurationManagerCalloutWindow->setStyleOverridePanel(styleOverride, Ids::slider, getName(), widgetTemplateId, fillColourString, lineColourString, fillColour2String, lineColour2String);
    configurationManagerCalloutWindow->addChangeListener(this);
    CallOutBox::launchAsynchronously(configurationManagerCalloutWindow, getScreenBounds(), nullptr);
}

void BCMSlider::applyLookAndFeel(bool noStyleOverride)
{
    BCMWidget::applyLookAndFeel(noStyleOverride);

    bool useColourOverrides = styleOverride.getProperty(Ids::useColourOverrides);

    if (!noStyleOverride && useColourOverrides)
    {
        String fillColourString  = styleOverride.getProperty(Ids::fillColour);
        String lineColourString  = styleOverride.getProperty(Ids::lineColour);
        String fillColour2String = styleOverride.getProperty(Ids::fillColour2);
        String lineColour2String = styleOverride.getProperty(Ids::lineColour2);

        if (fillColourString.isNotEmpty())
        {
            if (getSliderStyle() == IncDecButtons)
                setColour(TextButton::buttonColourId, Colour::fromString(fillColourString));
            else if (isRotary())
                setColour(Slider::rotarySliderFillColourId, Colour::fromString(fillColourString));
            else
                setColour(Slider::thumbColourId, Colour::fromString(fillColourString));
        }    

        if (lineColourString.isNotEmpty())
        {
            if (getSliderStyle() == IncDecButtons)
                setColour(TextButton::textColourOffId, Colour::fromString(lineColourString));
            else if (isRotary())
                setColour(Slider::rotarySliderOutlineColourId, Colour::fromString(lineColourString));
            else
                setColour(Slider::trackColourId, Colour::fromString(lineColourString));
        }

        if (fillColour2String.isNotEmpty())
            setColour(Slider::textBoxBackgroundColourId, Colour::fromString(fillColour2String));

        if (lineColour2String.isNotEmpty())
            setColour(Slider::textBoxTextColourId, Colour::fromString(lineColour2String));
    }
}

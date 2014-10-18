/**
 * Base class for Component classes that can map to a BCMParameter
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

#include "BCMWidget.h"
#include "../Core/Global.h"
#include "../Core/BCMParameter.h"
#include "../Core/ScopeSyncGUI.h"
#include "../Core/ScopeSync.h"
#include "../Configuration/ConfigurationManager.h"
#include "../Properties/WidgetProperties.h"

BCMWidget::BCMWidget(ScopeSyncGUI& owner)
    : scopeSyncGUI(owner), scopeSync(owner.getScopeSync())
{}

void BCMWidget::setParentWidget(Component* parent)
{
    parentWidget = parent;
}

void BCMWidget::applyWidgetProperties(WidgetProperties& properties)
{
    parentWidget->setComponentID(properties.id);

    properties.bounds.copyValues(componentBounds);
    applyBounds();

    bcmLookAndFeelId = properties.bcmLookAndFeelId;
    applyLookAndFeel(properties.noStyleOverride);
}

void BCMWidget::applyBounds()
{
    if (componentBounds.boundsType == BCMComponentBounds::relativeRectangle)
    {
        try
        {
            parentWidget->setBounds(componentBounds.relativeRectangleString);
        }
        catch (Expression::ParseError& error)
        {
            scopeSyncGUI.getScopeSync().setSystemError("Failed to set RelativeRectangle bounds for component", "Component: " + parentWidget->getName() + ", error: " + error.description);
            return;
        }
    }
    else if (componentBounds.boundsType == BCMComponentBounds::inset)
        parentWidget->setBoundsInset(componentBounds.borderSize);
    else
    {
        parentWidget->setBounds(
            componentBounds.x,
            componentBounds.y,
            componentBounds.width,
            componentBounds.height
        );
    }
}

void BCMWidget::applyLookAndFeel(bool noStyleOverride)
{
    if (!noStyleOverride)
    {
        styleOverride = scopeSyncGUI.getScopeSync().getConfiguration().getStyleOverride(getComponentType(), parentWidget->getName());
    
        if (styleOverride.isValid())
        {
            String overrideId = styleOverride.getProperty(Ids::lookAndFeelId);
            
            if (overrideId.isNotEmpty())
                bcmLookAndFeelId = overrideId;
        }
    }
    
    BCMLookAndFeel* bcmLookAndFeel = scopeSyncGUI.getScopeSync().getBCMLookAndFeelById(bcmLookAndFeelId);
    
    if (bcmLookAndFeel != nullptr)
        parentWidget->setLookAndFeel(bcmLookAndFeel);
    else
        bcmLookAndFeelId = String::empty;
}

BCMParameterWidget::BCMParameterWidget(ScopeSyncGUI& owner)
    : BCMWidget(owner),
      commandManager(owner.getScopeSync().getCommandManager())
{
    commandManager->registerAllCommandsForTarget(this);
}

BCMParameterWidget::~BCMParameterWidget()
{
    commandManager->setFirstCommandTarget(nullptr);
}

void BCMParameterWidget::getAllCommands(Array <CommandID>& commands)
{
    if (!scopeSync.configurationIsReadOnly())
    {
        const CommandID ids[] = { CommandIDs::saveConfig,
                                  CommandIDs::saveConfigAs,
                                  CommandIDs::applyConfigChanges,
                                  CommandIDs::discardConfigChanges,
                                  CommandIDs::undo,
                                  CommandIDs::redo,
                                  CommandIDs::deleteItems,
                                  CommandIDs::editItem,
                                  CommandIDs::editMappedItem,
                                  CommandIDs::overrideStyle,
                                  CommandIDs::clearStyleOverride
                                };

        commands.addArray (ids, numElementsInArray (ids));
    }
}

void BCMParameterWidget::getCommandInfo(CommandID commandID, ApplicationCommandInfo& result)
{
    switch (commandID)
    {
    case CommandIDs::undo:
        result.setInfo("Undo", "Undo latest change", CommandCategories::general, !(scopeSyncGUI.getScopeSync().getUndoManager().canUndo()));
        result.defaultKeypresses.add(KeyPress ('z', ModifierKeys::commandModifier, 0));
        break;
    case CommandIDs::redo:
        result.setInfo("Redo", "Redo latest change", CommandCategories::general, !(scopeSyncGUI.getScopeSync().getUndoManager().canRedo()));
        result.defaultKeypresses.add(KeyPress ('y', ModifierKeys::commandModifier, 0));
        break;
    case CommandIDs::saveConfig:
        result.setInfo("Save Configuration", "Save Configuration", CommandCategories::configmgr, !(scopeSyncGUI.getScopeSync().getConfiguration().hasChangedSinceSaved()));
        result.defaultKeypresses.add(KeyPress ('s', ModifierKeys::commandModifier, 0));
        break;
    case CommandIDs::saveConfigAs:
        result.setInfo("Save Configuration As...", "Save Configuration as a new file", CommandCategories::configmgr, 0);
        result.defaultKeypresses.add(KeyPress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        break;
    case CommandIDs::applyConfigChanges:
        result.setInfo("Apply Configuration Changes", "Applies changes made in the Configuration Manager to the relevant ScopeSync instance", CommandCategories::configmgr, 0);
        result.defaultKeypresses.add(KeyPress (KeyPress::returnKey, ModifierKeys::altModifier, 0));
        break;
    case CommandIDs::discardConfigChanges:
        result.setInfo("Discard Configuration Changes", "Discards all unsaved changes to the current Configuration", CommandCategories::configmgr, !(scopeSyncGUI.getScopeSync().getConfiguration().hasChangedSinceSaved()));
        result.defaultKeypresses.add(KeyPress ('d', ModifierKeys::commandModifier, 0));
        break;
    case CommandIDs::deleteItems:
        result.setInfo("Delete", "Delete selected items", CommandCategories::configmgr, mapsToParameter ? 0 : 1);
        result.defaultKeypresses.add (KeyPress (KeyPress::deleteKey, 0, 0));
        result.defaultKeypresses.add (KeyPress (KeyPress::backspaceKey, 0, 0));
        break;
    case CommandIDs::editItem:
        result.setInfo("Edit Item", "Edit the most recently selected item", CommandCategories::general, 0);
        result.defaultKeypresses.add(KeyPress ('e', ModifierKeys::commandModifier, 0));
        break;
    case CommandIDs::editMappedItem:
        result.setInfo("Edit Mapped Item", "Edit item mapped to the most recently selected item", CommandCategories::general, mapsToParameter ? 0 : 1);
        result.defaultKeypresses.add(KeyPress ('e', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        break;
    case CommandIDs::overrideStyle:
        result.setInfo("Override Style", "Override the style of the most recently selected widget", CommandCategories::general, 0);
        result.defaultKeypresses.add(KeyPress ('l', ModifierKeys::commandModifier, 0));
        break;
    case CommandIDs::clearStyleOverride:
        result.setInfo("Clear Style Override", "Clear a style override for the most recently selected widget", CommandCategories::general, styleOverride.isValid() ? 0 : 1);
        result.defaultKeypresses.add(KeyPress ('l', ModifierKeys::commandModifier, 0));
        break;
    }
}

bool BCMParameterWidget::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
        case CommandIDs::undo:                 undo(); break;
        case CommandIDs::redo:                 redo(); break;
        case CommandIDs::saveConfig:           scopeSyncGUI.getScopeSync().saveConfiguration(); break;
        case CommandIDs::saveConfigAs:         saveAs(); break;
        case CommandIDs::applyConfigChanges:   scopeSyncGUI.getScopeSync().applyConfiguration(); break;
        case CommandIDs::discardConfigChanges: scopeSyncGUI.getScopeSync().reloadSavedConfiguration(); break;
        case CommandIDs::deleteItems:          deleteMapping(); break;
        case CommandIDs::editItem:             editMapping(); break;
        case CommandIDs::editMappedItem:       editMappedParameter(); break;
        case CommandIDs::overrideStyle:        overrideStyle(); break;
        case CommandIDs::clearStyleOverride:   clearStyleOverride(); break;
        default:                               return false;
    }

    return true;
}

ApplicationCommandTarget* BCMParameterWidget::getNextCommandTarget()
{
    return &scopeSyncGUI;
}

void BCMParameterWidget::saveAs()
{
    File configurationFileDirectory = scopeSyncGUI.getScopeSync().getConfigurationDirectory();
    
    FileChooser fileChooser("Save Configuration File As...",
                            configurationFileDirectory,
                            "*.configuration");
    
    if (fileChooser.browseForFileToSave(true))
    {
        scopeSyncGUI.getScopeSync().saveConfigurationAs(fileChooser.getResult().getFullPathName());
    }
}

void BCMParameterWidget::undo()
{
    scopeSyncGUI.getScopeSync().getUndoManager().undo();
    scopeSyncGUI.getScopeSync().applyConfiguration();
}

void BCMParameterWidget::redo()
{
    scopeSyncGUI.getScopeSync().getUndoManager().redo();
    scopeSyncGUI.getScopeSync().applyConfiguration();
}

void BCMParameterWidget::showPopupMenu()
{
    commandManager->setFirstCommandTarget(this);

    PopupMenu m;
    m.addSectionHeader(String(getComponentType()) + ": " + parentWidget->getName());
    m.addCommandItem(commandManager, CommandIDs::editItem, "Edit Parameter Mapping");
    m.addCommandItem(commandManager, CommandIDs::editMappedItem, "Edit Mapped Parameter");
    m.addSeparator();
    m.addCommandItem(commandManager, CommandIDs::deleteItems, "Delete Parameter Mapping");
    m.addSeparator();
    m.addCommandItem(commandManager, CommandIDs::overrideStyle, "Override Style");
    m.addCommandItem(commandManager, CommandIDs::clearStyleOverride, "Clear Style Override");
    
    m.showMenuAsync(PopupMenu::Options(), nullptr);  
}

void BCMParameterWidget::setupMapping(const Identifier& componentType,     const String& componentName, 
                                      const Identifier& mappingParentType, const String& mappingParent)
{
    mapsToParameter      = false;
        
    // First try to use a specific mapping set up for this component. Even if it doesn't find a mapped parameter
    // we'll still stick to this mapping, so it can be fixed up later
    mappingComponentType = componentType;
    mappingComponentName = componentName;
    
    parameter = scopeSyncGUI.getUIMapping(Configuration::getMappingParentId(mappingComponentType), mappingComponentName, mapping);

    if (mapping.isValid())
    {
        // DBG("BCMParameterWidget::setupMapping - Using direct mapping: " + String(componentType) + "/" + componentName);
        if (parameter != nullptr)
            mapsToParameter = true;

        return;
    }
    
    if (mappingParentType.isValid() && mappingParent.isNotEmpty())
    {
        // DBG("BCMParameterWidget::setupMapping - Failing back to mappingParent: " + String(mappingComponentType) + "/" + mappingComponentName);
        // Otherwise fail back to a mappingParent (set in the layout XML)
        mappingComponentType = mappingParentType;
        mappingComponentName = mappingParent;

        parameter = scopeSyncGUI.getUIMapping(Configuration::getMappingParentId(mappingComponentType), mappingComponentName, mapping);

        if (parameter != nullptr)
        {
            mapsToParameter = true;
            return;
        }
    }

    // DBG("BCMParameterWidget::setupMapping - No mapping or parent mapping found for component: " + String(componentType) + "/" + componentName);
}

void BCMParameterWidget::deleteMapping()
{
    scopeSyncGUI.getScopeSync().getConfiguration().deleteMapping(mappingComponentType, mapping, nullptr);
    scopeSyncGUI.getScopeSync().applyConfiguration();
}

void BCMParameterWidget::editMapping()
{
    ConfigurationManagerCallout* configurationManagerCallout = new ConfigurationManagerCallout(scopeSyncGUI.getScopeSync(), 400, 34);
    DBG("BCMParameterWidget::editMapping from component: " + parentWidget->getName() + " - " + String(mappingComponentType) + "/" + mappingComponentName);
    configurationManagerCallout->setMappingPanel(mapping, mappingComponentType, mappingComponentName);
    configurationManagerCallout->addChangeListener(this);
    CallOutBox::launchAsynchronously(configurationManagerCallout, parentWidget->getScreenBounds(), nullptr);
}

void BCMParameterWidget::editMappedParameter()
{
    ConfigurationManagerCallout* configurationManagerCallout = new ConfigurationManagerCallout(scopeSyncGUI.getScopeSync(), 550, 700);
    configurationManagerCallout->setParameterPanel(parameter->getDefinition(), parameter->getParameterType());
    configurationManagerCallout->addChangeListener(this);
    CallOutBox::launchAsynchronously(configurationManagerCallout, parentWidget->getScreenBounds(), nullptr);
}

void BCMParameterWidget::overrideStyle()
{
    ConfigurationManagerCallout* configurationManagerCallout = new ConfigurationManagerCallout(scopeSyncGUI.getScopeSync(), 550, 34);
    configurationManagerCallout->setStyleOverridePanel(styleOverride, getComponentType(), parentWidget->getName());
    configurationManagerCallout->addChangeListener(this);
    CallOutBox::launchAsynchronously(configurationManagerCallout, parentWidget->getScreenBounds(), nullptr);
}

void BCMParameterWidget::clearStyleOverride()
{
    scopeSyncGUI.getScopeSync().getConfiguration().deleteStyleOverride(getComponentType(), styleOverride, nullptr);
    scopeSyncGUI.getScopeSync().applyConfiguration();
}

void BCMParameterWidget::changeListenerCallback(ChangeBroadcaster* /* source */)
{
    scopeSyncGUI.getScopeSync().applyConfiguration();
}
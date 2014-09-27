/**
 * Various TreeViewItem classes that support the Configuration
 * Manager TreeView.
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

#ifndef CONFIGURATIONTREEITEM_H_INCLUDED
#define CONFIGURATIONTREEITEM_H_INCLUDED

#include <JuceHeader.h>
#include "../Resources/Icons.h"
class ConfigurationManagerMain;

/* =========================================================================
 * ConfigurationTreeItem: Base class for Configuration Manager TreeViewItems
 */
class ConfigurationTreeItem  : public  TreeViewItem,
                               private ValueTree::Listener
{
public:
    ConfigurationTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);
    ~ConfigurationTreeItem();

    String       getUniqueName() const override;
    virtual bool mightContainSubItems() override;
    Font         getFont() const;
    virtual Icon getIcon() const;
    float        getIconSize() const;
    bool         isIconCrossedOut() const { return false; }
    void         paintContent(Graphics& g, const Rectangle<int>& area);
    void         paintOpenCloseButton(Graphics&, const Rectangle<float>& area, Colour backgroundColour, bool isMouseOver) override;
    Component*   createItemComponent() override;
    void         itemOpennessChanged(bool isNowOpen) override;
    virtual var  getDragSourceDescription() override;
    virtual bool isInterestedInDragSource(const DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    void         itemDropped(const DragAndDropTarget::SourceDetails&, int insertIndex) override;
    void         itemClicked(const MouseEvent& e) override;
    void         itemSelectionChanged(bool isNowSelected) override;
    void         itemDoubleClicked(const MouseEvent&) override;
    virtual void changePanel();
    virtual void deleteItem() {};
    virtual void insertItemBefore() {};
    virtual void insertItemAfter() {};

    virtual void showPopupMenu();
    virtual void showMultiSelectionPopupMenu() {}
    virtual void launchPopupMenu(PopupMenu&);
    virtual void handlePopupMenuResult(int);

    void         storeSelectionMove(int delta);
    void         storeSelectionMove();

    virtual int  getMillisecsAllowedForDragGesture() { return 120; };
    void         cancelDelayedSelectionTimer();

    virtual String getDisplayName() const;

    static void moveItems (TreeView& treeView, const Array<ValueTree>& items,
                           ValueTree newParent, int insertIndex, UndoManager& undoManager);
    static Array<ValueTree> getSelectedTreeViewItems (TreeView& treeView);
    
    struct WholeTreeOpennessRestorer   : public OpennessRestorer
    {
        WholeTreeOpennessRestorer (TreeViewItem& item)  : OpennessRestorer (getTopLevelItem (item))
        {}

    private:
        static TreeViewItem& getTopLevelItem (TreeViewItem& item)
        {
            if (TreeViewItem* const p = item.getParentItem())
                return getTopLevelItem (*p);

            return item;
        }
    };

    int textX;

protected:
    Colour    getBackgroundColour() const;
    Colour    getContrastingColour (float contrast) const;
    Colour    getContrastingColour (Colour targetColour, float minContrast) const;
    ValueTree tree;
    ConfigurationManagerMain& configurationManagerMain;
    UndoManager& undoManager;

    static void treeViewMultiSelectItemChosen(int resultCode, ConfigurationTreeItem* item);
    static void treeViewMenuItemChosen(int resultCode, WeakReference<ConfigurationTreeItem> item);

private:
    class        ItemSelectionTimer;
    friend class ItemSelectionTimer;
    ScopedPointer<Timer> delayedSelectionTimer;

    WeakReference<ConfigurationTreeItem>::Master masterReference;
    friend class WeakReference<ConfigurationTreeItem>;

    virtual void refreshSubItems();
    void valueTreePropertyChanged (ValueTree&, const Identifier&) override;

    void valueTreeChildAdded (ValueTree& parentTree, ValueTree&) override    { treeChildrenChanged(parentTree); }
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree&) override  { treeChildrenChanged(parentTree); }
    void valueTreeChildOrderChanged (ValueTree& parentTree) override         { treeChildrenChanged(parentTree); }
    void valueTreeParentChanged (ValueTree&) override {}

    void treeChildrenChanged (const ValueTree& parentTree);
    void invokeChangePanel();

    void deleteAllSelectedItems();
    void addNewSubItem();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConfigurationTreeItem)
};

/* =========================================================================
 * ParameterTreeItem: Base class for Parameter-based TreeViewItems
 */
class ParameterTreeItem : public ConfigurationTreeItem
{
public:
    ParameterTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    bool mightContainSubItems() override { return false; };
    var  getDragSourceDescription() override;
    bool isInterestedInDragSource (const DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    
    virtual Icon getIcon() const override { return Icon(); };
    virtual String getDisplayName() const override;

    void deleteItem() override;
    void insertItemBefore() override;
    void insertItemAfter() override;
    void insertParameterAt(int index);

private:
    void refreshSubItems() override;
    void showMultiSelectionPopupMenu() override;

    void showPopupMenu() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterTreeItem);
};

/* =========================================================================
 * HostParameterTreeItem: Host Parameter TreeViewItems
 */
class HostParameterTreeItem  : public ParameterTreeItem
{
public:
    HostParameterTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    Icon getIcon() const override;
    String getDisplayName() const override;
    void changePanel() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HostParameterTreeItem);
};

/* =========================================================================
 * ScopeParameterTreeItem: Scope Parameter TreeViewItems
 */
class ScopeParameterTreeItem  : public ParameterTreeItem
{
public:
    ScopeParameterTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    Icon getIcon() const override;
    String getDisplayName() const override;
    void changePanel() override;

private:    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopeParameterTreeItem);
};

/* =========================================================================
 * MappingTreeItem: Base class for Mapping-based TreeViewItems
 */
class MappingTreeItem : public ConfigurationTreeItem
{
public:
    MappingTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    bool mightContainSubItems() override { return false; };
    virtual var getDragSourceDescription() override;
    virtual bool isInterestedInDragSource (const DragAndDropTarget::SourceDetails& dragSourceDetails) override;

    void deleteItem() override;
    void insertItemBefore() override;
    void insertItemAfter() override;
    
    virtual Icon getIcon() const override { return Icon(); };
    virtual String getDisplayName() const override;

private:
    void refreshSubItems() override;
    void showMultiSelectionPopupMenu() override;
    
    void showPopupMenu() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MappingTreeItem);
};

/* =========================================================================
 * SliderMappingTreeItem: TreeViewItem for Slider Mappings
 */
class SliderMappingTreeItem : public MappingTreeItem
{
public:
    SliderMappingTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    var  getDragSourceDescription() override;
    Icon getIcon() const override;
    void changePanel() override;

private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderMappingTreeItem);
};

/* =========================================================================
 * LabelMappingTreeItem: TreeViewItem for Label Mappings
 */
class LabelMappingTreeItem : public MappingTreeItem
{
public:
    LabelMappingTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    var  getDragSourceDescription() override;
    Icon getIcon() const override;
    void changePanel() override;

private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelMappingTreeItem);
};

/* =========================================================================
 * ComboBoxMappingTreeItem: TreeViewItem for ComboBox Mappings
 */
class ComboBoxMappingTreeItem : public MappingTreeItem
{
public:
    ComboBoxMappingTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    var  getDragSourceDescription() override;
    Icon getIcon() const override;
    void changePanel() override;

private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComboBoxMappingTreeItem);
};

/* =========================================================================
 * TabbedComponentMappingTreeItem: TreeViewItem for TabbedComponent Mappings
 */
class TabbedComponentMappingTreeItem : public MappingTreeItem
{
public:
    TabbedComponentMappingTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    var  getDragSourceDescription() override;
    Icon getIcon() const override;
    void changePanel() override;

private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabbedComponentMappingTreeItem);
};

/* =========================================================================
 * TextButtonMappingTreeItem: TreeViewItem for TextButton Mappings
 */
class TextButtonMappingTreeItem : public MappingTreeItem
{
public:
    TextButtonMappingTreeItem(ConfigurationManagerMain& cmm, const ValueTree& v, UndoManager& um);

    var  getDragSourceDescription() override;
    Icon getIcon() const override;
    void changePanel() override;

private:
    String getDisplayName() const override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextButtonMappingTreeItem);
};

/* =========================================================================
 * TreeItemComponent: Component to display as the TreeViewItem
 */
class TreeItemComponent : public Component
{
public:
    TreeItemComponent (ConfigurationTreeItem& i);

    void paint (Graphics& g) override;
    void paintIcon (Graphics& g);
    void resized() override;

    ConfigurationTreeItem& item;

private:    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TreeItemComponent);

};

#endif  // CONFIGURATIONTREEITEM_H_INCLUDED
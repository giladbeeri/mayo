/****************************************************************************
** Copyright (c) 2016, Fougue SAS <https://www.fougue.pro>
** SPDX-License-Identifier: BSD-2-Clause
****************************************************************************/

#include "item_selection.h"

#include "../base/application_item_selection_model.h"
#include "../base/document.h"
#include "../gui/gui_application.h"

#include <vector>

namespace Mayo {

namespace {

bool isStrictAncestor(const Tree<TDF_Label>& tree, TreeNodeId ancestorId, TreeNodeId nodeId)
{
    TreeNodeId parentId = tree.nodeParent(nodeId);
    while (parentId != 0) {
        if (parentId == ancestorId)
            return true;

        parentId = tree.nodeParent(parentId);
    }

    return false;
}

std::vector<ApplicationItem> copyItems(gsl::span<const ApplicationItem> items)
{
    std::vector<ApplicationItem> copied;
    for (const ApplicationItem& item : items)
        copied.push_back(item);

    return copied;
}

} // namespace

ApplicationItem parentOfSelectedItems(gsl::span<const ApplicationItem> selected)
{
    if (selected.empty())
        return {};

    DocumentPtr document;
    std::vector<TreeNodeId> nodeIds;
    for (const ApplicationItem& item : selected) {
        if (!item.isDocumentTreeNode())
            return {};

        const DocumentPtr itemDocument = item.document();
        if (document && itemDocument != document)
            return {};

        document = itemDocument;
        nodeIds.push_back(item.documentTreeNode().id());
    }

    if (!document || nodeIds.empty())
        return {};

    const Tree<TDF_Label>& tree = document->modelTree();
    TreeNodeId candidateId = tree.nodeParent(nodeIds.front());
    while (candidateId != 0) {
        bool containsAll = true;
        for (TreeNodeId nodeId : nodeIds) {
            if (!isStrictAncestor(tree, candidateId, nodeId)) {
                containsAll = false;
                break;
            }
        }

        if (containsAll)
            return ApplicationItem(DocumentTreeNode(document, candidateId));

        candidateId = tree.nodeParent(candidateId);
    }

    return ApplicationItem(document);
}

void replaceApplicationSelection(GuiApplication* guiApp, gsl::span<const ApplicationItem> items)
{
    ApplicationItemSelectionModel* model = guiApp->selectionModel();
    const std::vector<ApplicationItem> replacement = copyItems(items);
    model->clear();
    model->add(replacement);
}

void clearApplicationSelection(GuiApplication* guiApp)
{
    replaceApplicationSelection(guiApp, gsl::span<const ApplicationItem>{});
}

} // namespace Mayo

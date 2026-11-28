/****************************************************************************
** Copyright (c) 2016, Fougue SAS <https://www.fougue.pro>
** SPDX-License-Identifier: BSD-2-Clause
****************************************************************************/

#include "item_selection.h"

#include "../base/application_item_selection_model.h"
#include "../gui/gui_application.h"

#include <vector>

namespace Mayo {

namespace {

std::vector<ApplicationItem> copyItems(gsl::span<const ApplicationItem> items)
{
    std::vector<ApplicationItem> copied;
    for (const ApplicationItem& item : items)
        copied.push_back(item);

    return copied;
}

} // namespace

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

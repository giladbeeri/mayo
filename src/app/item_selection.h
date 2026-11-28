/****************************************************************************
** Copyright (c) 2016, Fougue SAS <https://www.fougue.pro>
** SPDX-License-Identifier: BSD-2-Clause
****************************************************************************/

#pragma once

#include "../base/application_item.h"

#include <gsl/span>

namespace Mayo {

class GuiApplication;

// Nearest strict parent of every selected item.
// When one selected item contains the others, the result is that item's parent.
// Top-level items in the same document share that document as their parent.
ApplicationItem parentOfSelectedItems(gsl::span<const ApplicationItem> selected);

// Replaces the application selection and the matching 3D highlight.
void replaceApplicationSelection(GuiApplication* guiApp, gsl::span<const ApplicationItem> items);
void clearApplicationSelection(GuiApplication* guiApp);

} // namespace Mayo

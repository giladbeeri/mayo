/****************************************************************************
** Copyright (c) 2016, Fougue SAS <https://www.fougue.pro>
** SPDX-License-Identifier: BSD-2-Clause
****************************************************************************/

#pragma once

#include "../base/application_item.h"
#include "../base/document_ptr.h"
#include "../base/signal.h"

#include <QtCore/QObject>
#include <gsl/span>

#include <vector>

namespace Mayo {

class GuiApplication;

// Remembers selection changes so they can be revisited without using visibility undo.
class SelectionHistory : public QObject {
    Q_OBJECT
public:
    SelectionHistory(GuiApplication* guiApp, QObject* parent = nullptr);
    ~SelectionHistory() override;

    bool canGoBack() const;
    bool canGoForward() const;
    void goBack();
    void goForward();

signals:
    void historyChanged();

private:
    void onSelectionChanged(
            gsl::span<const ApplicationItem> selected, gsl::span<const ApplicationItem> deselected
        );
    void commitBurst();
    void applySelection(const std::vector<ApplicationItem>& items);
    void onDocumentClosed(const DocumentPtr& doc);
    std::vector<ApplicationItem> currentSelection() const;
    std::vector<ApplicationItem> liveItems(const std::vector<ApplicationItem>& items) const;

    GuiApplication* m_guiApp = nullptr;
    std::vector<std::vector<ApplicationItem>> m_undo;
    std::vector<std::vector<ApplicationItem>> m_redo;
    std::vector<ApplicationItem> m_beforeBurst;
    SignalConnectionHandle m_connSelectionChanged;
    SignalConnectionHandle m_connDocumentClosed;
    bool m_burstOpen = false;
    bool m_applying = false;
};

} // namespace Mayo

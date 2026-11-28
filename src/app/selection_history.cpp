/****************************************************************************
** Copyright (c) 2016, Fougue SAS <https://www.fougue.pro>
** SPDX-License-Identifier: BSD-2-Clause
****************************************************************************/

#include "selection_history.h"

#include "../base/application.h"
#include "../gui/gui_application.h"
#include "item_selection.h"

#include <QtCore/QTimer>

#include <gsl/util>

#include <algorithm>

namespace Mayo {

namespace {

constexpr size_t maxSelectionHistoryCount = 100;

bool containsItem(gsl::span<const ApplicationItem> items, const ApplicationItem& candidate)
{
    for (const ApplicationItem& item : items) {
        if (item == candidate)
            return true;
    }

    return false;
}

bool containsItem(const std::vector<ApplicationItem>& items, const ApplicationItem& candidate)
{
    return std::find(items.cbegin(), items.cend(), candidate) != items.cend();
}

bool sameSelection(const std::vector<ApplicationItem>& lhs, const std::vector<ApplicationItem>& rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    for (const ApplicationItem& item : lhs) {
        if (!containsItem(rhs, item))
            return false;
    }

    return true;
}

std::vector<ApplicationItem> selectionBeforeChange(
        gsl::span<const ApplicationItem> current,
        gsl::span<const ApplicationItem> selected,
        gsl::span<const ApplicationItem> deselected
    )
{
    std::vector<ApplicationItem> before;
    for (const ApplicationItem& item : current) {
        if (!containsItem(selected, item))
            before.push_back(item);
    }

    for (const ApplicationItem& item : deselected) {
        if (!containsItem(before, item))
            before.push_back(item);
    }

    return before;
}

void eraseDocumentItems(std::vector<ApplicationItem>* items, const DocumentPtr& doc)
{
    items->erase(
        std::remove_if(items->begin(), items->end(), [&](const ApplicationItem& item) {
            return item.document() == doc;
        }),
        items->end()
    );
}

void scrubStack(std::vector<std::vector<ApplicationItem>>* stack, const DocumentPtr& doc)
{
    for (std::vector<ApplicationItem>& items : *stack)
        eraseDocumentItems(&items, doc);

    stack->erase(
        std::unique(stack->begin(), stack->end(), sameSelection),
        stack->end()
    );
}

} // namespace

SelectionHistory::SelectionHistory(GuiApplication* guiApp, QObject* parent)
    : QObject(parent),
      m_guiApp(guiApp)
{
    m_connSelectionChanged = m_guiApp->selectionModel()->signalChanged.connectSlot(
        &SelectionHistory::onSelectionChanged, this
    );
    m_connDocumentClosed = m_guiApp->application()->signalDocumentClosed.connectSlot(
        &SelectionHistory::onDocumentClosed, this
    );
}

SelectionHistory::~SelectionHistory()
{
    m_connSelectionChanged.disconnect();
    m_connDocumentClosed.disconnect();
}

bool SelectionHistory::canGoBack() const
{
    return !m_undo.empty();
}

bool SelectionHistory::canGoForward() const
{
    return !m_redo.empty();
}

void SelectionHistory::goBack()
{
    if (m_burstOpen)
        this->commitBurst();

    if (m_undo.empty())
        return;

    std::vector<ApplicationItem> target = std::move(m_undo.back());
    m_undo.pop_back();
    m_redo.push_back(this->liveItems(this->currentSelection()));
    if (m_redo.size() > maxSelectionHistoryCount)
        m_redo.erase(m_redo.begin());

    this->applySelection(this->liveItems(target));
}

void SelectionHistory::goForward()
{
    if (m_burstOpen)
        this->commitBurst();

    if (m_redo.empty())
        return;

    std::vector<ApplicationItem> target = std::move(m_redo.back());
    m_redo.pop_back();
    m_undo.push_back(this->liveItems(this->currentSelection()));
    if (m_undo.size() > maxSelectionHistoryCount)
        m_undo.erase(m_undo.begin());

    this->applySelection(this->liveItems(target));
}

void SelectionHistory::onSelectionChanged(
        gsl::span<const ApplicationItem> selected, gsl::span<const ApplicationItem> deselected
    )
{
    if (m_applying || m_burstOpen)
        return;

    m_beforeBurst = selectionBeforeChange(this->currentSelection(), selected, deselected);
    m_burstOpen = true;
    QTimer::singleShot(0, this, [this]{ this->commitBurst(); });
}

void SelectionHistory::commitBurst()
{
    if (!m_burstOpen || m_applying)
        return;

    m_burstOpen = false;
    const std::vector<ApplicationItem> before = this->liveItems(m_beforeBurst);
    const std::vector<ApplicationItem> current = this->liveItems(this->currentSelection());
    m_beforeBurst.clear();
    if (sameSelection(before, current))
        return;

    m_redo.clear();
    m_undo.push_back(before);
    if (m_undo.size() > maxSelectionHistoryCount)
        m_undo.erase(m_undo.begin());

    emit this->historyChanged();
}

void SelectionHistory::applySelection(const std::vector<ApplicationItem>& items)
{
    m_applying = true;
    auto guard = gsl::finally([=]{ m_applying = false; });
    replaceApplicationSelection(m_guiApp, items);
    emit this->historyChanged();
}

void SelectionHistory::onDocumentClosed(const DocumentPtr& doc)
{
    scrubStack(&m_undo, doc);
    scrubStack(&m_redo, doc);
    if (m_burstOpen)
        eraseDocumentItems(&m_beforeBurst, doc);

    emit this->historyChanged();
}

std::vector<ApplicationItem> SelectionHistory::currentSelection() const
{
    std::vector<ApplicationItem> items;
    for (const ApplicationItem& item : m_guiApp->selectionModel()->selectedItems())
        items.push_back(item);

    return items;
}

std::vector<ApplicationItem> SelectionHistory::liveItems(const std::vector<ApplicationItem>& items) const
{
    Application* app = m_guiApp->application().get();
    std::vector<ApplicationItem> live;
    for (const ApplicationItem& item : items) {
        const DocumentPtr doc = item.document();
        if (doc && app->findDocumentByIdentifier(doc->identifier()).get() == doc.get())
            live.push_back(item);
    }

    return live;
}

} // namespace Mayo

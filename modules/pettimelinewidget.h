#ifndef PETTIMELINEWIDGET_H
#define PETTIMELINEWIDGET_H

#include <QWidget>
#include <QListView>
#include <QVBoxLayout>
#include <QLabel>
#include "pettimelinemodel.h"
#include "timelinedelegate.h"

class PetTimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit PetTimelineWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        m_model = new PetTimelineModel(this);
        m_delegate = new TimelineDelegate(this);

        // 空状态提示
        m_emptyPlaceholder = new QWidget();
        QVBoxLayout *emptyL = new QVBoxLayout(m_emptyPlaceholder);
        emptyL->setAlignment(Qt::AlignCenter);
        QLabel *emptyIcon = new QLabel();
        emptyIcon->setStyleSheet("font-size: 40px; color: #dcdfe6;");
        QLabel *emptyText = new QLabel("暂无相关寄养记录");
        emptyText->setStyleSheet("color: #909399; font-size: 13px; margin-top: 8px;");
        emptyL->addWidget(emptyIcon);
        emptyL->addWidget(emptyText);

        m_view = new QListView();
        m_view->setModel(m_model);
        m_view->setItemDelegate(m_delegate);
        m_view->setResizeMode(QListView::Adjust);
        m_view->setSelectionMode(QAbstractItemView::NoSelection);
        m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_view->setStyleSheet(
            "QListView { border: none; background: transparent; } "
            "QScrollBar:vertical { background: transparent; width: 6px; margin: 0px; } "
            "QScrollBar::handle:vertical { background: #e0e0e0; min-height: 20px; border-radius: 3px; } "
            "QScrollBar::handle:vertical:hover { background: #d0d0d0; } "
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } "
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        );

        layout->addWidget(m_emptyPlaceholder);
        layout->addWidget(m_view);
        
        m_emptyPlaceholder->hide();
    }

    void setLogs(const QList<PetActivityLog> &logs) {
        m_model->setLogs(logs);
        bool hasLogs = !logs.isEmpty();
        m_view->setVisible(hasLogs);
        m_emptyPlaceholder->setVisible(!hasLogs);
        if (hasLogs) m_view->scrollTo(m_model->index(0), QAbstractItemView::PositionAtTop);
    }

    void scrollToDate(const QString &dateStr) {
        int row = m_model->findFirstIndexForDate(dateStr);
        if (row != -1) {
            m_view->scrollTo(m_model->index(row), QAbstractItemView::PositionAtTop);
            m_model->setHighlight(row + 1);
        }
    }

    PetTimelineModel* model() const { return m_model; }
    TimelineDelegate* delegate() const { return m_delegate; }
    QListView* view() const { return m_view; }

private:
    QListView *m_view;
    PetTimelineModel *m_model;
    TimelineDelegate *m_delegate;
    QWidget *m_emptyPlaceholder;
};

#endif // PETTIMELINEWIDGET_H

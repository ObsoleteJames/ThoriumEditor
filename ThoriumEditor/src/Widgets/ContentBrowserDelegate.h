#pragma once

#include <QStyledItemDelegate>
#include <QEvent>

class FCBItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	FCBItemDelegate(QWidget* parent = nullptr);

	//bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

	void destroyEditor(QWidget* editor, const QModelIndex& index) const override;

	bool eventFilter(QObject* editor, QEvent* event) override;

signals:
	void editorCancelled(const QModelIndex& index) const;
	void editorFinished(const QModelIndex& index) const;

private:
	QEvent::Type lastEventType;
	int lastKey;

};

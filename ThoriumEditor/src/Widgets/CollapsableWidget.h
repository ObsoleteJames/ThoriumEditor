#pragma once

#include <QWidget>
#include <QRect>

class QLabel;
class QPushButton;

class CCollapsableWidget : public QWidget
{
	Q_OBJECT

public:
	enum EHeaderType
	{
		ROOT_HEADER,
		NESTED_HEADER
	};

public:
	CCollapsableWidget(QWidget* parent = nullptr);
	CCollapsableWidget(QWidget* widget, QWidget* parent);
	CCollapsableWidget(const QString& text, QWidget* widget = nullptr, QWidget* parent = nullptr);
	virtual ~CCollapsableWidget();

	void SetWidget(QWidget* w);
	inline QWidget* Widget() const { return widget; }
	
	void SetCollapsed(bool b);
	inline bool IsCollapsed() const { return bCollapsed; }

	QPushButton* GetHeader() const { return header; }
	void SetHeaderType(EHeaderType type);

	void SetText(const QString& text);
	QString Text() const;

	void SetHeaderSize(const QRect& size);

private:
	QPushButton* header;

	QWidget* widget;
	bool bCollapsed = true;

};

#pragma once

#include <QWidget>
#include <QVector2D>

enum EViewportLayout
{
	ViewportLayout_Single, // single viewport
	ViewportLayout_DoubleH, // 2 viewports split horizontally
	ViewportLayout_DoubleV, // 2 viewports split vertically
	ViewportLayout_TripleH, // 3 viewports split horizontally
	ViewportLayout_TripleV, // 3 viewports split vertically
	ViewportLayout_TripleHF, // 3 viewports split horizontally flipped
	ViewportLayout_TripleVF, // 3 viewports split vertically flipped
	ViewportLayout_Quad
};

class CViewportHandle;

class CViewportArea : public QWidget
{
	Q_OBJECT

public:
	CViewportArea(QWidget* parent = nullptr);

	void addWidget(QWidget* widget);
	
	void setViewportLayout(EViewportLayout l);
	inline EViewportLayout viewportLayout() const { return vLayout; }

protected:
	bool event(QEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;

	void updateLayout();
	void updateVisible();

	EViewportLayout getLayoutFromWidgets();

private:
	QList<QWidget*> widgets;
	CViewportHandle* splitV;
	CViewportHandle* splitH;

	// 0 - 1
	//float sizes[2];
	QVector2D sizes;

	EViewportLayout vLayout = ViewportLayout_Single;
};

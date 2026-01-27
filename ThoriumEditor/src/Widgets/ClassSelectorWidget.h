#pragma once

#include <QWidget>
#include "Object/Class.h"

class CClassSelectorWidget : public QWidget
{
	Q_OBJECT

	friend class P_ClassSelectorDrowdown;

public:
	CClassSelectorWidget(QWidget* parent = nullptr);
	CClassSelectorWidget(FClass* filter, QWidget* parent = nullptr);
	
	inline FClass* GetClass() const { return clas; }
	void SetClass(FClass* clas);

	inline FClass* GetClassFilter() const { return filterClass; }
	void SetClassFilter(FClass* clas);

Q_SIGNALS:
	void ClassChanged();

protected:
	void paintEvent(QPaintEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

	void Init();

protected:
	FClass* clas;
	FClass* filterClass;
};

#pragma once

#include "EngineCore.h"
#include <functional>
#include "FramelessDialog.h"

class FClass;
class QTreeWidget;
class QPushButton;

class CClassSelectorDialog : public CFramelessDialog
{
	Q_OBJECT

public:
	CClassSelectorDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	virtual ~CClassSelectorDialog();

	inline void SetFilterClass(FClass* c) { filterClass = c; }
	inline FClass* GetSelectedClass() const { return selectedClass; }

	int exec() override;
	
private:
	void AddClassItem(FClass* c, void* parent);

private:
	FClass* selectedClass;
	FClass* filterClass;

	QTreeWidget* classTree;
	QPushButton* selectButton;
	QPushButton* cancelButton;
};

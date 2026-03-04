#pragma once

#include <Util/Core.h>
#include <Util/Pointer.h>
#include "Widgets/PropertyEditor.h"

struct FProperty;
struct FEnum;
class QComboBox;
class QCheckBox;
class IPropertyHandler;
class QPushButton;

class CEnumProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CEnumProperty(void* value, const FProperty* property, QWidget* parent = nullptr);
	CEnumProperty(const FString& name, void* value, FEnum* type, QWidget* parent = nullptr);
	CEnumProperty(const FString& name, int* value, const TArray<TPair<FString, int64>>& options, QWidget* parent = nullptr);

	void Update();

	//void SetValue(int* value, const TArray<TPair<FString, int64>>& options) { this->options = options; byteSize = 4; this->value = value; ResetOptions(); Update(); }

private:
	void InitEnum(const FString& name);
	void InitFlags(const FString& name);

private Q_SLOTS:
	void valueChanged(int);

	void ResetOptions();

private:
	TArray<TPair<FString, int64>> options;
	QComboBox* editor;
	uint8 byteSize;
	void* value = nullptr;
	FString undoName;
	
	bool bIsFlag = false;
	//QWidget* flagContent = nullptr;
	TArray<QCheckBox*> flagEditors;

	TUniquePtr<IPropertyHandler> handler;
	QPushButton* revertBtn = nullptr;
};

#pragma once

#include "ToolsWindow.h"
#include "Assets/Material.h"
#include "Game/World.h"
#include "Game/Components/ModelComponent.h"

class CViewportWidget;
class QUndoStack;
class QTableView;
class QStandardItemModel;
class QVBoxLayout;

class CMaterialEditor : public CToolsWindow
{
	Q_OBJECT
	ToolsWindowBody(CMaterialEditor, "Material Editor", true)

public:
	CMaterialEditor() = default;
	virtual ~CMaterialEditor();

	bool Shutdown() override;
	void SetupUi() override;

public:
	void SetMaterial(CMaterial* mat);
	void SetMaterial(const FString& path);
	inline CMaterial* GetMaterial() const { return material; }

	void NewMaterial();

private:
	void OpenMaterial();

	void UpdateProperties();

	void Init();

	// Rendering
	void DoRender();
	void SwapBuffers();

private:
	TObjectPtr<CWorld> world;
	TObjectPtr<CMaterial> material;
	TObjectPtr<CModelComponent> modelComp;

	CCameraProxy* cam;
	CViewportWidget* viewport;

	QUndoStack* undoStack = nullptr;

	QMenu* menuFile;
	QMenu* menuEdit;

	QWidget* propertiesWidget;
	QVBoxLayout* propertiesLayout;

	QWidget* settingsWidget;
	QVBoxLayout* settingsLayout;

	ads::CDockWidget* propertiesDock = nullptr;
	ads::CDockWidget* settingsDock = nullptr;

	TArray<QWidget*> curProperties;
};

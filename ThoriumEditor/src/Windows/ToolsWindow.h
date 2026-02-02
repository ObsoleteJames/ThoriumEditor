#pragma once

#include "Editor.h"
#include "ToolsClass.h"
#include <Util/Guid.h>
#include <QMainWindow>
#include <QDockWidget>
#include <QMenu>
#include <QSettings>

#include <Util/Map.h>
#include <Registry/RegistryBase.h>

namespace ads
{
	class CDockManager;
	class CDockWidget;
}

CREATE_OBJECT_REGISTRY_DLL(ToolsRegisteredWindows, FToolsWindowClass*, EDITOR_API);

#define SDK_REGISTER_WINDOW(clas, _Name, _ToolBarPath, _Icon) \
struct WindowClass_Decl_##clas : public FToolsWindowClass \
{ \
	WindowClass_Decl_##clas() { \
		Name = _Name; \
		ToolBarPath = _ToolBarPath; \
		icon = _Icon; \
		Id = FString(#clas).Hash(); \
		ToolsRegisteredWindows::AddToRegistry(this); \
	} \
	~WindowClass_Decl_##clas() { ToolsRegisteredWindows::Remove(this); } \
	CToolsWindow* Create() override { return CToolsWindow::Create<clas>(); } \
}; \
extern WindowClass_Decl_##clas _Discardable_WindowsClass_Inst_##clas; \
FToolsWindowClass* clas::Class() { return &_Discardable_WindowsClass_Inst_##clas; } \
WindowClass_Decl_##clas _Discardable_WindowsClass_Inst_##clas = WindowClass_Decl_##clas ()

#define _ToolsBaseBody(Class, Name) \
	static const char* GetName() { return Name; } \
	static SizeType GetId() { return FString(#Class).Hash(); }

#define ToolsWindowBody(_class, Name, AllowMultipleInstances) \
public: \
	friend class CToolsWindow; \
	_ToolsBaseBody(_class, Name) \
	static FToolsWindowClass* Class(); \
	static _class* GetInstance() { return (_class*)CToolsWindow::GetByName(GetName()); } \
	static bool TW_AllowMultiple() { return AllowMultipleInstances; }

class QLabel;
class QPushButton;
class QMenuBar;

class EDITOR_API CToolsWindow : public QMainWindow
{
	Q_OBJECT
	Q_DISABLE_COPY(CToolsWindow)

public:
	CToolsWindow(QWidget* parent = nullptr);
	virtual ~CToolsWindow();

	template<class T>
	static T* Create();

	template<class T>
	static T* Get();

	static FToolsWindowClass* GetClassById(SizeType _id);
	static CToolsWindow* CreateById(SizeType _id);
	static CToolsWindow* GetByName(const char* name);
	static bool CloseAll(CToolsWindow* ignore = nullptr);

	inline ads::CDockManager* getDockManager() const { return dockmanager; }

	//static void ReloadStyle();
	//static FString& GetStyleSheet();

	inline SizeType GetId() const { return IdHash; }

	bool CloseWindow();

protected:
	//void SetWindowTitle(const QString& title);

	virtual bool Shutdown();
	virtual void UpdateStyle() {}
	virtual void SetupUi();

	void SaveState();
	void RestoreState();

	virtual void UserSaveState(QSettings& out) {}
	virtual void UserRestoreState(QSettings& in) {}

	void closeEvent(QCloseEvent* event) override;

private:
	static TMap<SizeType, CToolsWindow*>& GetAll();

protected:
	QMenuBar* menuBar;
	ads::CDockManager* dockmanager = nullptr;

private:
	FGuid ID;
	SizeType IdHash;

public:
	const char* Name;

};

template<class T>
T* CToolsWindow::Create()
{
	static_assert(std::is_base_of<CToolsWindow, T>::value, "Template must derive from 'CToolsWindow'!");

	const char* name = T::GetName();
	if (!T::TW_AllowMultiple())
	{
		if (auto* wnd = GetByName(name))
		{
			wnd->show();
			wnd->setFocus();
			return (T*)wnd;
		}
	}

	T* wnd = new T();
	wnd->Name = name;
	wnd->IdHash = T::GetId();
	wnd->setWindowTitle(T::GetName());
	if (FToolsWindowClass* c = GetClassById(T::GetId()))
		if (c->icon)
			wnd->setWindowIcon(*c->icon);

	wnd->SetupUi();
	wnd->show();
	return wnd;
}

template<class T>
T* CToolsWindow::Get()
{
	static_assert(std::is_base_of<CToolsWindow, T>::value, "Template must derive from 'FToolsWindow'!");

	// TODO: Get by UID;
	SizeType id = T::GetId();
	for (auto it : GetAll())
		if (it.second->IdHash == id)
			return (T*)it.second;

	return nullptr;
}

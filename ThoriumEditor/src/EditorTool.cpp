
#include "EditorTool.h"

#include <QAction>

//TArray<IEditorTool*>& IEditorTool::_Tools()
//{
//    static TArray<IEditorTool*> tools;
//    return tools;
//}
//
//void IEditorTool::InitAll()
//{
//    for (auto t : _Tools())
//        t->Init();
//}
//
//void IEditorTool::ShutdownAll()
//{
//    for (auto t : _Tools())
//        t->Shutdown();
//}
//
//IEditorTool* IEditorTool::GetTool(const QString& name)
//{
//    for (auto t : GetTools())
//        if (t->objectName() == name)
//            return t;
//
//    return nullptr;
//}
//
//void IEditorTool::RegisterTool(IEditorTool* tool)
//{
//    _Tools().Add(tool);
//}
//
//void IEditorTool::UnregisterTool(IEditorTool* tool)
//{
//    _Tools().Erase(_Tools().Find(tool));
//}

#pragma once

#include "layer.h"
#include <iostream>

struct FEditorLog
{
public:
    FEditorLog(const FString& name);
    ~FEditorLog();

    inline const FString& Name() const { return name; }
    inline void Log(const FString& txt) { log += txt; }
    inline const FString& Log() const { return log; }

    inline void Clear() { log.Clear(); }

private:
    FString name;
    FString log;
};

class CEditorLogWnd : public CLayer
{
public:
    void OnUIRender() override;

    void OpenLog(const FString& name);
    void OpenLog(int index);

private:
    int curIndex = 0;
    float sizeL = 200;

};

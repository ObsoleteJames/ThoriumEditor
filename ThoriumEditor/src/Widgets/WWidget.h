#pragma once

#include "EditorCore.h"
#include "Object/Object.h"
#include "Math/Vectors.h"
#include "Util/KeyValue.h"

#include "WWidget.generated.h"

class BLContext;

STRUCT()
struct SDK_API FRect
{
	GENERATED_BODY()

public:
	FRect() = default;
	FRect(const FVector2& pos, const FVector2& size);
	FRect(const FRect& rect);

	inline FVector2 Min() const { return position; }
	inline FVector2 Max() const { return position + size; }

public:
	FVector2 position;
	FVector2 size;
};

ENUM()
enum class EWidgetLayout
{
	FREE,
	VERTICAL,
	HORIZONTAL,
	VERTICAL_STRETCH,
	HORIZONTAL_STRETCH,
};

CLASS()
class SDK_API WWidget : public CObject
{
	GENERATED_BODY()

public:
	WWidget() = default;

	void AddWidget(WWidget* widget);
	void RemoveWidget(WWidget* widget);
	void ClearChildren();

	inline const TArray<TObjectPtr<WWidget>>& ChildWidgets() const { return childWidgets; }

	inline WWidget* GetParent() const { return parent; }

public:
	virtual void SaveSettings(KVCategory* kv);
	virtual void LoadSettings(KVCategory* kv);
	
	virtual void Render(BLContext* ctx);
	virtual void UpdateLayout();

protected:
	void OnDelete() override;
	
	// call this whenever the widget has changed in any way.
	void Update();

public:
	FRect rect;
	float paddingLeft = 4;
	float paddingRight = 4;
	float paddingTop = 4;
	float paddingBottom = 4;
	float itemPadding = 6;

	EWidgetLayout layout;

private:
	TArray<TObjectPtr<WWidget>> childWidgets;
	TObjectPtr<WWidget> parent;
};

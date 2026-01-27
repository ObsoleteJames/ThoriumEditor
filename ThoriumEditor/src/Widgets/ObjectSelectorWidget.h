#pragma once

#include <QWidget>

#include "Object/Object.h"

class CObjectSelectorWidget : public QWidget
{
	Q_OBJECT

	friend class CObjectSelectDialog;
		
public:
	CObjectSelectorWidget(QWidget* parent = nullptr);
	CObjectSelectorWidget(FClass* filter, QWidget* parent = nullptr);
	CObjectSelectorWidget(CObject* obj, FClass* filter, QWidget* parent = nullptr);

	//inline const TObjectPtr<CObject>& GetObject() const { return obj; }
	inline SizeType GetObjectId() const { return objId; }
	void SetObject(CObject* obj);
	void SetObject(SizeType obj);

	inline FClass* GetClassFilter() const { return filterType; }
	void SetClassFilter(FClass*);

	void SetValidator(const std::function<bool(CObject*)>& func) { validator = func; }
	void SetAssetValidator(const std::function<bool(const FString&)>& func) { assetValidator = func; }

	static bool DefaultValidator(CObject* obj) { return true; }

Q_SIGNALS:
	void ObjectChanged();

protected:
	void paintEvent(QPaintEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

	void Init();

public:
	bool bAllowNull = true;

private:
	std::function<bool(CObject*)> validator;
	std::function<bool(const FString&)> assetValidator;
	FClass* filterType;
	//TObjectPtr<CObject> obj;
	QString text;
	SizeType objId;

	bool bTypeIsAsset;

};

#pragma once

#include <QTreeWidgetItem>
#include <QListWidget>

template<typename T>
class TTreeDataItem : public QTreeWidgetItem
{
public:
	TTreeDataItem(int type = 0) : QTreeWidgetItem(type) {}
	TTreeDataItem(const QStringList& strings, int type = 0) : QTreeWidgetItem(strings, type) {}
	TTreeDataItem(QTreeWidget* tree, int type = 0) : QTreeWidgetItem(tree, type) {}
	TTreeDataItem(T in, QTreeWidgetItem* parent, int type = 0) : QTreeWidgetItem(parent, type), data(in) {}
	TTreeDataItem(T in, QTreeWidget* tree, int type = 0) : QTreeWidgetItem(tree, type), data(in) {}
	TTreeDataItem(T in, int type = 0) : QTreeWidgetItem(type), data(in) {}

	inline const T& GetData() const { return data; }
	inline void SetData(const T& newData) { data = newData; }

private:
	T data;
};

template<typename T>
class TListDataItem : public QListWidgetItem
{
public:
	TListDataItem(int type = 0) : QListWidgetItem(type) {}
	TListDataItem(QListWidget* tree, int type = 0) : QListWidgetItem(tree, type) {}
	TListDataItem(T in, QListWidgetItem* parent, int type = 0) : QListWidgetItem(parent, type), data(in) {}
	TListDataItem(T in, QListWidget* tree, int type = 0) : QListWidgetItem(tree, type), data(in) {}
	TListDataItem(T in, int type = 0) : QListWidgetItem(type), data(in) {}

	inline const T& GetData() const { return data; }
	inline void SetData(const T& newData) { data = newData; }

private:
	T data;
};

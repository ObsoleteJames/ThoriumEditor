
#include "ViewportArea.h"
#include <QEvent>
#include <QSplitter>

CViewportArea::CViewportArea(QWidget* parent) : QWidget(parent)
{
	sizes = { 0.5f, 0.5f };

}

void CViewportArea::addWidget(QWidget* widget)
{
	if (widgets.size() == 4)
	{
		qDebug() << "CViewportArea::addWidget() attempted to add more than 4 widgets to area";
		return;
	}

	if (widget->parentWidget() != this)
		widget->setParent(this);

	widgets.push_back(widget);
	updateLayout();
	updateVisible();
}

void CViewportArea::setViewportLayout(EViewportLayout l)
{
	vLayout = l;
	updateLayout();
	updateVisible();
}

bool CViewportArea::event(QEvent* e)
{
	switch (e->type())
	{
	case QEvent::LayoutRequest:
		updateLayout();
	case QEvent::Show:
	case QEvent::Hide:
	case QEvent::HideToParent:
	case QEvent::ShowToParent:
		updateVisible();
		break;
	}

	return QWidget::event(e);
}

void CViewportArea::resizeEvent(QResizeEvent* e)
{
	updateLayout();
	QWidget::resizeEvent(e);
}

void CViewportArea::updateLayout()
{
	EViewportLayout _layout = getLayoutFromWidgets();
	
	QRect r = contentsRect();
	//r.setTopLeft(mapToGlobal(r.topLeft()));

	bool bVertical = (_layout == ViewportLayout_DoubleV || _layout == ViewportLayout_TripleV);

	if (_layout == ViewportLayout_Single)
	{
		widgets[0]->setGeometry(r);
	}
	else if (_layout == ViewportLayout_DoubleH || _layout == ViewportLayout_DoubleV)
	{
		QRect r1 = r;
		QRect r2 = r;

		bool bVertical = _layout == ViewportLayout_DoubleV;

		if (bVertical)
		{
			r1.setWidth(r.width() * sizes[0]);
			r2.setX(r1.width());
			r2.setWidth(r.width() * (1.f - sizes[0]));
		}
		else
		{
			r1.setHeight(r.height() * sizes[1]);
			r2.setY(r1.height());
			r2.setHeight(r.height() * (1.f - sizes[1]));
		}

		widgets[0]->setGeometry(r1);
		widgets[1]->setGeometry(r2);
	}
	else if (_layout == ViewportLayout_TripleH || _layout == ViewportLayout_TripleV || _layout == ViewportLayout_TripleHF || _layout == ViewportLayout_TripleVF)
	{
		QRect r1 = r;
		QRect r2 = r;
		QRect r3 = r;

		bool bVertical = _layout == ViewportLayout_TripleV || _layout == ViewportLayout_TripleVF;
		bool bFlipped = _layout > ViewportLayout_TripleV;

		if (bVertical)
		{
			if (bFlipped)
			{
				r1.setWidth(r.width() * sizes[0]);

				r2.setX(r.width() * sizes[0]);
				r2.setWidth(r.width() * (1.f - sizes[0]));
				r2.setHeight(r.height() * sizes[1]);
				r3.setX(r2.x());
				r3.setY(r2.height());
				r3.setWidth(r2.width());
				r3.setHeight(r.height() * (1.f - sizes[1]));
			}
			else
			{
				r1.setWidth(r.width() * sizes[0]);
				r1.setHeight(r.height() * sizes[1]);
				r2.setY(r1.height());
				r2.setWidth(r1.width());
				r2.setHeight(r.height() * (1.f - sizes[1]));

				r3.setX(r1.width());
				r3.setWidth(r.width() * (1.f - sizes[0]));
			}
		}
		else
		{
			if (bFlipped)
			{
				r1.setHeight(r.height() * sizes[1]);

				r2.setY(r.width() * sizes[1]);
				r2.setWidth(r.width() * sizes[0]);
				r2.setHeight(r.height() * (1.f - sizes[1]));
				r3.setY(r2.y());
				r3.setX(r2.width());
				r3.setWidth(r.width() * (1.f - sizes[0]));
				r3.setHeight(r2.height());
			}
			else
			{
				r1.setWidth(r.width() * sizes[0]);
				r1.setHeight(r.height() * sizes[1]);
				r2.setX(r1.width());
				r2.setWidth(r.width() * (1.f - sizes[0]));
				r2.setHeight(r1.height());

				r3.setY(r1.height());
				r3.setHeight(r.height() * (1.f - sizes[1]));
			}
		}

		widgets[0]->setGeometry(r1);
		widgets[1]->setGeometry(r2);
		widgets[2]->setGeometry(r3);
	}
	else if (_layout == ViewportLayout_Quad)
	{
		QRect r1 = r;
		QRect r2 = r;
		QRect r3 = r;
		QRect r4 = r;

		r1.setWidth(r.width() * sizes[0]);
		r1.setHeight(r.height() * sizes[1]);
		r2.setX(r1.width());
		r2.setWidth(r.width() * (1.f - sizes[0]));
		r2.setHeight(r.height() * sizes[1]);

		r3.setY(r1.height());
		r3.setWidth(r.width() * sizes[0]);
		r3.setHeight(r.height() * (1.f - sizes[1]));
		r4.setX(r1.width());
		r4.setY(r1.height());
		r4.setWidth(r.width() * (1.f - sizes[0]));
		r4.setHeight(r.height() * (1.f - sizes[1]));

		widgets[0]->setGeometry(r1);
		widgets[1]->setGeometry(r2);
		widgets[2]->setGeometry(r3);
		widgets[3]->setGeometry(r4);
	}
}

void CViewportArea::updateVisible()
{
	if (!isVisible())
	{
		for (auto* w : widgets)
			w->setHidden(true);
		return;
	}

	EViewportLayout _layout = getLayoutFromWidgets();

	// LUT
	const qsizetype ammount[] = {
		1,
		2,
		2,
		3,
		3,
		3,
		3,
		4
	};

	int numVisible = (int)std::min(ammount[(int)_layout], widgets.size());
	for (int i = 0; i < widgets.size(); i++)
	{
		widgets[i]->setHidden(i >= numVisible);
	}
}

// get the layout that is usable for the current widgets.
EViewportLayout CViewportArea::getLayoutFromWidgets()
{
	if (widgets.size() < 2)
		return ViewportLayout_Single;

	if (vLayout == ViewportLayout_TripleH || vLayout == ViewportLayout_TripleV || vLayout == ViewportLayout_TripleHF || vLayout == ViewportLayout_TripleVF)
	{
		if (widgets.size() < 3)
		{
			bool bFlipped = vLayout > ViewportLayout_TripleV;
			if (bFlipped)
				return (EViewportLayout)((int)vLayout - 4);
			return (EViewportLayout)((int)vLayout - 2);
		}
	}

	if (vLayout == ViewportLayout_Quad && widgets.size() < 4)
	{
		if (widgets.size() == 2)
			return ViewportLayout_DoubleH;
		
		return ViewportLayout_TripleH;
	}
	return vLayout;
}


#include "DragBox.h"
#include <QCursor>
#include <cmath>

CDragBox::CDragBox(QWidget* parent) : QDoubleSpinBox(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setKeyboardTracking(false);
}

void CDragBox::setPixelsPerStep(double pixelsPerStep)
{
    if (pixelsPerStep > 0.0)
        pixelsPerStep = pixelsPerStep;
}

double CDragBox::pixelsPerStep() const
{
    return pixelStep;
}

void CDragBox::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) 
    {
        bPressed = true;
        pressPos = event->globalPos();
        startValue = value();
        // Do not enter dragging immediately — wait until movement exceeds startDragDistance.
        //event->accept();
        //return;
    }

    QDoubleSpinBox::mousePressEvent(event);
}

void CDragBox::mouseMoveEvent(QMouseEvent* event)
{
    if (bPressed) 
    {
        QPoint globalPos = event->globalPos();
        QPoint diff = globalPos - pressPos;
        int dx = diff.x();

        if (!bDragging)
        {
            // Start dragging only after the mouse moved enough to avoid interfering with clicks.
            if (std::abs(dx) >= QApplication::startDragDistance()) 
            {
                bDragging = true;
                // Change cursor to indicate horizontal drag.
                oldCursor = cursor().shape();
                setCursor(Qt::SizeHorCursor);
            } 
            else 
            {
                // not yet dragging, don't change value
                return;
            }
        }

        // Calculate value change
        double pixelsPerStep = (pixelStep > 0.0 ? pixelStep : 5.0);
        double mod = modifierForKeyboardModifiers(event->modifiers());
        double steps = double(dx) / pixelsPerStep * mod;
        double newValue = startValue + steps * singleStep();

        setValue(newValue);

        event->accept();
        return;
    }

    QDoubleSpinBox::mouseMoveEvent(event);
}

void CDragBox::mouseReleaseEvent(QMouseEvent* event)
{
    if (bPressed && event->button() == Qt::LeftButton) 
    {
        bPressed = false;
        if (bDragging) 
        {
            // restore cursor
            setCursor(oldCursor);

            bDragging = false;
			event->accept();
            return;
        }
        //event->accept();
        //return;
    }

    QDoubleSpinBox::mouseReleaseEvent(event);
}

double CDragBox::modifierForKeyboardModifiers(Qt::KeyboardModifiers mods) const noexcept
{
    // Default multiplier 1.0
    double m = 1.0;
    if (mods & Qt::ShiftModifier) 
    {
        m *= 10.0; // coarse — bigger jumps
    }
    if (mods & Qt::ControlModifier) 
    {
        m *= 0.1;  // fine — smaller jumps
    }
    return m;
}
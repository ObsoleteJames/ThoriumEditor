#pragma once

#include "Editor.h"
#include <QDoubleSpinBox>
#include <QMouseEvent>
#include <QApplication>

/*
 * DragBox
 * A QDoubleSpinBox that supports horizontal dragging to change the value.
 * Drag horizontally: right to increase, left to decrease.
 * Hold Shift for coarse changes, Ctrl for fine changes.
 */
class CDragBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    explicit CDragBox(QWidget* parent = nullptr);

    // Configure sensitivity: how many pixels correspond to one singleStep change.
    // Lower value = more sensitive (fewer pixels per step).
    void setPixelsPerStep(double pixelsPerStep);
    double pixelsPerStep() const;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool bPressed = false;
    bool bDragging = false;
    QPoint pressPos;
    double startValue = 0.0;
    double pixelStep = 5.0; // default: one singleStep per 5 pixels
    Qt::CursorShape oldCursor = Qt::ArrowCursor;

    double modifierForKeyboardModifiers(Qt::KeyboardModifiers mods) const noexcept;
};


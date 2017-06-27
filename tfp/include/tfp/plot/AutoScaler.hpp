#pragma once
#include "tfp/plot/DeltaPlotPicker.hpp"


class QWidget;

namespace tfp {

/*!
 * Provides a method for auto-scaling the data in a plot when double-clicking.
 */
class AutoScaler : public DeltaPlotPicker
{
    Q_OBJECT

public:
    explicit AutoScaler(QWidget* canvas);
    ~AutoScaler();

    void setMouseButton(QFlags<Qt::MouseButton> btn)
            { btn_ = btn;}

    void autoScale();

protected:
    virtual void widgetMouseDoubleClickEvent(QMouseEvent *me) override;

private:
    QFlags<Qt::MouseButton> btn_;
};

} // namespace tfp

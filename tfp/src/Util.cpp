#include "tfp/Util.hpp"

#include <QLayout>
#include <QSpacerItem>
#include <QWidget>

#include <qwt_plot_textlabel.h>


namespace tfp {

// ----------------------------------------------------------------------------
void Util::clearLayout(QLayout* layout)
{
    QLayoutItem* child;
    while((child = layout->takeAt(0)) != NULL)
    {
        if(child->layout() != NULL)
            child->layout()->deleteLater();
        if(child->widget() != NULL)
            child->widget()->deleteLater();
    }
}

// ----------------------------------------------------------------------------
QwtPlotTextLabel* Util::createLoadingTextLabel()
{
    QFont font;
    font.setPointSize(48);
    font.setBold(true);

    QwtText text;
    text.setText("Loading...");
    text.setFont(font);
    text.setColor(QColor(160, 160, 160));

    QwtPlotTextLabel* label = new QwtPlotTextLabel;
    label->setText(text);

    return label;
}

} // namespace tfp

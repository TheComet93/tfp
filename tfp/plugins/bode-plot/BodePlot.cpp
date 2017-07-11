#include "BodePlot.hpp"
#include "tfp/views/RealtimePlot.hpp"
#include "tfp/models/System.hpp"
#include "tfp/math/TransferFunction.hpp"
#include "tfp/util/Plugin.hpp"
#include <QVBoxLayout>
#include <qwt_plot_curve.h>
#include <qwt_scale_engine.h>

#if defined(PLUGIN_BUILDING)
#  define PLUGIN_API Q_DECL_EXPORT
#else
#  define PLUGIN_API Q_DECL_IMPORT
#endif

using namespace tfp;

extern "C" {

PLUGIN_API bool start_plugin(Plugin* plugin, DataTree* dataTree)
{
    return plugin->registerTool<BodePlot>(
        Plugin::VISUALISER,
        Plugin::LTI_SYSTEM_CONTINUOUS,
        "Bode Plot",
        "Alex Murray",
        "Visualise the amplitude and phase of a system.",
        "alex.murray@gmx.ch"
    );
}

PLUGIN_API void stop_plugin(Plugin* plugin)
{
}

}

// ----------------------------------------------------------------------------
BodePlot::BodePlot(QWidget* parent) :
    Tool(parent),
    ampPlot_(new RealtimePlot),
    phasePlot_(new RealtimePlot),
    amplitude_(new QwtPlotCurve),
    phase_(new QwtPlotCurve)
{
    setLayout(new QVBoxLayout);

    ampPlot_->enableRectangleZoom();
    ampPlot_->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine);
    layout()->addWidget(ampPlot_);

    phasePlot_->enableRectangleZoom();
    phasePlot_->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine);
    layout()->addWidget(phasePlot_);

    amplitude_->setTitle("Amplitude");
    amplitude_->attach(ampPlot_);

    phase_->setTitle("Phase");
    phase_->attach(phasePlot_);
}

// ----------------------------------------------------------------------------
void BodePlot::autoScale()
{
    ampPlot_->autoScale();
    phasePlot_->autoScale();
}

// ----------------------------------------------------------------------------
void BodePlot::replot()
{
    ampPlot_->replot();
    phasePlot_->replot();
}

// ----------------------------------------------------------------------------
void BodePlot::onSystemParametersChanged()
{
    double ampdata[1000];
    double phasedata[1000];
    double freqdata[1000];
    double xStart, xEnd;

    system_->interestingFrequencyInterval(&xStart, &xEnd);
    system_->frequencyResponse(freqdata, ampdata, phasedata, xStart, xEnd, 1000);
    amplitude_->setSamples(freqdata, ampdata, 1000);
    phase_->setSamples(freqdata, phasedata, 1000);

    replot();
    if (ampPlot_->lastScaleWasAutomatic())
        ampPlot_->autoScale();
    if (phasePlot_->lastScaleWasAutomatic())
        phasePlot_->autoScale();
}

// ----------------------------------------------------------------------------
void BodePlot::onSystemStructureChanged()
{
    autoScale();
}

#ifndef SPECTRO_PLOT_H
#define SPECTRO_PLOT_H

#include "qwt_plot.h"
#include "qwt_plot_spectrogram.h"
#include "z2vector.h"
#include "qwt_plot_grid.h"
#include "qwt_plot.h"
#include "spectro_draw.h"
#include "photoclass.h"
#include "qwt_plot_panner.h"
#include "colormap.h"
#include "coloralphamap.h"
#include "rasterdata.h"

class MyZoomer;
class Spectro_Plot : public QwtPlot
{
    Q_OBJECT
public:
    enum EType {Ampl,Phase,Re,Im};
    bool is_transparent;
    EType etype;
    Spectro_Plot(QWidget * = NULL);
    void draw_spectr(Z2Vector z2vector,double intx1,double intx2,double inty1,double inty2,int colormax,double colormin);
    QwtPlot *plot;
    Spectro_draw *draw_widget;
    QPointF point_for_zoomer1;
    QPointF point_for_zoomer2;

private:
    QwtPlotSpectrogram *d_spectrogram;
    QwtPlotGrid *grid;
    MyZoomer* zoomer;
    QwtPlotPanner *panner;
    ColorAlphamap *alpha_color_map;
    RasterData *raster;
    RasterData *data;
    ColorMap *color_map;

signals:
    void signal_from_vector(QString a);


public slots:
    void slot_for_spectr(QPointF pos);
    void slot_for_zoomer(QPointF p1,QPointF p2);
    double trans_x(double x);
    double trans_y(double y);
    double inv_trans_x(double x);
    double inv_trans_y(double y);

};

#endif // SPECTRO_PLOT_H





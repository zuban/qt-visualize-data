#include "zgraph.h"
#include "zvector.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "myqwtplot.h"
#include "QEvent"
#include "QObject"
#include "QMouseEvent"
#include "qwt_plot_canvas.h"
#include "QPen"
#include "qwt_picker_machine.h"
#include "cuda_dll_module.h"
#include "time.h"
#include <QMessageBox>



ZGraph::ZGraph(QWidget *parent) :
    QWidget(parent)
{
    eSpecialMarker=nonactive;
    myPlot = new MyQwtPlot(this);
    cuda_mas = NULL;
    panner =new QwtPlotPanner(myPlot->canvas());
    panner->setMouseButton( Qt::RightButton );
    d_picker = new QwtPlotPicker(myPlot->canvas());
    d_picker->setStateMachine( new QwtPickerDragPointMachine() );
    connect(d_picker, SIGNAL( appended( const QPoint & ) ),SLOT( click_on_canvas( const QPoint & ) ) );

    d_selectedCurve = NULL;
    d_selectedPoint = -1;

    markers_count=0;
    eRun=first;
    graphs_count=0;
gate_pos1 = 0.0;
gate_pos2 = 0.0;
    //lines color
    eColor[0]=black;
    eColor[1]=darkGreen;
    eColor[2]=darkCyan;
    eColor[3]=darkGray;
    eColor[4]=gray;
    eColor[5]=black;
    eColor[6]=darkYellow;
    for (int i=7;i<N_GRAPHS_MAX;i++)
    {
        eColor[i]=black;
    }

    gate_marker1 = NULL;


    reference_marker=new QwtPlotMarker();
    reference_marker=NULL;
    for (int i=0;i<N_GRAPHS_MAX;i++)
    {
        eCurveType[i]=SolidLine;
        curve_width[i]=1.5;
    }
 zVector[0]=new ZVector();
    line_symbol=new QwtSymbol( QwtSymbol::Diamond, Qt::green, Qt::NoPen, QSize( 10,10 ));
    for (int i=0;i<N_GRAPHS_MAX;i++)
    {
        if (i!=0)
            zVector[i]=NULL;
        curve[i]=new QwtPlotCurve();
        curve[i]->setPen(* new QPen(getRGB(eColor[i]),curve_width[i],getPen(eCurveType[i])));
        basic_marker[i]=new QwtPlotMarker();
        //        QwtSymbol *basic_symbol[N_GRAPHS_MAX];



        line_marker[i]=new QwtPlotMarker();
        line_marker[i]->setLineStyle(QwtPlotMarker::VLine );
        line_marker[i]->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);
        line_marker[i]->setLinePen(QPen(Qt::black, 1.5, Qt::DashLine));
        line_marker[i]->setSymbol(line_symbol);
    }


    eType=Ampl;
    eAmplScale=AmplAutoscale;
    eAmplType=AmplLog;
    myPlot->setMouseTracking(true);
    myPlot->canvas()->setMouseTracking(true);
    myPlot->canvas()->installEventFilter(this);

    gate_enable = false;

}
void ZGraph::set_gate_marker(double l_angl, double r_angl)
{
    if (gate_enable==true)
    {
        if (gate_marker1==NULL)
        {
            gate_marker1 =new QwtPlotMarker();
            gate_marker1->setLineStyle(QwtPlotMarker::VLine );
            gate_marker1->setLinePen(QPen(QColor(0, 226, 83, 0x80),r_angl-l_angl, Qt::SolidLine));
            gate_marker1->setValue( (r_angl+l_angl)/2,-100000000000000);
            gate_marker1->attach(myPlot);
            gate_pos1=l_angl;
            gate_pos2 = r_angl;
        }
        else
        {
            delete gate_marker1;
            gate_marker1 = NULL;
            gate_marker1 =new QwtPlotMarker();
            gate_marker1->setLineStyle(QwtPlotMarker::VLine );
            gate_marker1->setLinePen(QPen(QColor(0, 226, 83, 0x80),r_angl-l_angl, Qt::SolidLine));
            gate_marker1->setValue( (r_angl+l_angl)/2,-100000000000000);
            gate_marker1->attach(myPlot);
            gate_pos1=l_angl;
            gate_pos2 = r_angl;
        }
        myPlot->replot();
    }
    else
    {
        if (gate_marker1!=NULL)
        {
            delete gate_marker1;
            gate_marker1 = NULL;
        }
    }
}
void ZGraph::enable_gate_marker(bool en)
{
    gate_enable = en;
    if (en==false)
    {
        if (gate_marker1!=NULL)
        {
            delete gate_marker1;
            gate_marker1 = NULL;
        }
    }
    else
    {
        set_gate_marker(gate_pos1,gate_pos2);
    }
}
void ZGraph::Cuda_draw_default_graph(int num,int type)
{
    if (cuda_mas==NULL)
    {
        QMessageBox::information(0,"error","Set array");
        return;
    }
    if (eRun==first)
    {
        if (type==1)
        {
            if (num>N_k)
            {
                QMessageBox::information(0,"error","invalid row or col number");
                return;
            }
            graphs_count=1;
            zVector[0]->start=F_start;
            zVector[0]->stop=F_stop;

            zVector[0]->resize(N_k);
            zVector[0]->zLenght=zVector[0]->size();
            double *xs = new double[N_k];
            double *ys = new double[N_k];
            for (int i=0;i<N_k;i++)
            {
                for (int j=0;j<N_fi;j++)
                {
                    if (j==num)
                    {
                        int ind=i+j*N_k;
                        (*zVector[0])[i].x = cuda_mas[ind].x;
                        (*zVector[0])[i].y = cuda_mas[ind].y;
                    }
                }
            }

            for (int i=0;i<zVector[0]->zLenght;i++)
            {
                double_complex C=(*zVector[0])[i];
                xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
                ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
            myPlot->setCanvasBackground(Qt::white);
            curve[0]->setRenderHint(QwtPlotItem::RenderAntialiased);
            curve[0]->setData(dataLin);
            curve[0]->attach(myPlot);
            QwtPlotGrid *grid=new QwtPlotGrid();
            grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
            grid->setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
            grid->attach(myPlot);
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
            myPlot->setAxisAutoScale(QwtPlot::yRight,true);
            myPlot->setAxisAutoScale(QwtPlot::xTop,true);

            myPlot->mylegend->add_graph();
            myPlot->replot();
            delete xs;
            delete ys;
            eRun=nonfirst;

            emit change_value_freq(F_start + ((F_stop-F_start)/N_k)*num);
        }
        if (type==2)
        {
            if (num>N_fi)
            {
                QMessageBox::information(0,"error","invalid row or col number");
                return;
            }
            graphs_count=1;
            zVector[0]->start=AzStart;
            zVector[0]->stop=AzStop;
            zVector[0]->resize(N_fi);
            zVector[0]->zLenght=zVector[0]->size();
            double *xs = new double[N_fi];
            double *ys = new double[N_fi];
            for (int i=0;i<N_k;i++)
            {
                for (int j=0;j<N_fi;j++)
                {
                    if (i==num)
                    {
                        int ind=i+j*N_k;
                        (*zVector[0])[j].x = cuda_mas[ind].x;
                        (*zVector[0])[j].y = cuda_mas[ind].y;
                    }
                }
            }
            for (int i=0;i<zVector[0]->zLenght;i++)
            {
                double_complex C=(*zVector[0])[i];
                xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
                ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
            myPlot->setCanvasBackground(Qt::white);
            curve[0]->setRenderHint(QwtPlotItem::RenderAntialiased);
            curve[0]->setData(dataLin);
            curve[0]->attach(myPlot);
            QwtPlotGrid *grid=new QwtPlotGrid();
            grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
            grid->setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
            grid->attach(myPlot);
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
            myPlot->setAxisAutoScale(QwtPlot::yRight,true);
            myPlot->setAxisAutoScale(QwtPlot::xTop,true);

            myPlot->mylegend->add_graph();
            myPlot->replot();
            delete xs;
            delete ys;
            eRun=nonfirst;
            emit change_value_angle(AzStart + ((AzStart-AzStop)/N_fi)*num);

        }
        if (type==3)
        {
            graphs_count=1;
            zVector[0]->start=AzStart;
            zVector[0]->stop=AzStop;

            zVector[0]->resize(N_fi);
            zVector[0]->zLenght=zVector[0]->size();
            double *xs = new double[N_fi];
            double *ys = new double[N_fi];
            double Av = 0.0;
            for (int j=0;j<N_fi;j++)
            {
                (*zVector[0])[j].x = 0.0;
                (*zVector[0])[j].y = 0.0;
                for (int i=0;i<N_k;i++)
                {
                    int ind=i+j*N_k;
                    double A =  cuda_mas[ind].x* cuda_mas[ind].x+ cuda_mas[ind].y* cuda_mas[ind].y;
                    (*zVector[0])[j].x += A;
                }
                (*zVector[0])[j].x = sqrt((*zVector[0])[j].x / N_k);
            }
            for (int i=0;i<zVector[0]->zLenght;i++)
            {
                double_complex C=(*zVector[0])[i];
                xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
                ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
            myPlot->setCanvasBackground(Qt::white);
            curve[0]->setRenderHint(QwtPlotItem::RenderAntialiased);
            curve[0]->setData(dataLin);
            curve[0]->attach(myPlot);
            QwtPlotGrid *grid=new QwtPlotGrid();
            grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
            grid->setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
            grid->attach(myPlot);
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
            myPlot->setAxisAutoScale(QwtPlot::yRight,true);
            myPlot->setAxisAutoScale(QwtPlot::xTop,true);

            myPlot->mylegend->add_graph();
            myPlot->replot();
            delete xs;
            delete ys;
            eRun=nonfirst;
        }
        if (type==4)
        {
            graphs_count=1;
            zVector[0]->start=F_start;
            zVector[0]->stop=F_stop;

            zVector[0]->resize(N_k);
            zVector[0]->zLenght=zVector[0]->size();
            double *xs = new double[N_k];
            double *ys = new double[N_k];
            double Av = 0.0;
            for (int i=0;i<N_k;i++)
            {
                (*zVector[0])[i].x = 0.0;
                (*zVector[0])[i].y = 0.0;
                for (int j=0;j<N_fi;j++)
                {
                    int ind=i+j*N_k;
                    double A =  cuda_mas[ind].x* cuda_mas[ind].x+ cuda_mas[ind].y* cuda_mas[ind].y;
                    (*zVector[0])[i].x += A;
                }
                (*zVector[0])[i].x = sqrt((*zVector[0])[i].x / N_fi);
            }
            for (int i=0;i<zVector[0]->zLenght;i++)
            {
                double_complex C=(*zVector[0])[i];
                xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
                ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
            myPlot->setCanvasBackground(Qt::white);
            curve[0]->setRenderHint(QwtPlotItem::RenderAntialiased);
            curve[0]->setData(dataLin);
            curve[0]->attach(myPlot);
            QwtPlotGrid *grid=new QwtPlotGrid();
            grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
            grid->setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
            grid->attach(myPlot);
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
            myPlot->setAxisAutoScale(QwtPlot::yRight,true);
            myPlot->setAxisAutoScale(QwtPlot::xTop,true);
            myPlot->mylegend->add_graph();
            myPlot->replot();
            delete xs;
            delete ys;
            eRun=nonfirst;
        }
        if (type==6)
        {
            int id = (int)(N_fi/2);
//            if (id>N_k)
//            {
//                QMessageBox::information(0,"error","invalid row or col number");
//                return;
//            }
            graphs_count=1;
            zVector[0]->start=F_start;
            zVector[0]->stop=F_stop;

            zVector[0]->resize(N_k);
            zVector[0]->zLenght=zVector[0]->size();
            double *xs = new double[N_k];
            double *ys = new double[N_k];
            for (int i=0;i<N_k;i++)
            {
                for (int j=0;j<N_fi;j++)
                {
                    if (j==id)
                    {
                        int ind=i+j*N_k;
                        (*zVector[0])[i].x = cuda_mas[ind].x;
                        (*zVector[0])[i].y = cuda_mas[ind].y;
                    }
                }
            }

            for (int i=0;i<zVector[0]->zLenght;i++)
            {
                double_complex C=(*zVector[0])[i];
                xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
                ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
            myPlot->setCanvasBackground(Qt::white);
            curve[0]->setRenderHint(QwtPlotItem::RenderAntialiased);
            curve[0]->setData(dataLin);
            curve[0]->attach(myPlot);
            QwtPlotGrid *grid=new QwtPlotGrid();
            grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
            grid->setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
            grid->attach(myPlot);
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
            myPlot->setAxisAutoScale(QwtPlot::yRight,true);
            myPlot->setAxisAutoScale(QwtPlot::xTop,true);

            myPlot->mylegend->add_graph();
            myPlot->replot();
            delete xs;
            delete ys;
            eRun=nonfirst;
        }
        if (type==5)
        {
            int id = (int)(N_k/2);
//            if (id>N_fi)
//            {
//                QMessageBox::information(0,"error","invalid row or col number");
//                return;
//            }
            graphs_count=1;
            zVector[0]->start=AzStart;
            zVector[0]->stop=AzStop;
            zVector[0]->resize(N_fi);
            zVector[0]->zLenght=zVector[0]->size();
            double *xs = new double[N_fi];
            double *ys = new double[N_fi];
            for (int i=0;i<N_k;i++)
            {
                for (int j=0;j<N_fi;j++)
                {
                    if (i==id)
                    {
                        int ind=i+j*N_k;
                        (*zVector[0])[j].x = cuda_mas[ind].x;
                        (*zVector[0])[j].y = cuda_mas[ind].y;
                    }
                }
            }
            for (int i=0;i<zVector[0]->zLenght;i++)
            {
                double_complex C=(*zVector[0])[i];
                xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
                ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
            myPlot->setCanvasBackground(Qt::white);
            curve[0]->setRenderHint(QwtPlotItem::RenderAntialiased);
            curve[0]->setData(dataLin);
            curve[0]->attach(myPlot);
            QwtPlotGrid *grid=new QwtPlotGrid();
            grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
            grid->setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
            grid->attach(myPlot);
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
            myPlot->setAxisAutoScale(QwtPlot::yRight,true);
            myPlot->setAxisAutoScale(QwtPlot::xTop,true);

            myPlot->mylegend->add_graph();
            myPlot->replot();
            delete xs;
            delete ys;
            eRun=nonfirst;

        }

    }

    myPlot->replot();
    eSpecialMarker=active;
}

bool ZGraph::eventFilter(QObject *target, QEvent *event)
{
    if (target == myPlot->canvas())
    {
        if (event->type() == QEvent::MouseMove)
        {
            show_special_marker(myPlot->invTransform(QwtPlot::xBottom,((QMouseEvent*)event)->pos().x()));
            return false;
        }
        return false;
    }
    return true;
}

double ZGraph::getPointX(QwtPlotCurve* curveid,int Point)
{
    int k=0;
    for (int i=0;i<N_GRAPHS_MAX;i++)
    {
        if (curve[i]!=NULL)
            if (curveid==curve[i])
            {
                k=i;
            }
    }
    if (zVector[k]!=NULL)
        if (eType==Ampl)
        {
            if (eAmplType==AmplLog)
            {
                double_complex C=(*zVector[k])[Point];
                double xs=zVector[k]->start+(Point)*(zVector[k]->stop-zVector[k]->start)/(zVector[k]->zLenght-1);
                return xs;
            }

            if (eAmplType==AmplLin)
            {
                double_complex C=(*zVector[k])[Point];
                double xs=zVector[k]->start+Point*(zVector[k]->stop-zVector[k]->start)/(zVector[k]->zLenght-1);
                return xs;
            }
        }
    if (eType==Phase)
    {
        double_complex C=(*zVector[k])[Point];
        double  xs=zVector[k]->start+Point*(zVector[k]->stop-zVector[k]->start)/(zVector[k]->zLenght-1);
        return xs;
    }
    if (eType==Real)
    {
        double_complex C=(*zVector[k])[Point];
        double xs=zVector[k]->start+Point*(zVector[k]->stop-zVector[k]->start)/(zVector[k]->zLenght-1);
        return xs;
    }
    if (eType==Imag)
    {
        double_complex C=(*zVector[k])[Point];
        double xs=zVector[k]->start+Point*(zVector[k]->stop-zVector[k]->start)/(zVector[k]->zLenght-1);
        return xs;
    }
    return 0;
}

double ZGraph::getPointY(QwtPlotCurve* curveid, int Point)
{
    int k=0;
    for (int i=0;i<N_GRAPHS_MAX;i++)
    {
        if (curve[i]!=NULL)
            if (curveid==curve[i])
                k=i;
    }
    if (zVector[k]!=NULL)
        if (eType==Ampl)
        {
            if (eAmplType==AmplLog)
            {
                double_complex C=(*zVector[k])[Point];
                double  ys=10.0*log10(C.x*C.x+C.y*C.y+ 1.0e-20);
                return ys;
            }
            if (eAmplType==AmplLin)
            {
                double_complex C=(*zVector[k])[Point];
                double  ys=sqrt(C.x*C.x+C.y*C.y);
                return ys;
            }
        }

    if (eType==Phase)
    {
        double_complex C=(*zVector[k])[Point];
        double  ys=atan2(C.y,C.x)*180.0/3.1415926;
        return ys;
    }
    if (eType==Real)
    {
        double_complex C=(*zVector[k])[Point];
        double ys=C.x;
        return ys;
    }
    if (eType==Imag)
    {
        double_complex C=(*zVector[k])[Point];
        double ys=C.y;
        return ys;
    }
    return 0;
}

int ZGraph::z_getNumber(double x)
{
    if (zVector[0]==NULL)
        return 0;
    if (x<zVector[0]->start)
    {
        return 0;
    }
    if (x>zVector[0]->stop)
    {
        return zVector[0]->zLenght-1 ;
    }
    int i=int((x-zVector[0]->start)/(zVector[0]->stop-zVector[0]->start)*(zVector[0]->zLenght-1)+0.5);
    return i;
}
void ZGraph::show_special_marker(double x)
{
    if (eSpecialMarker==active)
    {
        int number=z_getNumber(x);

        for (int i=0;i<graphs_count;i++)
        {
            if (line_marker[i]==NULL)
            {
                if (line_marker[i]!=NULL)
                    line_marker[i]=new QwtPlotMarker();
            }
            line_marker[i]->detach();
            line_marker[i]->setValue( getPointX(curve[i],number), getPointY(curve[i],number));
            line_marker[i]->setSymbol(line_symbol);
            line_marker[i]->attach(myPlot);
        }
    }
    else
    {
    }
    myPlot->replot();
}

void ZGraph::redefine_type(QString type)
{
    if (type=="Amplitude")
    {
        eType=Ampl;
        eAmplScale=AmplAutoscale;
    }
    if (type=="Phase")
    {
        eType=Phase;
        ePhaseScale=PhaseAutoscale;
    }
    if (type=="Real")
    {
        eType=Real;
        eReImScale=ReAutoscale;
    }
    if (type=="Imag")
    {
        eType=Imag;
        eReImScale=ReAutoscale;
    }
    if (type=="Lin")
    {
        eAmplType=AmplLin;
        eAmplScale=AmplAutoscale;
    }
    if (type=="Log")
    {
        eAmplType=AmplLog;
        eAmplScale=AmplAutoscale;
    }
    reDraw();
    reDrawScale(0,0);
}

void ZGraph::reDraw()
{
    change_markers();
    for (int j=0;j<graphs_count;j++)
    {
        if (zVector[j]==NULL)
            continue;
        if (curve[j]==NULL)
            continue;
        if (eType==Ampl)
        {
            if (eAmplType==AmplLog)
            {
                double xs[zVector[j]->zLenght];
                double ys[zVector[j]->zLenght];
                for (int i=0;i<zVector[j]->zLenght;i++)
                {
                    double_complex C=(*zVector[j])[i];
                    xs[i]=zVector[j]->start+i*(zVector[j]->stop-zVector[j]->start)/(zVector[j]->zLenght-1);
                    ys[i]=10.0*log10(C.x*C.x+C.y*C.y+ 1.0e-20);
                }
                QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[j]->zLenght);
                curve[j]->setData(dataLin);
            }

            if (eAmplType==AmplLin)
            {
                double xs[zVector[j]->zLenght];
                double ys[zVector[j]->zLenght];
                for (int i=0;i<zVector[j]->zLenght;i++)
                {
                    double_complex C=(*zVector[j])[i];
                    xs[i]=zVector[j]->start+i*(zVector[j]->stop-zVector[j]->start)/(zVector[j]->zLenght-1);
                    ys[i]=sqrt(C.x*C.x+C.y*C.y+ 1.0e-20);
                }
                QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[j]->zLenght);
                curve[j]->setData(dataLin);
            }
        }
        if (eType==Phase)
        {
            double xs[zVector[j]->zLenght];
            double ys[zVector[j]->zLenght];
            for (int i=0;i<zVector[j]->zLenght;i++)
            {
                double_complex C=(*zVector[j])[i];
                xs[i]=zVector[j]->start+i*(zVector[j]->stop-zVector[j]->start)/(zVector[j]->zLenght-1);
                ys[i]=atan2(C.y,C.x)*180.0/3.1415926;
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[j]->zLenght);
            curve[j]->setData(dataLin);

        }
        if (eType==Real)
        {
            double xs[zVector[j]->zLenght];
            double ys[zVector[j]->zLenght];
            for (int i=0;i<zVector[j]->zLenght;i++)
            {
                double_complex C=(*zVector[j])[i];
                xs[i]=zVector[j]->start+i*(zVector[j]->stop-zVector[j]->start)/(zVector[j]->zLenght-1);
                ys[i]=C.x;
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[j]->zLenght);
            curve[j]->setData(dataLin);
        }
        if (eType==Imag)
        {
            double xs[zVector[j]->zLenght];
            double ys[zVector[j]->zLenght];
            for (int i=0;i<zVector[j]->zLenght;i++)
            {
                double_complex C=(*zVector[j])[i];
                xs[i]=zVector[j]->start+i*(zVector[j]->stop-zVector[j]->start)/(zVector[j]->zLenght-1);
                ys[i]=C.y;
            }
            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[j]->zLenght);
            curve[j]->setData(dataLin);
        }
        curve[j]->attach(myPlot);
    }
    myPlot->replot();

}
QColor ZGraph::getRGB(EColor color)
{
    if  (color==white)
    {
        return Qt::white;
    }
    if  (color==red)
    {
        return Qt::red;
    }
    if  (color==green)
    {
        return Qt::green;
    }
    if  (color==cyan)
    {
        return Qt::cyan;
    }
    if  (color==magenta)
    {
        return Qt::magenta;
    }
    if  (color==yellow)
    {
        return Qt::yellow;
    }
    if  (color==gray)
    {
        return Qt::gray;
    }
    if  (color==black)
    {
        return Qt::black;
    }
    if  (color==blue)
    {
        return Qt::blue;
    }
    if  (color==darkGreen)
    {
        return Qt::darkGreen;
    }
    if  (color==darkRed)
    {
        return Qt::darkRed;
    }
    if  (color==darkBlue)
    {
        return Qt::darkBlue;
    }
    if  (color==darkCyan)
    {
        return Qt::darkCyan;
    }
    if  (color==darkMagenta)
    {
        return Qt::darkMagenta;
    }
    if  (color==darkYellow)
    {
        return Qt::darkYellow;
    }
    if  (color==darkGray)
    {
        return Qt::darkGray;
    }
    if  (color==lightGray)
    {
        return Qt::lightGray;
    }
    return Qt::black;
}

Qt::PenStyle ZGraph::getPen(ECurveType type)
{

    if  (type==SolidLine)
    {
        return Qt::SolidLine;
    }
    if  (type==DashLine)
    {
        return Qt::DashLine;
    }
    if  (type==DotLine)
    {
        return Qt::DotLine;
    }
    if  (type==DashDotLine)
    {
        return Qt::DashDotLine;
    }
    if  (type==DashDotDotLine)
    {
        return Qt::DashDotDotLine;
    }
    if  (type==CustomDashLine)
    {
        return Qt::CustomDashLine;
    }
    return Qt::SolidLine;
}

void ZGraph::change_graph_color(QColor color)
{
    int k=myPlot->mylegend->active_graph;

    curve[k-1]->setPen(* new QPen(color,curve_width[k-1],getPen(eCurveType[k-1])));
    myPlot->mylegend->change_color(myPlot->mylegend->active_graph,color);
    myPlot->replot();

    if  (color==Qt::white)
    {
        eColor[k-1]=white;
    }
    if  (color==Qt::red)
    {
        eColor[k-1]=red;
    }
    if  (color==Qt::green)
    {
        eColor[k-1]=green;
    }
    if  (color==Qt::cyan)
    {
        eColor[k-1]=cyan;
    }
    if  (color==Qt::magenta)
    {
        eColor[k-1]=magenta;
    }
    if  (color==Qt::yellow)
    {
        eColor[k-1]=yellow;
    }
    if  (color==Qt::gray)
    {
        eColor[k-1]=gray;
    }
    if  (color==Qt::black)
    {
        eColor[k-1]=black;
    }
    if  (color==Qt::blue)
    {
        eColor[k-1]=blue;
    }

    if  (color==Qt::darkGreen)
    {
        eColor[k-1]=darkGreen;
    }
    if  (color==Qt::darkRed)
    {
        eColor[k-1]=darkRed;
    }
    if  (color==Qt::darkBlue)
    {
        eColor[k-1]=darkBlue;
    }
    if  (color==Qt::darkCyan)
    {
        eColor[k-1]=darkCyan;
    }
    if  (color==Qt::darkMagenta)
    {
        eColor[k-1]=darkMagenta;
    }
    if  (color==Qt::darkYellow)
    {
        eColor[k-1]=darkYellow;
    }
    if  (color==Qt::darkGray)
    {
        eColor[k-1]=darkGray;
    }
    if  (color==Qt::lightGray)
    {
        eColor[k-1]=lightGray;
    }
}
void ZGraph::change_graph_style(Qt::PenStyle style)
{
    int k=myPlot->mylegend->active_graph;
    curve[k-1]->setPen(* new QPen(getRGB(eColor[k-1]),curve_width[k-1],style));
    myPlot->mylegend->change_type(myPlot->mylegend->active_graph,style);
    myPlot->replot();
    if  (style==Qt::SolidLine)
    {
        eCurveType[k-1]=SolidLine;
    }
    if  (style==Qt::DashLine)
    {
        eCurveType[k-1]=DashLine;
    }
    if  (style==Qt::DotLine)
    {
        eCurveType[k-1]=DotLine;
    }
    if  (style==Qt::DashDotLine)
    {
        eCurveType[k-1]=DashDotLine;
    }
    if  (style==Qt::DashDotDotLine)
    {
        eCurveType[k-1]=DashDotDotLine;
    }
    if  (style==Qt::CustomDashLine)
    {
        eCurveType[k-1]=CustomDashLine;
    }
}
void ZGraph::change_graph_width(double x)
{
    int k=myPlot->mylegend->active_graph;
    curve[k-1]->setPen(* new QPen(getRGB(eColor[k-1]),x,getPen(eCurveType[k-1])));
    curve_width[k-1]=x;
    myPlot->replot();
}
void ZGraph::add_marker()
{
    if (myPlot->mymarker->marker_list.size()>myPlot->mymarker->nGraphsMax-1)
        return;
    append_marker1();
    myPlot->mymarker->markers_count=myPlot->mymarker->marker_list.size();
    myPlot->mymarker->repaint();
}
void ZGraph::delete_marker()
{
    if (myPlot->mymarker->markers_count>0)
    {
        if (myPlot->mymarker->active_marker==-1)
        {
            if (reference_marker!=NULL)
                delete reference_marker;
            reference_marker=NULL;
            ref_curve_list.clear();
        }
        else
        {
            delete_marker1();
            myPlot->mymarker->markers_count=myPlot->mymarker->marker_list.size();
            myPlot->mymarker->repaint();
            myPlot->replot();
        }
    }
    if (myPlot->mymarker->markers_count==0)
    {
        if (reference_marker!=NULL)
            delete reference_marker;
        reference_marker=NULL;
        ref_curve_list.clear();
    }
}

void ZGraph::select(const QPoint &pos)
{
    QwtPlotCurve *curve = NULL;
    double dist = 10e10;
    int index = -1;
    const QwtPlotItemList& itmList = myPlot->itemList();
    for ( QwtPlotItemIterator it = itmList.begin();
          it != itmList.end(); ++it )
    {
        if ( (*it)->rtti() == QwtPlotItem::Rtti_PlotCurve )
        {
            QwtPlotCurve *c = (QwtPlotCurve*)(*it);

            double d;
            int idx = c->closestPoint(pos, &d);
            if ( d < dist )
            {
                curve = c;
                index = idx;
                dist = d;
            }
        }
    }
    showCursor(false);
    d_selectedCurve = NULL;
    d_selectedPoint = -1;

    if ( curve && dist < 20 ) // 10 pixels tolerance
    {
        d_selectedCurve = curve;
        d_selectedPoint = index;
        myPlot->replot();
        showCursor(true);
    }
    else
    {
        // change_gate_marker(pos);
    }
}

void ZGraph::showCursor(bool showIt)
{
    if ( !d_selectedCurve )
    {
        return;
    }
    int k=myPlot->mymarker->active_marker;
    if (myPlot->mymarker->markers_count>0)
    {
        if  (k==-1)
        {
            if (reference_marker!=NULL)
            {
                delete reference_marker;
                reference_marker=NULL;
                ref_curve_list.clear();
            }
            reference_marker = new QwtPlotMarker();
            reference_marker->setSymbol(new QwtSymbol( QwtSymbol::Diamond, Qt::darkBlue, Qt::NoPen, QSize( 10, 10 ) ) );
            reference_marker->setValue( getPointX(d_selectedCurve,d_selectedPoint)    ,getPointY(d_selectedCurve,d_selectedPoint));
            ref_curve_list.append(d_selectedCurve);
            myPlot->mymarker->ref_x=getPointX(d_selectedCurve,d_selectedPoint);
            myPlot->mymarker->ref_y=getPointY(d_selectedCurve,d_selectedPoint);
            QwtText text("R     ");
            reference_marker->setLabel(text);
            reference_marker->attach(myPlot);
        }
        else
        {
            marker_list[k]->detach();
            marker_list[k]->setSymbol(new QwtSymbol( QwtSymbol::Diamond, Qt::red, Qt::NoPen, QSize( 10, 10 ) ) );
            marker_list[k]->setValue( getPointX(d_selectedCurve,d_selectedPoint)    ,getPointY(d_selectedCurve,d_selectedPoint));
            QString info_2 = ":  x = "+ QString::number( getPointX(d_selectedCurve,d_selectedPoint) ,'f',3) +
                    "; y = " + QString::number(getPointY(d_selectedCurve,d_selectedPoint),'f',3);
            myPlot->mymarker->xs[k]=getPointX(d_selectedCurve,d_selectedPoint);
            myPlot->mymarker->ys[k]=getPointY(d_selectedCurve,d_selectedPoint);
            curve_list[k]=d_selectedCurve;
            *(myPlot->mymarker->marker_list[k])=info_2;
            marker_list[k]->attach(myPlot);
            myPlot->replot();
            myPlot->mymarker->repaint();
        }
    }
}

void ZGraph::click_on_canvas(const QPoint &pos)
{
    select(pos);
}
void ZGraph::reference_marker_toggled(bool checked)
{
    if (myPlot->mymarker==NULL) return;
    if (checked==true)
    {
        myPlot->mymarker->reference_marker_mode=MyMarker::active;
        myPlot->mymarker->active_marker=-1;
        myPlot->mymarker->set_enable_delete_button(false);
        myPlot->mymarker->repaint();

    }
    if (checked==false)
    {
        myPlot->mymarker->reference_marker_mode=MyMarker::nonactive;
        myPlot->mymarker->active_marker=0;
        myPlot->mymarker->set_enable_delete_button(true);
        myPlot->mymarker->repaint();
    }
}
void ZGraph::append_marker1()
{
    QwtPlotMarker *q=new QwtPlotMarker();
    marker_list.append(q);
    QString* str=new QString("   Marker");
    myPlot->mymarker->marker_list.append(str);
    myPlot->mymarker->active_marker=myPlot->mymarker->marker_list.size()-1;
    myPlot->mymarker->set_enable_delete_button(true);
    display_numbers();
    curve_list.append(NULL);
    myPlot->mymarker->repaint();
}
void ZGraph::delete_marker1()
{

    delete marker_list[myPlot->mymarker->active_marker];
    delete myPlot->mymarker->marker_list[myPlot->mymarker->active_marker];
    myPlot->mymarker->marker_list.removeAt(myPlot->mymarker->active_marker);
    marker_list.removeAt(myPlot->mymarker->active_marker);
    curve_list.removeAt(myPlot->mymarker->active_marker);
    myPlot->mymarker->active_marker=marker_list.size()-1;
    myPlot->mymarker->set_enable_delete_button(true);
    display_numbers();
    myPlot->mymarker->repaint();
}
void ZGraph::display_numbers()
{
    if (marker_list.count()>0)
    {
        for (int i=0;i<marker_list.count();i++)
        {
            marker_list[i]->setLabel(QString::number(i+1)+"     ");
        }
    }
}
void ZGraph::change_markers()
{
    if (marker_list.size()==0)
        return;
    for (int i=0;i<marker_list.size();i++)
    {
        marker_list[i]->detach();
        marker_list[i]->setValue(getPointX(curve_list[i],z_getNumber(myPlot->mymarker->xs[i])),getPointY(curve_list[i],z_getNumber(myPlot->mymarker->xs[i])));
        QString info = ":  x = "+ QString::number(getPointX(curve_list[i],z_getNumber(myPlot->mymarker->xs[i])) ,'f',3) +
                "; y = " + QString::number(getPointY(curve_list[i],z_getNumber(myPlot->mymarker->xs[i])),'f',3);
        myPlot->mymarker->xs[i]=getPointX(curve_list[i],z_getNumber(myPlot->mymarker->xs[i]));
        myPlot->mymarker->ys[i]=getPointY(curve_list[i],z_getNumber(myPlot->mymarker->xs[i]));
        *(myPlot->mymarker->marker_list[i])=info;
        myPlot->mymarker->repaint();
        marker_list[i]->attach(myPlot);
    }
    if (ref_curve_list.size()!=0)
    {

        reference_marker->detach();
        reference_marker->setValue(getPointX(ref_curve_list[0],z_getNumber(myPlot->mymarker->ref_x)),getPointY(ref_curve_list[0],z_getNumber(myPlot->mymarker->ref_x)));
        myPlot->mymarker->ref_x=getPointX(ref_curve_list[0],z_getNumber(myPlot->mymarker->ref_x));
        myPlot->mymarker->ref_y=getPointY(ref_curve_list[0],z_getNumber(myPlot->mymarker->ref_x));
        myPlot->mymarker->repaint();
        reference_marker->attach(myPlot);
    }
    myPlot->replot();
}


void ZGraph::reDrawScale(double Max,double Span)
{

    if (eType==Ampl)
    {
        if (eAmplType==AmplLog)
        {
            if (eAmplScale==AmplAutoscale)
            {
                myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
                myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
            }
            if (eAmplScale==AmplManual)
            {
                myPlot->setAxisScale(QwtPlot::yLeft,(Max-Span),Max);
            }
            if (eAmplScale==AmplUp)
            {
                double span1=20*log10(Span);
                double b=20*log10(get_max_compare());
                myPlot->setAxisScale(QwtPlot::yLeft,b-span1,b);
            }
        }
        if (eAmplType==AmplLin)
        {
            if (eAmplScale==AmplAutoscale)
            {
                myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
                myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
            }
            if (eAmplScale==AmplManual)
            {
                double span11=20*log10(Span);
                double b1=20*log10(Max);
                myPlot->setAxisScale(QwtPlot::yLeft,(b1-span11),b1);
            }
            if (eAmplScale==AmplUp)  //�� ��������� ��������
            {
                myPlot->setAxisScale(QwtPlot::yLeft,0,get_max_compare());
            }
        }
    }
    if (eType==Phase)
    {
        if (ePhaseScale==PhaseAutoscale)
        {
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
        }
        if (ePhaseScale==PhaseManual)
        {
            double tempSpan=Span;
            double tempCenter=Max;
            myPlot->setAxisScale(QwtPlot::yLeft,tempCenter-tempSpan/2,tempCenter+tempSpan/2);
        }
        if (ePhaseScale==PhaseUp)
        {
            double tempSpan=Span;
            double tempCenter=Max;//��������� ������������
            myPlot->setAxisScale(QwtPlot::yLeft,tempCenter-tempSpan/2,tempCenter+tempSpan/2);
        }
    }
    if (eType==Real)
    {
        if (eReImScale==ReAutoscale)
        {
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
        }
        if (eReImScale==ReManual)
        {
            double tempSpan=Span;
            double tempCenter=Max;
            myPlot->setAxisScale(QwtPlot::yLeft,tempCenter-tempSpan/2,tempCenter+tempSpan/2);
        }
        if (eReImScale==ReUp)
        {
            double tempSpan=Span;
            double tempCenter=Max;
            myPlot->setAxisScale(QwtPlot::yLeft,tempCenter-tempSpan/2,tempCenter+tempSpan/2);
        }
    }
    if (eType==Imag)
    {
        if (eReImScale==ReAutoscale)
        {
            myPlot->setAxisAutoScale(QwtPlot::xBottom,true);
            myPlot->setAxisAutoScale(QwtPlot::yLeft,true);
        }
        if (eReImScale==ReManual)
        {
            double tempSpan=Span;
            double tempCenter=Max;
            myPlot->setAxisScale(QwtPlot::yLeft,tempCenter-tempSpan/2,tempCenter+tempSpan/2);
        }
        if (eReImScale==ReUp)
        {
            double tempSpan=Span;
            double tempCenter=Max;
            myPlot->setAxisScale(QwtPlot::yLeft,tempCenter-tempSpan/2,tempCenter+tempSpan/2);
        }
    }
    myPlot->replot();
}
double ZGraph::get_max_compare()
{
    if (graphs_count!=0)
    {
        double max=-100000000000000;
        for (int i=0;i<graphs_count;i++)
        {
            if (max<zVector[i]->getMaxValue())
                max=zVector[i]->getMaxValue();
        }
        return max;
    }
    return 0;
}
double ZGraph::get_min_compare()
{
    if (graphs_count!=0)
    {
        double min=10000000000000000;
        for (int i=0;i<graphs_count;i++)
        {
            if (min>zVector[i]->getMinValue())
                min=zVector[i]->getMinValue();
        }
        return min;
    }
    return 0;
}

void ZGraph::enable_xy_zoomer()
{
    if (zoomer!=NULL)
        return;
    zoomer = new QwtPlotZoomer(QwtPlot::xBottom,QwtPlot::yLeft,myPlot->canvas(),false);
}

void ZGraph::delete_xy_zoomer()
{
    delete zoomer;
    zoomer = NULL;
}

void ZGraph::enable_x_zoomer()
{
    if (zoomer!=NULL)
        return;
    zoomer = new QwtPlotZoomer(QwtPlot::xBottom,QwtPlot::yRight,myPlot->canvas(),false);
}

void ZGraph::delete_x_zoomer()
{
    delete zoomer;
    zoomer = NULL;
}

void ZGraph::enable_y_zoomer()
{
    if (zoomer!=NULL)
        return;
    zoomer = new QwtPlotZoomer(QwtPlot::xTop,QwtPlot::yLeft,myPlot->canvas(),false);
}

void ZGraph::delete_y_zoomer()
{
    delete zoomer;
    zoomer = NULL;
}

void ZGraph::enable_magnifier()
{
    if (magnifier==NULL)
        magnifier =new QwtPlotMagnifier(myPlot->canvas());
    if (scrollzoomer==NULL)
        scrollzoomer=new  ScrollZoomer(myPlot->canvas());
    magnifier->setMouseButton(Qt::NoButton);
    magnifier->setAxisEnabled(QwtPlot::yLeft,false);
}
void ZGraph::delete_magnifier()
{
    delete magnifier;
    delete scrollzoomer;
    magnifier = NULL;
    scrollzoomer = NULL;
}

void ZGraph::resizeMyPlot(int x, int y)
{
    myPlot->resize(x,y);
    myPlot->mylegend->move(x-200,20);
}

void ZGraph::Cuda_redraw_graph(int num,int type)
{
    if (cuda_mas==NULL)
    {
        QMessageBox::information(0,"error","Set array");
        return;
    }
    if (type==1)
    {
        if (num>N_k)
        {
            QMessageBox::information(0,"error","invalid row or col number");
            return;
        }

        zVector[0]->start=F_start;
        zVector[0]->stop=F_stop;

        zVector[0]->resize(N_k);
        zVector[0]->zLenght=zVector[0]->size();
        double *xs = new double[N_k];
        double *ys = new double[N_k];
        for (int i=0;i<N_k;i++)
        {
            for (int j=0;j<N_fi;j++)
            {
                if (j==num)
                {
                    int ind=i+j*N_k;
                    (*zVector[0])[i].x = cuda_mas[ind].x;
                    (*zVector[0])[i].y = cuda_mas[ind].y;
                }

            }
        }

        for (int i=0;i<zVector[0]->zLenght;i++)
        {
            double_complex C=(*zVector[0])[i];
            xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
            ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
        }

        QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
        curve[0]->setData(dataLin);
        curve[0]->attach(myPlot);
        myPlot->replot();
        delete xs;
        delete ys;
        emit change_value_freq(F_start + ((F_stop-F_start)/N_k)*num);
    }
    if (type==2)
    {
        if (num>N_fi)
        {
            QMessageBox::information(0,"error","invalid row or col number");
            return;
        }
        zVector[0]->start=AzStart;
        zVector[0]->stop=AzStop;

        zVector[0]->resize(N_fi);
        zVector[0]->zLenght=zVector[0]->size();
        double *xs = new double[N_fi];
        double *ys = new double[N_fi];
        for (int i=0;i<N_k;i++)
        {
            for (int j=0;j<N_fi;j++)
            {
                if (i==num)
                {
                    int ind=i+j*N_k;
                    (*zVector[0])[j].x = cuda_mas[ind].x;
                    (*zVector[0])[j].y = cuda_mas[ind].y;
                }

            }
        }

        for (int i=0;i<zVector[0]->zLenght;i++)
        {
            double_complex C=(*zVector[0])[i];
            xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
            ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
        }

        QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
        curve[0]->setData(dataLin);
        curve[0]->attach(myPlot);
        myPlot->replot();
        delete xs;
        delete ys;
         emit change_value_angle(AzStart + ((AzStart-AzStop)/N_fi)*num);
    }
    if (type==3)
    {
        zVector[0]->start=AzStart;
        zVector[0]->stop=AzStop;

        zVector[0]->resize(N_fi);
        zVector[0]->zLenght=zVector[0]->size();
        double *xs = new double[N_fi];
        double *ys = new double[N_fi];

        double Av = 0.0;
        for (int j=0;j<N_fi;j++)
        {
            (*zVector[0])[j].x = 0.0;
            (*zVector[0])[j].y = 0.0;
            for (int i=0;i<N_k;i++)
            {
                int ind=i+j*N_k;
                double A =  cuda_mas[ind].x* cuda_mas[ind].x+ cuda_mas[ind].y* cuda_mas[ind].y;
                (*zVector[0])[j].x += A;
            }
            (*zVector[0])[j].x = sqrt((*zVector[0])[j].x / N_k);
        }

        for (int i=0;i<zVector[0]->zLenght;i++)
        {
            double_complex C=(*zVector[0])[i];
            xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
            ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
        }

        QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
        curve[0]->setData(dataLin);
        curve[0]->attach(myPlot);
        myPlot->replot();
        delete xs;
        delete ys;
    }
    if (type==4)
    {
        zVector[0]->start=F_start;
        zVector[0]->stop=F_stop;

        zVector[0]->resize(N_k);
        zVector[0]->zLenght=zVector[0]->size();
        double *xs = new double[N_k];
        double *ys = new double[N_k];
        double Av = 0.0;
        for (int i=0;i<N_k;i++)
        {
            (*zVector[0])[i].x = 0.0;
            (*zVector[0])[i].y = 0.0;
            for (int j=0;j<N_fi;j++)
            {
                int ind=i+j*N_k;
                double A =  cuda_mas[ind].x* cuda_mas[ind].x+ cuda_mas[ind].y* cuda_mas[ind].y;
                (*zVector[0])[i].x += A;
            }
            (*zVector[0])[i].x = sqrt((*zVector[0])[i].x / N_fi);
        }

        for (int i=0;i<zVector[0]->zLenght;i++)
        {
            double_complex C=(*zVector[0])[i];
            xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
            ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
        }

        QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
        curve[0]->setData(dataLin);
        curve[0]->attach(myPlot);
        myPlot->replot();
        delete xs;
        delete ys;
    }
    if (type==6)
    {
        int id = (int)(N_fi/2);
//        if (id>N_k)
//        {
//            QMessageBox::information(0,"error","invalid row or col number");
//            return;
//        }
        zVector[0]->start=F_start;
        zVector[0]->stop=F_stop;

        zVector[0]->resize(N_k);
        zVector[0]->zLenght=zVector[0]->size();
        double *xs = new double[N_k];
        double *ys = new double[N_k];
        for (int i=0;i<N_k;i++)
        {
            for (int j=0;j<N_fi;j++)
            {
                if (j==id)
                {
                    int ind=i+j*N_k;
                    (*zVector[0])[i].x = cuda_mas[ind].x;
                    (*zVector[0])[i].y = cuda_mas[ind].y;
                }

            }
        }

        for (int i=0;i<zVector[0]->zLenght;i++)
        {
            double_complex C=(*zVector[0])[i];
            xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
            ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
        }

        QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
        curve[0]->setData(dataLin);
        curve[0]->attach(myPlot);
        myPlot->replot();
        delete xs;
        delete ys;
    }
    if (type==5)
    {
        int id = (int)(N_k/2);
        if (id>N_fi)
        {
            QMessageBox::information(0,"error","invalid row or col number");
            return;
        }
        zVector[0]->start=AzStart;
        zVector[0]->stop=AzStop;

        zVector[0]->resize(N_fi);
        zVector[0]->zLenght=zVector[0]->size();
        double *xs = new double[N_fi];
        double *ys = new double[N_fi];
        for (int i=0;i<N_k;i++)
        {
            for (int j=0;j<N_fi;j++)
            {
                if (i==id)
                {
                    int ind=i+j*N_k;
                    (*zVector[0])[j].x = cuda_mas[ind].x;
                    (*zVector[0])[j].y = cuda_mas[ind].y;
                }

            }
        }

        for (int i=0;i<zVector[0]->zLenght;i++)
        {
            double_complex C=(*zVector[0])[i];
            xs[i]=zVector[0]->start+i*(zVector[0]->stop-zVector[0]->start)/(zVector[0]->zLenght-1);
            ys[i]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
        }

        QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[0]->zLenght);
        curve[0]->setData(dataLin);
        curve[0]->attach(myPlot);
        myPlot->replot();
        delete xs;
        delete ys;
    }

}

void ZGraph::get_data_2D(QString str)
{
    bool f = true;
    if (cuda_mas!=NULL)
    {
        delete cuda_mas;
        f = false;
    }

    cuda_mas=NULL;


    cdata->d_readmasSize(str,N_k,N_fi);
    cuda_mas=new double_complex[N_k*N_fi];
    cdata->d_readFile(str,N_k,N_fi,F_start,F_stop,AzStart,AzStop,cuda_mas);
    if (f==false)
    {
        emit replot_2d();

    }
}
void ZGraph::signal_on_copy_pushButton_clicked()
{
    for (int i=1;i<N_GRAPHS_MAX;i++)
    {
        if (zVector[i]!=NULL)
            continue;
        else
        {
            zVector[i] = new ZVector();
            zVector[i]->resize(zVector[0]->zLenght);
            zVector[i]->start = zVector[0]->start;
            zVector[i]->stop = zVector[0]->stop;
            zVector[i]->zLenght = zVector[0]->zLenght;
            for (int j=0;j<zVector[i]->zLenght;j++)
            {
                (*zVector[i])[j]= (*zVector[0])[j];
            }
            double *xs = new double[zVector[i]->zLenght];
            double *ys = new double[zVector[i]->zLenght];


            for (int k=0;k<zVector[i]->zLenght;k++)
            {
                double_complex C=(*zVector[i])[k];
                xs[k]=zVector[i]->start+k*(zVector[i]->stop-zVector[i]->start)/(zVector[i]->zLenght-1);
                ys[k]=10.0*log10(C.x*C.x+C.y*C.y + 1.0e-20);
            }

            QwtPointArrayData *dataLin = new QwtPointArrayData(&xs[0],&ys[0],zVector[i]->zLenght);
            curve[i]->setData(dataLin);
            curve[i]->attach(myPlot);
            myPlot->replot();

            delete xs;
            delete ys;
            graphs_count++;
            myPlot->mylegend->add_graph();
            return;
        }
    }
}
void ZGraph::signal_on_clear_pushButton_clicked()
{
    for (int i=1;i<N_GRAPHS_MAX;i++)
    {
        if (zVector[i]==NULL)
            continue;
        else
        {
            delete zVector[i];
            zVector[i] = NULL;

            if (curve[i]!=NULL)
            {
            delete curve[i];
            curve[i]=new QwtPlotCurve();
            curve[i]->setPen(* new QPen(getRGB(eColor[i]),curve_width[i],getPen(eCurveType[i])));
            }
            myPlot->mylegend->delete_legend();
            myPlot->mylegend->add_graph();
        }
    }
    myPlot->replot();
}


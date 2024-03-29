#include "spectrwidget.h"
#include "ui_spectrwidget.h"
#include "QFileDialog"
#include "time.h"
#include "QLabel"
#include "QGraphicsScene"
#include "QCoreApplication"
#include "cuda_dll_module.h"
#include "ui_form_spectr.h"
#include "form_spectr.h"
#include <QMessageBox>
#include "QImageWriter"
#include "qwt_plot_renderer.h"


double GLOBAL_F_start;
double GLOBAL_F_stop;

bool IS_DOUBLE = true;

float FLOAT_GLOBAL_F_start;
float FLOAT_GLOBAL_F_stop;
//#include "H5Cpp.h"
//struct double2 {double x; double y;};

SpectrWidget::SpectrWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpectrWidget)
{
    ui->setupUi(this);
    zoomer_mod = false;
    z2vector = new Z2Vector();
    sp_plot=new Spectro_Plot(this);
    sp_plot->setWindowOpacity(0.5);
    sp_plot->move(5,60);
    sp_plot->resize(this->width()-5,this->height()-60);
    control=NULL;
    data=NULL;
    ui->progressBar->setValue(0);
    ui->polygon_pushButton->setChecked(true);

//    connect(this, SIGNAL(QCoreApplication::aboutToQuit()), this, SLOT(closing()));


    connect(sp_plot->draw_widget,SIGNAL(signla_x_shift(double)),this,SLOT(change_x_shift(double)));
    connect(sp_plot->draw_widget,SIGNAL(signla_y_shift(double)),this,SLOT(change_y_shift(double)));
    connect(sp_plot->draw_widget,SIGNAL(signla_x_scale(double)),this,SLOT(change_x_scale(double)));
    connect(sp_plot->draw_widget,SIGNAL(signla_y_scale(double)),this,SLOT(change_y_scale(double)));
    connect(sp_plot->draw_widget,SIGNAL(signla_fi(double)),this,SLOT(change_fi(double)));
    connect(sp_plot->draw_widget,SIGNAL(signla_k(double)),this,SLOT(change_k(double)));

    connect(sp_plot->draw_widget,SIGNAL(signal_to_change_scale_scale1(double)),this,SLOT(slot_to_change_scale_scale1(double)));
    connect(sp_plot->draw_widget,SIGNAL(signal_to_change_scale_scale2(double)),this,SLOT(slot_to_change_scale_scale2(double)));
    connect(sp_plot->draw_widget,SIGNAL(signal_to_change_scale_angle1(double,double)),this,SLOT(slot_to_change_scale_angle1(double,double)));
    connect(sp_plot->draw_widget,SIGNAL(signal_to_change_scale_point1(QPointF)),this,SLOT(slot_to_change_scale_point1(QPointF)));
    connect(sp_plot->draw_widget,SIGNAL(signal_to_change_scale_point22(QPointF)),this,SLOT(slot_to_change_scale_point22(QPointF)));
    connect(sp_plot->draw_widget,SIGNAL(signal_to_change_scale_angle2(double,double)),this,SLOT(slot_to_change_scale_angle2(double,double)));
    connect(sp_plot->draw_widget,SIGNAL(signal_test(QString)),this,SLOT(slot_test(QString)));
    this->showMaximized();

}

SpectrWidget::~SpectrWidget()
{
    delete sp_plot;
    delete z2vector;
    delete data;
    delete ui;
}

//void SpectrWidget::closing()
//{
//    printf("clllose");
//    QApplication::quit();
//}

void SpectrWidget::on_read_pushbutton_clicked()
{
    if (data!=NULL)
    {
        QMessageBox::information(0,"error","Only one graph");
        return;
    }
    QString way = QFileDialog::getOpenFileName(this, tr("Open File"), "",tr(".bin Files (*.bin)"));
    if (way==0)
    {
        return;
    }

    ui->progressBar->setValue(10);
    data = new IOData(this);
    int N_k;
    int N_fi;
    double F_start,F_stop, AzStart,AzStop;
    ui->progressBar->setValue(15);
    data->d_readmasSize(way,N_k,N_fi);
    double_complex *masin=new double_complex[N_k*N_fi];



    ui->progressBar->setValue(20);
    data->d_readFile(way,N_k,N_fi,F_start,F_stop,AzStart,AzStop,masin);
    control->ui->Nk_lineedit_data->setText(QString::number(N_k));
    control->ui->Nfi_lineedit_data->setText(QString::number(N_fi));
    control->ui->Fstart_lineedit_data->setText(QString::number(F_start));
    control->ui->Fstop_lineedit_data->setText(QString::number(F_stop));
    control->ui->Azstart_lieedit_data->setText(QString::number(AzStart));
    control->ui->Azstop_lineedit_data->setText(QString::number(AzStop));
    ui->progressBar->setValue(50);

    emit set_data_2D(way);

    if (IS_DOUBLE)
    {
        double pi = 3.1415926;
        bool bWhole = true;
        int NEWCOL=2048;// int NEWCOL=2048;
        int NEWROW=1024;
        double fi_degspan = 20.0;
        if(bWhole){
            fi_degspan = 360.0;
            // N_fi = 15000;
        }
        double_complex *mas=new double_complex[NEWCOL*NEWROW];
        bool a = SetArrayZ2Z(N_k,N_fi,F_start,F_stop,AzStart,AzStop,(double*)masin,false);
        ui->progressBar->setValue(60);

        double dAzStart =  40;
        double dAzStop =  60;
        bool b = CalcZ2Z(NEWCOL,NEWROW,F_start,F_stop,dAzStart,dAzStop,(double*)mas);
        ui->progressBar->setValue(70);

        GLOBAL_F_start = F_start;
        GLOBAL_F_stop  = F_stop;
        z2vector->resize(NEWCOL*NEWROW);
        z2vector->set_zero_vector(NEWCOL,NEWROW,-100,100,-100,100);
        for (int i=0;i<z2vector->size()-1;i++)
        {
            (*z2vector)[i].x = mas[i].x;
            (*z2vector)[i].y = mas[i].y;
        }
        ui->progressBar->setValue(90);

        sp_plot->draw_spectr(*z2vector,get_zstart()+control->ui->spectr_shift_lineedit->text().toDouble(),get_zstop()+control->ui->spectr_shift_lineedit->text().toDouble(),get_xstart(),get_xstop(),ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
        z2vector->startx = get_zstart()+control->ui->spectr_shift_lineedit->text().toDouble();
        z2vector->stopx = get_zstop()+control->ui->spectr_shift_lineedit->text().toDouble();
        z2vector->starty = get_xstart();
        z2vector->stopy = get_xstop();
        double scale = max(fabs(get_xstart()-get_xstop()),fabs(get_zstart()-get_zstop()));
        z2vector->scale = scale;
            sp_plot->setAxisScale(QwtPlot::yLeft,-scale/2,scale/2);
            sp_plot->setAxisScale(QwtPlot::xBottom,-scale/2,scale/2);


        sp_plot->replot();
        if (zoomer_mod)
            sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
        ui->progressBar->setValue(100);
        delete mas;
    }
    else {

//        bool bWhole = true;
//        int NEWCOL=2048;
//        int NEWROW=1024;
//        float fi_degspan = 20.0;
//        if(bWhole){
//            fi_degspan = 360.0;
//            //N_fi = 15000;
//        }
//        float_complex *mas=new float_complex[NEWCOL*NEWROW];
//        bool a = SetArrayC2C(N_k,N_fi,F_start,F_stop,AzStart,AzStop,(float*)masin,true);
//        float dAzStart =  40;
//        float dAzStop =  60;
//        bool b = CalcC2C(NEWCOL,NEWROW,F_start,F_stop,dAzStart,dAzStop,(float*)mas);
//        GLOBAL_F_start = F_start;
//        GLOBAL_F_stop  = F_stop;
//        z2vector->resize(NEWCOL*NEWROW);
//        z2vector->set_zero_vector(NEWCOL,NEWROW,-100,100,-100,100);
//        for (int i=0;i<z2vector->size()-1;i++)
//        {
//            (*z2vector)[i].x = mas[i].x;
//            (*z2vector)[i].y = mas[i].y;
//        }
//        ui->progressBar->setValue(90);
//        sp_plot->draw_spectr(*z2vector,f_get_zstart()+control->ui->spectr_shift_lineedit->text().toDouble(),f_get_zstop()+control->ui->spectr_shift_lineedit->text().toDouble(),f_get_xstart(),f_get_xstop(),ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
//        ui->progressBar->setValue(100);
//        delete mas;







        float pi = 3.1415926;
        bool bWhole = true;
        int NEWCOL=2048;// int NEWCOL=2048;
        int NEWROW=1024;
        float fi_degspan = 20.0;
        if(bWhole){
            fi_degspan = 360.0;
            // N_fi = 15000;
        }
        float_complex *mas=new float_complex[NEWCOL*NEWROW];
        bool a = SetArrayC2C(N_k,N_fi,(float)F_start,(float)F_stop,(float)AzStart,(float)AzStop,(float*)masin,true);
        ui->progressBar->setValue(60);

        float dAzStart =  40;
        float dAzStop =  60;
        bool b = CalcC2C(NEWCOL,NEWROW,(float)F_start,(float)F_stop,(float)dAzStart,(float)dAzStop,(float*)mas);
        ui->progressBar->setValue(70);

        FLOAT_GLOBAL_F_start = (float)F_start;
        FLOAT_GLOBAL_F_stop  = (float)F_stop;
        z2vector->resize(NEWCOL*NEWROW);
        z2vector->set_zero_vector(NEWCOL,NEWROW,-100,100,-100,100);
        for (int i=0;i<z2vector->size()-1;i++)
        {
            (*z2vector)[i].x = mas[i].x;
            (*z2vector)[i].y = mas[i].y;
        }
        ui->progressBar->setValue(90);

        sp_plot->draw_spectr(*z2vector,f_get_zstart()+control->ui->spectr_shift_lineedit->text().toFloat(),f_get_zstop()+control->ui->spectr_shift_lineedit->text().toFloat(),f_get_xstart(),f_get_xstop(),ui->max_spinbox->text().toFloat(),ui->min_spinbox->text().toFloat());
        z2vector->startx = f_get_zstart()+control->ui->spectr_shift_lineedit->text().toFloat();
        z2vector->stopx = f_get_zstop()+control->ui->spectr_shift_lineedit->text().toFloat();
        z2vector->starty = f_get_xstart();
        z2vector->stopy = f_get_xstop();
        double scale = max(fabs(f_get_xstart()-f_get_xstop()),fabs(f_get_zstart()-f_get_zstop()));
        z2vector->scale = scale;
            sp_plot->setAxisScale(QwtPlot::yLeft,-scale/2,scale/2);
            sp_plot->setAxisScale(QwtPlot::xBottom,-scale/2,scale/2);


        sp_plot->replot();
        if (zoomer_mod)
            sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
        ui->progressBar->setValue(100);
        delete mas;
    }
    delete masin;
emit set_gate_marker(-control->ui->Az_span_lineEdit->text().toDouble()/2,control->ui->Az_span_lineEdit->text().toDouble()/2);
delete data;
}







bool bInProgress = false;
void SpectrWidget::change_FI0_cuda_test(int count)
{

    if (IS_DOUBLE)
    {
        ui->progressBar->setValue(50);
        if(bInProgress) return;
        bInProgress = true;
        int NEWCOL=2048;
        int NEWROW=1024;
        double_complex *mas=new double_complex[NEWCOL*NEWROW];
        double dAzStart = (double)count - control->ui->Az_span_lineEdit->text().toDouble()/2;
        double dAzStop = (double)count + control->ui->Az_span_lineEdit->text().toDouble()/2;
        try{
            bool b = CalcZ2Z(NEWCOL,NEWROW,GLOBAL_F_start,GLOBAL_F_stop,dAzStart,dAzStop,(double*)mas);
        }
        catch(...) {
            printf ("cuda error occur 205 line spectwidget\n");
            bInProgress = false;
            delete mas;
            return;
        }

        try{
            //test end
            z2vector->resize(NEWROW*NEWCOL);

            z2vector->set_zero_vector(NEWCOL,NEWROW,-100,100,-100,100);

            for (int i=0;i<z2vector->size()-1;i++)
            {
                (*z2vector)[i].x = mas[i].x;
                (*z2vector)[i].y = mas[i].y;
            }
            delete mas;
            sp_plot->draw_spectr(*z2vector,get_zstart()+control->ui->spectr_shift_lineedit->text().toDouble(),get_zstop()+control->ui->spectr_shift_lineedit->text().toDouble(),get_xstart(),get_xstop(),ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
            z2vector->startx = get_zstart()+control->ui->spectr_shift_lineedit->text().toDouble();
            z2vector->stopx = get_zstop()+control->ui->spectr_shift_lineedit->text().toDouble();
            z2vector->starty = get_xstart();
            z2vector->stopy = get_xstop();

           double scale = max(fabs(get_xstart()-get_xstop()),fabs(get_zstart()-get_zstop()));
           z2vector->scale = scale;

           sp_plot->setAxisScale(QwtPlot::yLeft,-scale/2,scale/2);
           sp_plot->setAxisScale(QwtPlot::xBottom,-scale/2,scale/2);
           if (zoomer_mod)
           {
               sp_plot->setAxisScale(QwtPlot::yLeft,sp_plot->trans_y(sp_plot->point_for_zoomer1.y()),sp_plot->trans_y(sp_plot->point_for_zoomer2.y()));
               sp_plot->setAxisScale(QwtPlot::xBottom,sp_plot->trans_x(sp_plot->point_for_zoomer1.x()),sp_plot->trans_x(sp_plot->point_for_zoomer2.x()));
           }
           sp_plot->replot();

            ui->progressBar->setValue(100);
            sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toDouble(),control->ui->y_shift_lineEdit->text().toDouble(),control->ui->x_scale_lineEdit->text().toDouble(),control->ui->y_scale_lineEdit->text().toDouble(),
                                                control->ui->fi_lineEdit->text().toDouble(),control->ui->horizontalSlider_for_cuda_test->value(),control->ui->k_lineEdit->text().toDouble());

        }
        catch(...) {
            printf ("RENDER ERROR\n");
            bInProgress = false;
            return;
        }
        bInProgress = false;
    }
    else {
//        ui->progressBar->setValue(50);

//        if(bInProgress) return;
//        bInProgress = true;
//        printf ("count enter %d \n", count);
//        int NEWCOL=2048;
//        int NEWROW=1024;
//        float_complex *mas=new float_complex[NEWCOL*NEWROW];
//        float dAzStart = (float)count - control->ui->Az_span_lineEdit->text().toDouble()/2;
//        float dAzStop = (float)count + control->ui->Az_span_lineEdit->text().toDouble()/2;
//        try{
//            bool b = CalcC2C(NEWCOL,NEWROW,GLOBAL_F_start,GLOBAL_F_stop,dAzStart,dAzStop,(float*)mas);
//        }
//        catch(...) {
//            printf ("CUDA ERROR\n");
//            bInProgress = false;
//            return;
//        }
//        printf ("count proceed\n");
//        try{
//            //test end
//            z2vector->resize(NEWROW*NEWCOL);
//            printf ("count  %d \n", count);
//            z2vector->set_zero_vector(NEWCOL,NEWROW,-100,100,-100,100);

//            for (int i=0;i<z2vector->size()-1;i++)
//            {
//                (*z2vector)[i].x = mas[i].x;
//                (*z2vector)[i].y = mas[i].y;
//            }

//            delete mas;

//            sp_plot->draw_spectr(*z2vector,f_get_zstart()+control->ui->spectr_shift_lineedit->text().toDouble(),f_get_zstop()+control->ui->spectr_shift_lineedit->text().toDouble(),f_get_xstart(),f_get_xstop(),ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
//            ui->progressBar->setValue(100);
//        }
//        catch(...) {
//            printf ("RENDER ERROR\n");
//            bInProgress = false;
//            return;
//        }
//        printf ("count exit\n");
//        bInProgress = false;






        ui->progressBar->setValue(50);
        if(bInProgress) return;
        bInProgress = true;
        int NEWCOL=2048;
        int NEWROW=1024;
        float_complex *mas=new float_complex[NEWCOL*NEWROW];
        float dAzStart = (float)count - control->ui->Az_span_lineEdit->text().toFloat()/2;
        float dAzStop = (float)count + control->ui->Az_span_lineEdit->text().toFloat()/2;
        try{
            bool b = CalcC2C(NEWCOL,NEWROW,FLOAT_GLOBAL_F_start,FLOAT_GLOBAL_F_stop,(float)dAzStart,(float)dAzStop,(float*)mas);
        }
        catch(...) {
            printf ("cuda error occur 205 line spectwidget\n");
            bInProgress = false;
            delete mas;
            return;
        }

        try{
            //test end
            z2vector->resize(NEWROW*NEWCOL);

            z2vector->set_zero_vector(NEWCOL,NEWROW,-100,100,-100,100);

            for (int i=0;i<z2vector->size()-1;i++)
            {
                (*z2vector)[i].x = mas[i].x;
                (*z2vector)[i].y = mas[i].y;
            }
            delete mas;
            sp_plot->draw_spectr(*z2vector,f_get_zstart()+control->ui->spectr_shift_lineedit->text().toDouble(),f_get_zstop()+control->ui->spectr_shift_lineedit->text().toDouble(),f_get_xstart(),f_get_xstop(),ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
            z2vector->startx = f_get_zstart()+control->ui->spectr_shift_lineedit->text().toDouble();
            z2vector->stopx = f_get_zstop()+control->ui->spectr_shift_lineedit->text().toDouble();
            z2vector->starty = f_get_xstart();
            z2vector->stopy = f_get_xstop();

           float scale = max(fabs(f_get_xstart()-f_get_xstop()),fabs(f_get_zstart()-f_get_zstop()));
           z2vector->scale = scale;

           sp_plot->setAxisScale(QwtPlot::yLeft,-scale/2,scale/2);
           sp_plot->setAxisScale(QwtPlot::xBottom,-scale/2,scale/2);
           if (zoomer_mod)
           {
               sp_plot->setAxisScale(QwtPlot::yLeft,sp_plot->trans_y(sp_plot->point_for_zoomer1.y()),sp_plot->trans_y(sp_plot->point_for_zoomer2.y()));
               sp_plot->setAxisScale(QwtPlot::xBottom,sp_plot->trans_x(sp_plot->point_for_zoomer1.x()),sp_plot->trans_x(sp_plot->point_for_zoomer2.x()));
           }
           sp_plot->replot();

            ui->progressBar->setValue(100);
            sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toFloat(),control->ui->y_shift_lineEdit->text().toFloat(),control->ui->x_scale_lineEdit->text().toFloat(),control->ui->y_scale_lineEdit->text().toFloat(),
                                                control->ui->fi_lineEdit->text().toFloat(),control->ui->horizontalSlider_for_cuda_test->value(),control->ui->k_lineEdit->text().toFloat());

        }
        catch(...) {
            printf ("RENDER ERROR\n");
            bInProgress = false;
            return;
        }
        bInProgress = false;
    }
    emit set_gate_marker(count-control->ui->Az_span_lineEdit->text().toDouble()/2,count+control->ui->Az_span_lineEdit->text().toDouble()/2);
}
void SpectrWidget::on_type_combobox_currentIndexChanged(const QString &arg1)
{
    if (arg1=="Ampl")
    {
        sp_plot->etype=Spectro_Plot::Ampl;
        sp_plot->draw_spectr(*z2vector,z2vector->startx,z2vector->stopx,z2vector->starty,z2vector->stopy,ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
        sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->replot();
        if (zoomer_mod)
            sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
    }
    if (arg1=="Phase")
    {
        sp_plot->etype=Spectro_Plot::Phase;
        sp_plot->draw_spectr(*z2vector,z2vector->startx,z2vector->stopx,z2vector->starty,z2vector->stopy,ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
        sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->replot();
        if (zoomer_mod)
            sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
    }
    if (arg1=="Re")
    {
        sp_plot->etype=Spectro_Plot::Re;
        sp_plot->draw_spectr(*z2vector,z2vector->startx,z2vector->stopx,z2vector->starty,z2vector->stopy,ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
        sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->replot();
        if (zoomer_mod)
            sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
    }
    if (arg1=="Im")
    {
        sp_plot->etype=Spectro_Plot::Im;
        sp_plot->draw_spectr(*z2vector,z2vector->startx,z2vector->stopx,z2vector->starty,z2vector->stopy,ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
        sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->replot();
        if (zoomer_mod)
            sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
    }


}

void SpectrWidget::on_max_spinbox_valueChanged(int arg1)
{
    sp_plot->draw_spectr(*z2vector,z2vector->startx,z2vector->stopx,z2vector->starty,z2vector->stopy,ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
    sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
    sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
    sp_plot->replot();
    if (zoomer_mod)
        sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
}

void SpectrWidget::on_min_spinbox_valueChanged(double arg1)
{
    sp_plot->draw_spectr(*z2vector,z2vector->startx,z2vector->stopx,z2vector->starty,z2vector->stopy,ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
    sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
    sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
    sp_plot->replot();
    if (zoomer_mod)
        sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
}

void SpectrWidget::on_rect_pushButton_clicked()
{
    sp_plot->draw_widget->eType=Spectro_draw::Rect;
    sp_plot->draw_widget->is_new_item=true;
}

void SpectrWidget::on_polygon_pushButton_clicked()
{
    sp_plot->draw_widget->eType=Spectro_draw::Polygon;
    sp_plot->draw_widget->is_new_item=true;
}

void SpectrWidget::on_spec_type_combobox_currentIndexChanged(const QString &arg1)
{
    if (arg1=="Transparent")
    {
        sp_plot->is_transparent=true;
        sp_plot->draw_spectr(*z2vector,z2vector->startx,z2vector->stopx,z2vector->starty,z2vector->stopy,ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
        sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->replot();
        if (zoomer_mod)
            sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
    }
    if (arg1=="Normal")
    {
        sp_plot->is_transparent=false;
        sp_plot->draw_spectr(*z2vector,z2vector->startx,z2vector->stopx,z2vector->starty,z2vector->stopy,ui->max_spinbox->text().toDouble(),ui->min_spinbox->text().toDouble());
        sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->replot();
        if (zoomer_mod)
            sp_plot->slot_for_zoomer(sp_plot->point_for_zoomer1,sp_plot->point_for_zoomer1);
    }
}

void SpectrWidget::on_select_point_pushButton_clicked()
{


}

void SpectrWidget::on_line_pushButton_clicked()
{
    sp_plot->draw_widget->eType=Spectro_draw::Line;
    sp_plot->draw_widget->is_new_item=true;
}

void SpectrWidget::on_color_comboBox_currentIndexChanged(const QString &arg1)
{
    if (arg1=="black")
    {

        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::black));
    }
    if (arg1=="cyan")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::cyan));
    }
    if (arg1=="darkCyan")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::darkCyan));
    }
    if (arg1=="red")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::red));
    }
    if (arg1=="darkRed")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::darkRed));
    }
    if (arg1=="magenta")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::magenta));
    }
    if (arg1=="darkMagenta")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::darkMagenta));
    }
    if (arg1=="green")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::green));
    }
    if (arg1=="darkGreen")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::darkGreen));
    }
    if (arg1=="yellow")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::yellow));
    }
    if (arg1=="darkYellow")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::darkYellow));
    }
    if (arg1=="blue")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::blue));
    }
    if (arg1=="darkBlue")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::darkBlue));
    }
    if (arg1=="gray")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::gray));
    }
    if (arg1=="darkGray")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::darkGray));
    }
    if (arg1=="lightGray")
    {
        sp_plot->draw_widget->change_color(sp_plot->draw_widget->getRGB(Spectro_draw::lightGray));
    }
    if (arg1=="black")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::black;
    }
    if (arg1=="cyan")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::cyan;
    }
    if (arg1=="darkCyan")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::darkCyan;
    }
    if (arg1=="red")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::red;
    }
    if (arg1=="darkRed")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::darkRed;
    }
    if (arg1=="magenta")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::magenta;
    }
    if (arg1=="darkMagenta")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::darkMagenta;
    }
    if (arg1=="green")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::green;
    }
    if (arg1=="darkGreen")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::darkGreen;
    }
    if (arg1=="yellow")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::yellow;
    }
    if (arg1=="darkYellow")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::darkYellow;
    }
    if (arg1=="blue")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::blue;
    }
    if (arg1=="darkBlue")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::darkBlue;
    }
    if (arg1=="gray")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::gray;
    }
    if (arg1=="darkGray")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::darkGray;
    }
    if (arg1=="lightGray")
    {
        sp_plot->draw_widget->eColor=Spectro_draw::lightGray;
    }
    sp_plot->draw_widget->repaint();
}

void SpectrWidget::on_rect_pushButton_toggled(bool checked)
{
    if (checked==true)
    {
        sp_plot->draw_widget->eType=Spectro_draw::Rect;

        ui->line_pushButton->setChecked(false);


        ui->polygon_pushButton->setChecked(false);
    }
}

void SpectrWidget::on_polygon_pushButton_toggled(bool checked)
{
    if (checked==true)
    {
        sp_plot->draw_widget->eType=Spectro_draw::Polygon;
        ui->line_pushButton->setChecked(false);
        ui->rect_pushButton->setChecked(false);
    }
}

void SpectrWidget::on_circle_pushButton_toggled(bool checked)
{
    if (checked==true)
    {
        sp_plot->draw_widget->eType=Spectro_draw::Circle;

        ui->line_pushButton->setChecked(false);
        ui->rect_pushButton->setChecked(false);
        ui->polygon_pushButton->setChecked(false);
    }
}

void SpectrWidget::on_line_pushButton_toggled(bool checked)
{
    if (checked==true)
    {
        sp_plot->draw_widget->eType=Spectro_draw::Line;

        ui->rect_pushButton->setChecked(false);

        ui->polygon_pushButton->setChecked(false);
    }
}



void SpectrWidget::on_delete_last_pushButton_clicked()
{
    sp_plot->draw_widget->delete_last();
}

void SpectrWidget::on_select_point_pushButton_toggled(bool checked)
{
    if (checked==true)
        sp_plot->draw_widget->is_select_mode_enable=true;
    else
        sp_plot->draw_widget->is_select_mode_enable=false;
}

void SpectrWidget::on_cont_read_pushButton_clicked()
{
    sp_plot->draw_widget->read_from_file();

}




void SpectrWidget::on_zoom_pushButton_toggled(bool checked)
{
    if (checked==true)
    {
        ui->select_point_pushButton->setChecked(false);
        ui->line_pushButton->setChecked(false);
        ui->rect_pushButton->setChecked(false);
        sp_plot->draw_widget->is_zoom_mode_enable=true;
        ui->polygon_pushButton->setChecked(false);
        //TODO ��� ������� enabled false

        ui->delete_sketch_pushButton->setEnabled(false);
        ui->delete_last_pushButton->setEnabled(false);
        ui->line_pushButton->setEnabled(false);
        ui->polygon_pushButton->setEnabled(false);
        ui->rect_pushButton->setEnabled(false);
        ui->select_point_pushButton->setEnabled(false);
       // ui->zoom_pushButton->setEnabled(false);
        zoomer_mod = true;

    }
    if (checked==false)
    {
        ui->delete_sketch_pushButton->setEnabled(true);
        ui->delete_last_pushButton->setEnabled(true);
        ui->line_pushButton->setEnabled(true);
        ui->polygon_pushButton->setEnabled(true);
        ui->rect_pushButton->setEnabled(true);
        ui->select_point_pushButton->setEnabled(true);
        ui->zoom_pushButton->setEnabled(true);

        sp_plot->draw_widget->is_zoom_mode_enable=false;
        zoomer_mod = false;
        sp_plot->setAxisScale(QwtPlot::yLeft,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->setAxisScale(QwtPlot::xBottom,-z2vector->scale/2,z2vector->scale/2);
        sp_plot->replot();


    }
}

void SpectrWidget::change_x_shift(double a)
{
    control->ui->x_shift_lineEdit->setText(QString::number(a));
}
void SpectrWidget::change_y_shift(double a)
{
    control->ui->y_shift_lineEdit->setText(QString::number(a));
}
void SpectrWidget::change_x_scale(double a)
{
    control->ui->x_scale_lineEdit->setText(QString::number(a));
}
void SpectrWidget::change_y_scale(double a)
{
    control->ui->y_scale_lineEdit->setText(QString::number(a));
}
void SpectrWidget::change_fi(double a)
{
    control->ui->fi_lineEdit->setText(QString::number(a));
}
void SpectrWidget::change_k(double a)
{
    control->ui->k_lineEdit->setText(QString::number(a));
}

void SpectrWidget::on_x_scale_lineEdit_textChanged(const QString &arg1)
{
    sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toDouble(),control->ui->y_shift_lineEdit->text().toDouble(),control->ui->x_scale_lineEdit->text().toDouble(),control->ui->y_scale_lineEdit->text().toDouble(),
                                        control->ui->fi_lineEdit->text().toDouble(),0.0,control->ui->k_lineEdit->text().toDouble());


}

void SpectrWidget::on_x_shift_lineEdit_textChanged(const QString &arg1)
{
    sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toDouble(),control->ui->y_shift_lineEdit->text().toDouble(),control->ui->x_scale_lineEdit->text().toDouble(),control->ui->y_scale_lineEdit->text().toDouble(),
                                        control->ui->fi_lineEdit->text().toDouble(),0.0,control->ui->k_lineEdit->text().toDouble());
}

void SpectrWidget::on_y_shift_lineEdit_textChanged(const QString &arg1)
{
    sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toDouble(),control->ui->y_shift_lineEdit->text().toDouble(),control->ui->x_scale_lineEdit->text().toDouble(),control->ui->y_scale_lineEdit->text().toDouble(),
                                        control->ui->fi_lineEdit->text().toDouble(),0.0,control->ui->k_lineEdit->text().toDouble());
}

void SpectrWidget::on_y_scale_lineEdit_textChanged(const QString &arg1)
{
    sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toDouble(),control->ui->y_shift_lineEdit->text().toDouble(),control->ui->x_scale_lineEdit->text().toDouble(),control->ui->y_scale_lineEdit->text().toDouble(),
                                        control->ui->fi_lineEdit->text().toDouble(),0.0,control->ui->k_lineEdit->text().toDouble());
}

void SpectrWidget::on_delete_sketch_pushButton_clicked()
{
    if (sp_plot->draw_widget->sketch_list.size()!=0)
    {
        sp_plot->draw_widget->sketch_list.clear();
        sp_plot->draw_widget->repaint();
    }
}

void SpectrWidget::on_fi_shift_lineEdit_textChanged(const QString &arg1)
{
    sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toDouble(),control->ui->y_shift_lineEdit->text().toDouble(),control->ui->x_scale_lineEdit->text().toDouble(),control->ui->y_scale_lineEdit->text().toDouble(),
                                        control->ui->fi_lineEdit->text().toDouble(),0.0,control->ui->k_lineEdit->text().toDouble());
}


void SpectrWidget::on_fi_lineEdit_textChanged(const QString &arg1)
{
    sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toDouble(),control->ui->y_shift_lineEdit->text().toDouble(),control->ui->x_scale_lineEdit->text().toDouble(),control->ui->y_scale_lineEdit->text().toDouble(),
                                        control->ui->fi_lineEdit->text().toDouble(),0.0,control->ui->k_lineEdit->text().toDouble());
}



void SpectrWidget::on_flip_checkBox_toggled(bool checked)
{
    sp_plot->draw_widget->flip();
}

void SpectrWidget::on_sketch_checkBox_toggled(bool checked)
{
    if (checked==true)
    {
        sp_plot->draw_widget->is_sketch_mode_enable=true;
    }
    if (checked==false)
        sp_plot->draw_widget->is_sketch_mode_enable=false;
}

void SpectrWidget::on_scale_checkBox_toggled(bool checked)
{
    if (checked==true)
    {
        sp_plot->draw_widget->is_scale_mode_enable=true;

    }
    if (checked==false)
        sp_plot->draw_widget->is_scale_mode_enable=false;
}

void SpectrWidget::on_fi_shift_slider_sliderMoved(int position)
{

    sp_plot->draw_widget->replot_sketch(control->ui->x_shift_lineEdit->text().toDouble(),control->ui->y_shift_lineEdit->text().toDouble(),control->ui->x_scale_lineEdit->text().toDouble(),control->ui->y_scale_lineEdit->text().toDouble(),
                                        control->ui->fi_lineEdit->text().toDouble(),0.0,control->ui->k_lineEdit->text().toDouble());
}

void SpectrWidget::slot_to_change_scale_scale1(double a)
{
    control->ui->x_y_scale_lineedit1->setText(QString::number(a));
}
void SpectrWidget::slot_to_change_scale_scale2(double a)
{
    control->ui->x_y_scale_lineedit2->setText(QString::number(a));
}
void SpectrWidget::slot_to_change_scale_angle1(double a,double b)
{
    double x=360*atan2(b,a)/(2*3.1415926);
    control->ui->angle_lineedit1->setText(QString::number(x));
}
void SpectrWidget::slot_to_change_scale_angle2(double a, double b)
{
    double x=360*atan2(b,a)/(2*3.1415926);
    control->ui->angle_lineedit2->setText(QString::number(x));
}
void SpectrWidget::slot_to_change_scale_point1(QPointF point)
{
    control->ui->x_shift_lineedit1->setText(QString::number(point.x()));
    control->ui->y_shift_lineedit1->setText(QString::number(point.y()));
}
void SpectrWidget::slot_to_change_scale_point22(QPointF point)
{
    control->ui->x_shift_lineedit2->setText(QString::number(point.x()));
    control->ui->y_shift_lineedit2->setText(QString::number(point.y()));
}


void SpectrWidget::on_x_y_scale_pushButton_clicked()
{
    sp_plot->draw_widget->is_scale_mode_scale1=true;
    sp_plot->draw_widget->is_scale_mode_angle1=false;
    sp_plot->draw_widget->is_scale_mode_point1=false;

    sp_plot->draw_widget->is_scale_mode_scale2=false;
    sp_plot->draw_widget->is_scale_mode_angle2=false;
    sp_plot->draw_widget->is_scale_mode_point2=false;

}

void SpectrWidget::on_angle_pushbutton_clicked()
{
    sp_plot->draw_widget->is_scale_mode_scale1=false;
    sp_plot->draw_widget->is_scale_mode_angle1=true;
    sp_plot->draw_widget->is_scale_mode_point1=false;

    sp_plot->draw_widget->is_scale_mode_scale2=false;
    sp_plot->draw_widget->is_scale_mode_angle2=false;
    sp_plot->draw_widget->is_scale_mode_point2=false;
}

void SpectrWidget::on_x_y_shitft_pushButton_clicked()
{
    sp_plot->draw_widget->is_scale_mode_scale1=false;
    sp_plot->draw_widget->is_scale_mode_angle1=false;
    sp_plot->draw_widget->is_scale_mode_point1=true;

    sp_plot->draw_widget->is_scale_mode_scale2=false;
    sp_plot->draw_widget->is_scale_mode_angle2=false;
    sp_plot->draw_widget->is_scale_mode_point2=false;
}

void SpectrWidget::on_ok_x_y_scale_pushButton_clicked()
{
    double a=control->ui->x_y_scale_lineedit1->text().toDouble()/control->ui->x_y_scale_lineedit2->text().toDouble();

    control->ui->x_scale_lineEdit->setText(QString::number(control->ui->x_scale_lineEdit->text().toDouble()*a));
    control->ui->y_scale_lineEdit->setText(QString::number(control->ui->y_scale_lineEdit->text().toDouble()*a));
    control->ui->x_y_scale_lineedit1->setText(control->ui->x_y_scale_lineedit2->text());
}

void SpectrWidget::on_ok_angle_pushbutton_clicked()
{
    if (control->ui->angle_lineedit2->text()=="")
    {

        control->ui->fi_lineEdit->setText(QString::number(control->ui->fi_lineEdit->text().toDouble()+control->ui->angle_lineedit1->text().toDouble()));
    }
    else
    {
        double a=control->ui->angle_lineedit2->text().toDouble()-control->ui->angle_lineedit1->text().toDouble();
        control->ui->angle_lineedit1->setText(control->ui->angle_lineedit2->text());
        control->ui->fi_lineEdit->setText(QString::number(control->ui->fi_lineEdit->text().toDouble()+a));
    }

}

void SpectrWidget::on_ok_x_y_shift_pushButton_clicked()
{
    double a=control->ui->x_shift_lineedit1->text().toDouble()-control->ui->x_shift_lineedit2->text().toDouble();

    double b=control->ui->y_shift_lineedit1->text().toDouble()-control->ui->y_shift_lineedit2->text().toDouble();
    control->ui->x_shift_lineedit1->setText(control->ui->x_shift_lineedit2->text());
    control->ui->y_shift_lineedit1->setText(control->ui->y_shift_lineedit2->text());
    control->ui->x_shift_lineEdit->setText(QString::number(a));
    control->ui->y_shift_lineEdit->setText(QString::number(b));
}

void SpectrWidget::on_x_y_scale_pushButton_2_clicked()
{
    sp_plot->draw_widget->is_scale_mode_scale1=false;
    sp_plot->draw_widget->is_scale_mode_angle1=false;
    sp_plot->draw_widget->is_scale_mode_point1=false;

    sp_plot->draw_widget->is_scale_mode_scale2=true;
    sp_plot->draw_widget->is_scale_mode_angle2=false;
    sp_plot->draw_widget->is_scale_mode_point2=false;
}

void SpectrWidget::on_angle_pushbutton_2_clicked()
{
    sp_plot->draw_widget->is_scale_mode_scale1=false;
    sp_plot->draw_widget->is_scale_mode_angle1=false;
    sp_plot->draw_widget->is_scale_mode_point1=false;

    sp_plot->draw_widget->is_scale_mode_scale2=false;
    sp_plot->draw_widget->is_scale_mode_angle2=true;
    sp_plot->draw_widget->is_scale_mode_point2=false;
}

void SpectrWidget::on_x_y_shitft_pushButton_2_clicked()
{
    sp_plot->draw_widget->is_scale_mode_scale1=false;
    sp_plot->draw_widget->is_scale_mode_angle1=false;
    sp_plot->draw_widget->is_scale_mode_point1=false;

    sp_plot->draw_widget->is_scale_mode_scale2=false;
    sp_plot->draw_widget->is_scale_mode_angle2=false;
    sp_plot->draw_widget->is_scale_mode_point2=true;
}

void SpectrWidget::resizeEvent ( QResizeEvent * event )
{
    sp_plot->resize(this->width()-5,this->height()-60);
    sp_plot->draw_widget->resize(this->width()-40,this->height()-80);
    QWidget::resizeEvent(event);
}

void SpectrWidget::get_control(Form_Spectr* control1)
{
    control = control1;
}
void SpectrWidget::signal_on_pushButton_clicked()
{


    zoomer_mod = false;
    if (z2vector!=NULL)
        delete z2vector;
    z2vector = new Z2Vector();
   // sp_plot=new Spectro_Plot(this);
    if (data!=NULL)
        delete data;
    data=NULL;
    ui->progressBar->setValue(0);
    on_read_pushbutton_clicked();

}
void SpectrWidget::export_spectr_pushbutton()
{
#ifndef QT_NO_PRINTER
        QString fileName = "plot.pdf";
#else
        QString fileName = "plot.png";
#endif

#ifndef QT_NO_FILEDIALOG
        const QList<QByteArray> imageFormats =
                QImageWriter::supportedImageFormats();

        QStringList filter;
        filter += "PDF Documents (*.pdf)";
#ifndef QWT_NO_SVG
        filter += "SVG Documents (*.svg)";
#endif
        filter += "Postscript Documents (*.ps)";

        if ( imageFormats.size() > 0 )
        {
            QString imageFilter("Images (");
            for ( int i = 0; i < imageFormats.size(); i++ )
            {
                if ( i > 0 )
                    imageFilter += " ";
                imageFilter += "*.";
                imageFilter += imageFormats[i];
            }
            imageFilter += ")";

            filter += imageFilter;
        }

        fileName = QFileDialog::getSaveFileName(
                    this, "Export File Name", fileName,
                    filter.join(";;"), NULL, QFileDialog::DontConfirmOverwrite);
#endif
        if ( !fileName.isEmpty() )
        {
            QwtPlotRenderer renderer;
            // renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground, false);

            renderer.renderDocument(sp_plot, fileName, QSizeF(300, 200), 85);
            QMessageBox::information(0,"information","Operation Complete");
        }

}
double_complex SpectrWidget::MakeComplex(double x,double y)
{
    double_complex c;
    c.x = x;
    c.y = y;
    return c;
}
float_complex SpectrWidget::fMakeComplex(float x,float y)
{
    float_complex c;
    c.x = x;
    c.y = y;
    return c;
}



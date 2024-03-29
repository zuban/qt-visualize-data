#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "spectrwidget.h"
#include "form_spectr.h"



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->showMaximized();
    Form_Spectr *contr;
    contr= ui->widget_spectr_contr;
    connect(this,SIGNAL(get_control(Form_Spectr*)),ui->widget_spectr,SLOT(get_control(Form_Spectr*)));
    emit get_control(ui->widget_spectr_contr);


    connect(ui->widget_contr_2D,SIGNAL(signal_on_add_pushButton_clicked()),ui->widget_2D,SLOT(slot_on_add_pushButton_clicked()));
    connect(ui->widget_contr_2D,SIGNAL(signal_on_export_pushButton_clicked()),ui->widget_2D,SLOT(slot_on_export_pushButton_clicked()));
    connect(ui->widget_contr_2D,SIGNAL(signal_on_test_cuda_pushButton_clicked(int,int)),ui->widget_2D,SLOT(slot_on_test_cuda_pushButton_clicked(int,int)));

    connect(ui->widget_spectr_contr,SIGNAL(signal_on_read_pushbutton_clicked()),ui->widget_spectr,SLOT(on_read_pushbutton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_cont_read_pushButton_clicked()),ui->widget_spectr,SLOT(on_cont_read_pushButton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_x_y_scale_pushButton_clicked()),ui->widget_spectr,SLOT(on_x_y_scale_pushButton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_ok_x_y_scale_pushButton_clicked()),ui->widget_spectr,SLOT(on_ok_x_y_scale_pushButton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_x_y_scale_pushButton_2_clicked()),ui->widget_spectr,SLOT(on_x_y_scale_pushButton_2_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_scale_checkBox_toggled(bool)),ui->widget_spectr,SLOT(on_scale_checkBox_toggled(bool)));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_flip_checkBox_toggled(bool)),ui->widget_spectr,SLOT(on_flip_checkBox_toggled(bool)));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_angle_pushbutton_clicked()),ui->widget_spectr,SLOT(on_angle_pushbutton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_ok_angle_pushbutton_clicked()),ui->widget_spectr,SLOT(on_ok_angle_pushbutton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_angle_pushbutton_2_clicked()),ui->widget_spectr,SLOT(on_angle_pushbutton_2_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_x_y_shitft_pushButton_clicked()),ui->widget_spectr,SLOT(on_x_y_shitft_pushButton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_ok_x_y_shift_pushButton_clicked()),ui->widget_spectr,SLOT(on_ok_x_y_shift_pushButton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_x_y_shitft_pushButton_2_clicked()),ui->widget_spectr,SLOT(on_x_y_shitft_pushButton_2_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_x_shift_lineEdit_textChanged(QString)),ui->widget_spectr,SLOT(on_x_shift_lineEdit_textChanged(QString)));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_fi_lineEdit_textChanged(QString)),ui->widget_spectr,SLOT(on_fi_lineEdit_textChanged(QString)));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_y_shift_lineEdit_textChanged(QString)),ui->widget_spectr,SLOT(on_y_shift_lineEdit_textChanged(QString)));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_x_scale_lineEdit_textChanged(QString)),ui->widget_spectr,SLOT(on_x_scale_lineEdit_textChanged(QString)));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_y_scale_lineEdit_textChanged(QString)),ui->widget_spectr,SLOT(on_y_scale_lineEdit_textChanged(QString)));

    connect(ui->widget_spectr_contr,SIGNAL(change_FI0_cuda_test(int)),ui->widget_spectr,SLOT(change_FI0_cuda_test(int)));
    connect(ui->widget_contr_2D,SIGNAL(enable_gate_marker(bool)),ui->widget_2D->zGraph,SLOT(enable_gate_marker(bool)));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_export_spectr_pushbutton_clicked()),ui->widget_spectr,SLOT(export_spectr_pushbutton()));
    connect(ui->widget_spectr,SIGNAL(set_gate_marker(double,double)),ui->widget_2D->zGraph,SLOT(set_gate_marker(double,double)));
    connect(ui->widget_spectr,SIGNAL(set_data_2D(QString)),ui->widget_2D->zGraph,SLOT(get_data_2D(QString)));
    connect(ui->widget_contr_2D,SIGNAL(signal_on_copy_pushButton_clicked()),ui->widget_2D->zGraph,SLOT(signal_on_copy_pushButton_clicked()));
    connect(ui->widget_contr_2D,SIGNAL(signal_on_clear_pushButton_clicked()),ui->widget_2D->zGraph,SLOT(signal_on_clear_pushButton_clicked()));
    connect(ui->widget_spectr_contr,SIGNAL(signal_on_pushButton_clicked()),ui->widget_spectr,SLOT(signal_on_pushButton_clicked()));
    connect(ui->widget_2D->zGraph,SIGNAL(replot_2d()),ui->widget_contr_2D,SLOT(replot_2d()));

    connect(ui->widget_2D->zGraph,SIGNAL(change_value_freq(double)),ui->widget_contr_2D,SLOT(change_value_freq(double)));
    connect(ui->widget_2D->zGraph,SIGNAL(change_value_angle(double)),ui->widget_contr_2D,SLOT(change_value_angle(double)));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closing(){
    QCoreApplication::quit();
}




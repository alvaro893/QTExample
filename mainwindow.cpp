#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_thread.startPort("/dev/ttyACM0", 10);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [=](){
        qInfo("HI");
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    timer->start(ui->timeEdit->text().toInt());
}

void MainWindow::on_latencySpinBox_valueChanged(int arg1)
{
    QString rateText = ui->rateEdit->text();

    uint8_t rate = rateText.toUInt();
    uint8_t latency = (uint8_t)arg1;

    unsigned char bytes [] {0xff, rate, latency};
    QByteArray barr( ((char*)bytes), 3);
    m_thread.squeduleWrite(barr, 10);
}

void MainWindow::scheduleData()
{
    QString rateText = ui->rateEdit->text();

    uint8_t rate = rateText.toUInt();
    uint8_t latency = (uint8_t)ui->latencySpinBox->value();

    unsigned char bytes [] {0xff, rate, latency};
    QByteArray barr( ((char*)bytes), 3);
    m_thread.squeduleWrite(barr, 10);
}

void MainWindow::on_stopbutton_clicked()
{
    timer->stop();
}

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QDateTime>
#include <QtDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
   // m_thread.startPort("/dev/ttyACM0", 10);

    connect(&timer, &QTimer::timeout, [=](){
        sendMessage(ui->horizontalSlider->value());
    });

    connect(&m_thread, &CommunicationThread::error, [=](const QString s){
        qInfo() << s;
    });

    connect(&m_thread, &CommunicationThread::timeout, [=](const QString s){
        qInfo() << s;
    });

    fillSerialPortList();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    timer.start(ui->timeEdit->text().toInt());

}

void MainWindow::sendMessage(int arg1)
{
    QString rateText = ui->rateEdit->text();

    uint8_t rate = rateText.toUInt();
    uint8_t latency = (uint8_t)arg1;

    unsigned char bytes [] {0xff, rate, latency};
    QByteArray barr( ((char*)bytes), 3);
    m_thread.squeduleWrite(barr, 15);
}


void MainWindow::on_stopbutton_clicked()
{
    timer.stop();
}


void MainWindow::fillSerialPortList()
{
    ui->serialPortComboBox->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    for(int i = 0; i < ports.length(); i++){
        ui->serialPortComboBox->addItem(ports.at(i).portName());
    }
    isReady = true;
}

void MainWindow::on_serialPortComboBox_currentIndexChanged(const QString &arg1)
{
    if(isReady){
         m_thread.startPort(arg1, 10);
    }
}

void MainWindow::on_horizontalSlider_sliderPressed()
{
    //timer.start(ui->timeEdit->text().toInt());
}

void MainWindow::on_horizontalSlider_sliderReleased()
{
    //timer.stop();
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
   sendMessage(value);
}

void MainWindow::on_radioButton_3_toggled(bool checked)
{

}

void MainWindow::on_udpRadioButton_toggled(bool checked)
{
    if(ui->udpRadioButton->isChecked()){
        qInfo("udp radio butt");
        m_thread.startUdp(ui->udpPortEdit->text().toInt(),
                          ui->ipEdit->text());
    }
}


void MainWindow::on_tcpRadioButton_toggled(bool checked)
{
    if(ui->tcpRadioButton->isChecked()){
        qInfo("tcp radio butt");
        m_thread.startTcp(ui->tcpPortEdit->text().toInt(),
                          ui->ipEdit->text());
    }
}

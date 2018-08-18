#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include "communicationthread.h"
#include <QTimer>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void sendMessage();

private slots:
    void on_serialPortComboBox_currentIndexChanged(const QString &arg1);

    void on_latencySlider_valueChanged(int value);

    void on_udpRadioButton_toggled(bool checked);

    void on_tcpRadioButton_toggled(bool checked);

    void on_ipEdit_textChanged(const QString &arg1);

    void on_serialRadioButton_toggled(bool checked);

    void update_roll(double roll);

    void on_rateSlider_valueChanged(int value);

private:
    QSettings settings;
    bool isReady = false;
    QTimer timer;
    CommunicationThread m_thread;
    Ui::MainWindow *ui;
    void fillSerialPortList();
};

#endif // MAINWINDOW_H

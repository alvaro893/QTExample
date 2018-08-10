#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
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

private slots:
    void on_pushButton_clicked();
    void on_latencySpinBox_valueChanged(int arg1);
    void scheduleData();

    void on_stopbutton_clicked();

private:
    QTimer *timer;
    CommunicationThread m_thread;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

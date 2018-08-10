#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QSerialPort>
#include <QQueue>

class CommunicationThread : public QThread
{
    Q_OBJECT

public:
    explicit CommunicationThread(QObject *parent = nullptr);
    ~CommunicationThread() ;

    void startPort(const QString &portName, int waitTimeout);
    void squeduleWrite(QByteArray &arr, long delay);
    void close();

signals:
    void error(const QString &s);
    void timeout(const QString &s);

private:
    const static int IN_BUFFER_SIZE = 50;
    void run() override;


    QString m_portName;
    QString m_response;
    int m_waitTimeout = 0;
    QMutex m_mutex;
    bool m_quit = false;
    char in_buffer[IN_BUFFER_SIZE];
    long m_write_rate;
    QQueue<QByteArray> m_queue;
    QQueue<long> m_delay_queue;
};

#endif // COMMUNICATIONTHREAD_H

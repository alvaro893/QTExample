#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QQueue>
#include <QTimer>
#include <QTime>
#include <QAbstractSocket>


class CommunicationThread : public QThread
{
    Q_OBJECT

public:
    explicit CommunicationThread(QObject *parent = nullptr);
    ~CommunicationThread() ;

    void startPort(const QString &portName, int waitTimeout);
    void startUdp(const int port, const QString &ip);
    void startTcp(const int port, const QString &ip);
    void squeduleWrite(QByteArray &arr, long delay);
    void close();

signals:
    void error(const QString &s);
    void timeout(const QString &s);

private:
    const static int IN_BUFFER_SIZE = 50;
    const static int DEFAULT_TIMEOUT = 10;
    void run() override;
    bool shouldClosePort;
    inline quint64 millis() const {return QDateTime::currentMSecsSinceEpoch();}
    bool startSocket(QAbstractSocket &socket, const QString &ip, const quint16 &port);
    void startDeviceLoop(QIODevice &device);

    enum Protocol {SERIAL, UDP, TCP, NONE};
    Protocol m_current_protocol;
    quint16 m_udp_port;
    quint16 m_tcp_port;
    QString m_ip;
    
    QString m_portName;
    QString m_response;
    int m_waitTimeout = 0;
    QMutex m_mutex;
    bool m_quit = false;
    char in_buffer[IN_BUFFER_SIZE];
    long m_write_rate;
    QQueue<QByteArray> m_queue;
    QQueue<quint64> m_delay_queue;
};

#endif // COMMUNICATIONTHREAD_H

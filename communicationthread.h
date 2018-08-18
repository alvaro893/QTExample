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
    double getData(quint8 id);
signals:
    void error(const QString &s);
    void roll(double roll);
    void pitch(double pitch);
    void yaw(double yaw);

private:

    void run() override;
    bool shouldClosePort;
    inline quint64 millis() const {return QDateTime::currentMSecsSinceEpoch();}
    bool startSocket(QAbstractSocket &socket, const QString &ip, const quint16 &port);
    void startDeviceLoop(QIODevice &device);
    inline void parse(quint8 byteIn);

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

    long m_write_rate;
    QQueue<QByteArray> m_queue;
    QQueue<quint64> m_delay_queue;

    // protocol vars
    quint8 StartCounter = 0;
    quint8 ByteCounter = 0;
    quint8 StartOfLine = 0;
    quint8 DataFieldCounter = 2;
    quint8 DataIn[256*3];
    quint16 Data[256];
    quint8 deviceId = 1;
    quint8 deviceType = 1;
    quint8 messageCounter = 0;
    quint8 isEndOfLine = false;
    quint8 oldCounter = 0;
    quint16 ChecksumCounter = 0;
};

#endif // COMMUNICATIONTHREAD_H

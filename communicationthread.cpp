#include <QHostInfo>
#include <QTcpSocket>
#include <QtDebug>

#include "communicationthread.h"
#include <QUdpSocket>


CommunicationThread::CommunicationThread(QObject *parent) : QThread(parent){
    shouldClosePort = false;
    m_current_protocol = NONE;
}

CommunicationThread::~CommunicationThread()
{
    m_mutex.lock();
    m_quit = true;
    m_mutex.unlock();
    wait();
}

void CommunicationThread::startPort(const QString &portName, int waitTimeout)
{
    // lock the mutex to protect the fields
    const QMutexLocker locker(&m_mutex);
    m_queue.clear();
    m_delay_queue.clear();
    m_current_protocol = SERIAL;

    //m_waitTimeout = waitTimeout;
    m_portName = portName;
    shouldClosePort = true;
    if (!isRunning()){
        start(QThread::HighPriority);
        qInfo("starting thread for port %s", portName.toUtf8().constData());
    }

//    QTimer::singleShot(1000, this, [=](){
    //    });
}

void CommunicationThread::startUdp(const int port, const QString &ip)
{
    const QMutexLocker locker(&m_mutex);
    m_queue.clear();
    m_delay_queue.clear();
    m_current_protocol = UDP;
    shouldClosePort = true;
    m_udp_port = port;
    m_ip = ip;
    if (!isRunning()){
        start(QThread::HighPriority);
        qInfo("starting thread for udp port %d", port);
    }
}

void CommunicationThread::startTcp(const int port, const QString &ip)
{
    const QMutexLocker locker(&m_mutex);
    m_queue.clear();
    m_delay_queue.clear();
    m_current_protocol = TCP;
    shouldClosePort = true;
    m_tcp_port = port;
    m_ip = ip;
    if (!isRunning()){
        start(QThread::HighPriority);
        qInfo("starting thread for tcp port %d", port);
    }
}

void CommunicationThread::squeduleWrite(QByteArray &arr, long delay)
{
    const QMutexLocker locker(&m_mutex);
    m_queue.enqueue(arr);
    m_delay_queue.enqueue(delay);
}

void CommunicationThread::close()
{
    const QMutexLocker locker(&m_mutex);
    shouldClosePort = true;
}


/** every thing in here runs inside the Thread,
    all declared variables are on a different stack **/
void CommunicationThread::run()
{
    while(!m_quit){
        sleep(1); // to prevent running on the loop too fast, in case of error

        m_mutex.lock();
        //int currentWaitTimeout = m_waitTimeout;
        int currentProtocol = m_current_protocol;
        m_mutex.unlock();

        if (currentProtocol == SERIAL){
            // get port fields into this thread's stack. access must be protected            
            m_mutex.lock();
            QString currentPortName = m_portName;
            m_mutex.unlock();
            qInfo() << "creating QSerialPort instance with" << currentPortName.toUtf8();
            QSerialPort serial;
            serial.setPortName(currentPortName);
            serial.setBaudRate(QSerialPort::Baud115200);
            // opening port
            if(!serial.open(QIODevice::ReadWrite)){
                qInfo() << "could not open port";
                emit error(tr("Cannot open %1, error code %2")
                           .arg(currentPortName, serial.error()));
                sleep(1);
                continue;
            }
            startDeviceLoop(serial);
        }else if(currentProtocol == UDP){
            m_mutex.lock();
            QString ip = m_ip;
            quint16 port = m_udp_port;
            m_mutex.unlock();
            QUdpSocket udp;
            if(startSocket(udp, ip, port)){
                startDeviceLoop(udp);
            }
        }else if(currentProtocol == TCP){
            m_mutex.lock();
            QString ip = m_ip;
            quint16 port = m_tcp_port;
            m_mutex.unlock();

            QTcpSocket tcp;
            if(startSocket(tcp, ip, port)){
                startDeviceLoop(tcp);
            }

        }else{
            continue;
        }

    } // main while
}

bool CommunicationThread::startSocket(QAbstractSocket &socket, const QString &ip, const quint16 &port)
{
    qInfo() << "creating socket instance with" << ip << ":" << port;
    QHostAddress addr(ip);
    // if a hostname was given, look up for its ip addresses
    if(addr.isNull()){
        qInfo() <<"looking up for" << ip << "name...";
        QHostInfo info = QHostInfo::fromName(ip);
        if(info.addresses().isEmpty()) return false; // continue the loop
        qInfo() << info.addresses();
        // get the first ipv4 address
        for(int i=0; i<info.addresses().length(); i++){
            QHostAddress addr = info.addresses()[i];
            if(addr.protocol() == QAbstractSocket::IPv4Protocol){
                socket.connectToHost(addr, port);
                break;
            }
        }

    }else{
        socket.connectToHost(addr, port);
    }
    //socket.write(QByteArray("Hello"));
    qInfo() << "remote port:" << socket.localPort();
    return true;
}

void CommunicationThread::startDeviceLoop(QIODevice &device)
{
    quint64 writeTimer = 0;
    quint64 queueDelay;
    QByteArray barr;
    bool shouldDequeue = true;

    shouldClosePort = false;
    sleep(1);

    while(!shouldClosePort && !m_quit){

        //QByteArray data;
        if(device.waitForReadyRead(DEFAULT_TIMEOUT)){
            //data = device.readAll();
            if(device.read(in_buffer, IN_BUFFER_SIZE)){ qDebug() << in_buffer; }
        }else{
           // qDebug() << "Timeout read..." << QTime::currentTime().toString();
            //emit timeout(tr("Wait read request timeout %1").arg(QTime::currentTime().toString()));
        }

        //deque until the queue is empty and if the flag allows it
        if(!m_queue.isEmpty() && shouldDequeue){
            // get data from queue
            m_mutex.lock();
            barr = m_queue.dequeue();
            queueDelay = m_delay_queue.dequeue();
            m_mutex.unlock();
            shouldDequeue = false;
        }

        // write to device AFTER selected delay
        if(writeTimer - millis() > queueDelay && !shouldDequeue){
            device.write(barr);
            if(device.waitForBytesWritten(DEFAULT_TIMEOUT)){
                qDebug("wrote %d bytes with %lli delay", barr.size(), queueDelay);
            }

            writeTimer = millis();
            shouldDequeue = true;
        }

    } // read/write loop
    device.close();

}



#include "communicationthread.h"

#include <QTime>

CommunicationThread::CommunicationThread(QObject *parent) : QThread(parent){

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
    m_portName = portName;
    m_waitTimeout = waitTimeout;
    if (!isRunning())
        start();

}

void CommunicationThread::squeduleWrite(QByteArray &arr, long delay)
{
    const QMutexLocker locker(&m_mutex);
    m_queue.enqueue(arr);
    m_delay_queue.enqueue(delay);
}

void CommunicationThread::close()
{
    m_quit = true;
}

/** every thing in here runs inside the Thread,
    all declared variables are on a different stack **/
void CommunicationThread::run()
{
    bool currentPortNameChanged = false;
    // get port fields into this thread's stack. access must be protected
    m_mutex.lock();
    QString currentPortName;
    if (currentPortName != m_portName) {
        currentPortName = m_portName;
        currentPortNameChanged = true;
    }

    int currentWaitTimeout = m_waitTimeout;
    QString currentRespone = m_response;
    m_mutex.unlock();
    //

    QSerialPort serial;
    while(!m_quit){
        if(currentPortNameChanged){
            serial.close();
            serial.setPortName(currentPortName);
            serial.setBaudRate(QSerialPort::Baud115200);
            serial.setParity(QSerialPort::NoParity);
            serial.setDataBits(QSerialPort::Data8);
            serial.setStopBits(QSerialPort::OneStop);
            serial.setFlowControl(QSerialPort::NoFlowControl);

            if(!serial.open(QIODevice::ReadWrite)){
                emit error(tr("Cannot open %1, error code %2")
                           .arg(currentPortName, serial.error()));
                return;
            }else{
                serial.flush();
            }
        }

        // udp parent is also QIODevice
        QIODevice *device = &serial;

        while(device->isOpen() && !m_quit){
            //QByteArray data;
            if(device->waitForReadyRead(currentWaitTimeout)){
                //data = serial.readAll();
                 device->read(in_buffer, IN_BUFFER_SIZE);
                qInfo(in_buffer);
            // do something
            }else{
                emit timeout(tr("Wait read request timeout %1")
                             .arg(QTime::currentTime().toString()));
            }

            //wrote until the queue is empty
            if(!m_queue.isEmpty()){
                // get data from queue
                m_mutex.lock();
                QByteArray barr = m_queue.dequeue();
                long qdelay = m_delay_queue.dequeue();
                m_mutex.unlock();

                serial.write(barr);
                if(device->waitForBytesWritten(currentWaitTimeout)){
                    // todo
                    qInfo("wrote %d bytes with %lu delay", barr.size(), qdelay);
                }
            }

        }

    } // main while

    serial.close();


}

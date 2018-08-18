#include <QHostInfo>
#include <QTcpSocket>
#include <QtDebug>

#include "communicationthread.h"
#include <QUdpSocket>
#include <QtDebug>
#include <math.h>


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

/** thread safe */
double CommunicationThread::getData(quint8 id)
{
    const QMutexLocker locker(&m_mutex);
    double d  = floor( ((qint16)Data[id]) * 0.02197265625 * 100 ) / 100;
//    qInfo() << d;
    return d;
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
                //flush first
//                serial.flush();
//                serial.waitForBytesWritten(1000);
                //
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
    const int DEFAULT_TIMEOUT = 10;
    quint64 writeTimer = 0;
    quint64 queueDelay = 0;
    QByteArray barr;
    bool shouldDequeue = true;
    int i;


    shouldClosePort = false;
    sleep(1);

    while(!shouldClosePort && !m_quit){

        if(device.waitForReadyRead(DEFAULT_TIMEOUT)){
                QByteArray data = device.readAll();
                m_mutex.lock();
                for(i = 0; i < data.length(); i++){ parse(data[i]);}
                //qDebug() << data;
                //qInfo() << Data[1];

                m_mutex.unlock();

        }else{
           // qDebug() << "Timeout read..." << QTime::currentTime().toString();
        }

        //deque until the queue is empty and if the flag allows it
        m_mutex.lock();
        bool qEmpty = m_queue.isEmpty();
        m_mutex.unlock();
        if(!qEmpty && shouldDequeue){
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
            if(device.waitForBytesWritten(100)){
                qDebug("wrote %d bytes with %lli delay", barr.size(), queueDelay);
            }

            writeTimer = millis();
            shouldDequeue = true;
        }

        emit yaw(getData(3));
        emit pitch(getData(2));
        emit roll(getData(1));


    } // read/write loop
    device.close();

}


void CommunicationThread::parse(quint8 byteIn){
    //const QMutexLocker locker(&m_mutex);
  if (byteIn == 0xFF) {
      StartCounter++;  if (StartCounter == 3) {StartOfLine = 1; StartCounter = 0; ByteCounter = 0; return;}
    }

    if (byteIn != 0xFF) { StartCounter = 0; }

    /* starting sequence was detected*/
    if (StartOfLine == 1) {

      DataIn[ByteCounter] = byteIn;

      /* Header was saved. Now, we start counting for the data*/
      if(ByteCounter > 2){
        if (DataFieldCounter < 2) {
          DataFieldCounter++;
        } else {
         DataFieldCounter = 0; // whenever the data field counter is 0, the current byte is a data id.
//         Serial.println(byteIn); //prints all the data ids
        }
      }

      ByteCounter++;
    }

    /* when we detect data id = 0, we have reach the end of the message, the next to 2 bytes are the 16bit checksum*/
    if (DataFieldCounter == 0 && byteIn == 0 && ChecksumCounter >= 2) { // when data id is 0,
      isEndOfLine = true;
      ChecksumCounter = 0;
      return;
    }

    /* Once we reach the checksum, we count 2 times to get the 16 bit checksum*/
    if(ChecksumCounter < 2){
      ChecksumCounter++;
    }

    /* Now we got the whole message: both data and checksum */
    if (isEndOfLine && ChecksumCounter >= 2){
      int i;

      // calculte checksum
      quint16 calculated_checksum = 0;
      quint16 checksum = DataIn[ByteCounter-2] + DataIn[ByteCounter-1] *256; // checksum is on the last 2 bytes
      for (i = 0; i < ByteCounter-3; i++)  {calculated_checksum += DataIn[i]; }

      // proceed if checksum matches
          /* Header data */
          quint8 deviceId =   DataIn[0];
          quint8 deviceType = DataIn[1];
          quint8 messageCounter   = DataIn[2];

          /* Check order of messages */
          quint8 nextcounter = oldCounter + 1;
          oldCounter = messageCounter;


           /* Unpack data to device array, if applies */
            quint8  dataId;
            quint16 dataValue;
            for (i = 3; i < ByteCounter-2; i+=3) {
              dataId = DataIn[i];
              dataValue = (quint16)(DataIn[i + 1] + DataIn[i + 2] * 256);
              Data[dataId] = dataValue;

            }

         /* Debug printout */
        // Serial.print("DataIn:  ");for(i = 0; i < ByteCounter; i++) Serial.printf("%x-", DataIn[i]); Serial.println();
        // Serial.print("checksum was "); Serial.print(calculated_checksum, HEX); Serial.print(", but should "); Serial.println(checksum, HEX);
        // if(Data != NULL){
        //   Serial.print("Data:  ");
        //   for(i = 0; i < 10; i++) Serial.printf("%d-", Data[i]); Serial.println();
        // }

        /* reset variables for next message */
        StartOfLine = 0;
        ByteCounter = 0;
        StartCounter = 0;
        DataFieldCounter = 2;
        ChecksumCounter = 2;
        isEndOfLine = false;
      }


}




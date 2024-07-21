#include "tcpclient.h"

QDataStream &operator >>(QDataStream &out, ServiceHeader &data){

    out >> data.id;
    out >> data.idData;
    out >> data.status;
    out >> data.len;
    return out;
};
QDataStream &operator <<(QDataStream &in, ServiceHeader &data){

    in << data.id;
    in << data.idData;
    in << data.status;
    in << data.len;

    return in;
};

TCPclient::TCPclient(QObject *parent) : QObject(parent)
{

    socket = new QTcpSocket(this);

    connect(socket,&QTcpSocket::readyRead,this, &TCPclient::ReadyReed);
    connect(socket,&QTcpSocket::disconnected,this, &QTcpSocket::deleteLater);
    
}

/* write
 * Метод отправляет запрос на сервер. Сериализировать будем
 * при помощи QDataStream
*/
void TCPclient::SendRequest(ServiceHeader head)
{
    Data.clear();
    QDataStream out(&Data,QIODevice::WriteOnly );
    out.setVersion(QDataStream::Qt_6_2);
    out << head;
    socket->write(Data);

}

/* write
 * Такой же метод только передаем еще данные.
*/
void TCPclient::SendData(ServiceHeader head, QString str)
{
    Data.clear();
    QDataStream out(&Data,QIODevice::WriteOnly );
    out.setVersion(QDataStream::Qt_6_2);
    out << head;
    out << str;
    socket->write(Data);

}

/*
 * \brief Метод подключения к серверу
 */
void TCPclient::ConnectToHost(QHostAddress host, uint16_t port)
{
   socket->connectToHost(host,port);

   bool connectStatus = false;
   connectStatus = socket->isValid();
   emit sig_connectStatus(connectStatus);


}


void TCPclient::DisconnectFromHost()
{
    socket->disconnectFromHost();
    emit sig_Disconnected();

}


void TCPclient::ReadyReed()
{

    QDataStream incStream(socket);

    if(incStream.status() != QDataStream::Ok){
        QMessageBox msg;
        msg.setIcon(QMessageBox::Warning);
        msg.setText("Ошибка открытия входящего потока для чтения данных!");
        msg.exec();
    }


    //Читаем до конца потока
    while(incStream.atEnd() == false){
        //Если мы обработали предыдущий пакет, мы скинули значение idData в 0
        if(servHeader.idData == 0){

            //Проверяем количество полученных байт. Если доступных байт меньше чем
            //заголовок, то выходим из обработчика и ждем новую посылку. Каждая новая
            //посылка дописывает данные в конец буфера
            if(socket->bytesAvailable() < sizeof(ServiceHeader)){
                return;
            }
            else{
                //Читаем заголовок
                incStream >> servHeader;
                //Проверяем на корректность данных. Принимаем решение по заранее известному ID
                //пакета. Если он "битый" отбрасываем все данные в поисках нового ID.
                if(servHeader.id != ID){
                    uint16_t hdr = 0;
                    while(incStream.atEnd()){
                        incStream >> hdr;
                        if(hdr == ID){
                            incStream >> servHeader.idData;
                            incStream >> servHeader.status;
                            incStream >> servHeader.len;
                            break;
                        }
                    }

                }
            }
        }
        //Если получены не все данные, то выходим из обработчика. Ждем новую посылку
        if(socket->bytesAvailable() < servHeader.len){
            return;
        }
        else{
            //Обработка данных
            ProcessingData(servHeader, incStream);
            servHeader.idData = 0;
            servHeader.status = 0;
            servHeader.len = 0;
        }
    }
}


/*
 * Остался метод обработки полученных данных. Согласно протоколу
 * мы должны прочитать данные из сообщения и вывести их в ПИ.
 * Поскольку все типы сообщений нам известны реализуем выбор через
 * switch. Реализуем получение времени.
*/

void TCPclient::ProcessingData(ServiceHeader header, QDataStream &stream)
{

    int q = header.idData;

    QDateTime times;
    stream>>times;

    switch (header.idData){

        case GET_TIME:
        emit sig_sendTime(times);

        case GET_SIZE:

        //emit sig_sendFreeSize();

        case GET_STAT:

        //emit sig_sendStat();

        case SET_DATA:

        //emit sig_SendReplyForSetData();

        case CLEAR_DATA:
        default:
            return;

        }

}

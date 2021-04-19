#include "ServerWindow.h"
#include "ui_ServerWindow.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QtCore/QDebug>

// Конструктор класса
ServerWindow::ServerWindow(bool debug, quint16 port, QWidget* parent):
    QMainWindow(parent), currentPort(QString::number(port)), ui(new Ui::ServerWindow),
    connectionWebSocketServer(new QWebSocketServer(ServerWindow::SEARCH_CLIENT, QWebSocketServer::NonSecureMode, this))
{
    this->debug = debug;


    this->toConsole("Запуск графической оболочки.");

    this->ui->setupUi(this);
    connect(this->ui->usersListWidget, &QListWidget::itemClicked, this, &ServerWindow::eventUsersListWidgetItemClicked);

    this->toConsole("Проверка инициализации веб-сокета сервера...");

    // Если веб-сокет сервера слушает, то связываются события обработки нового подключения к серверу, а также делает сервер доступным для обнаружения
    if (this->connectionWebSocketServer->listen(QHostAddress::Any, port))
    {
        this->toConsole("Веб-сокет инициализирован.");
        this->toConsole("Подключение UDP-сокета...");

        connect(this->connectionWebSocketServer, &QWebSocketServer::newConnection, this, &ServerWindow::eventClientSocketConnected);
        this->searchUdpSocket.bind(ServerWindow::PORT_UDP_CLIENT, QUdpSocket::ShareAddress);
        connect(&this->searchUdpSocket, &QUdpSocket::readyRead, this, &ServerWindow::eventSearchServer);

        this->toConsole("UDP-сокет покдлючен.");
    }

    this->toConsole("Список всех доступных локальных IPv4:");

    // Заполняем список всех доступных локальных IPv4 адресов
    for (const QHostAddress& address: QNetworkInterface::allAddresses())
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress::LocalHost)
        {
            this->toConsole("Доступен адрес: " + address.toString() + ":" + this->currentPort);

            this->ui->addressesListWidget->addItem(address.toString() + ":" + this->currentPort);
        }

    // Генерация конца ключа шифрования (состоит из года, месяца, дня, часа, минуты, секунды и милисекунды запуска сервера)
    this->startDateTime = this->getCurrentDateTime();

    // Открытие локального веб-сокета для отправки сообщений
    this->serverWebSocket = new QWebSocket();
    this->serverWebSocket->open(QUrl("ws://" + this->connectionWebSocketServer->serverAddress().toString() + ":" + this->currentPort +
                                     "/?" + ServerWindow::SENDER_SERVER + "=" + this->startDateTime));

    this->searchUdpSocket.writeDatagram(QString(ServerWindow::SEARCH_CLIENT + ServerWindow::MESSAGE_SEPARATOR + ServerWindow::SEARCH_SERVER).toUtf8(),
                                        QHostAddress::Broadcast, ServerWindow::PORT_UDP_SERVER);
}

// Деструктор класса
ServerWindow::~ServerWindow()
{
    QString message = "Сервер был отключен.";

    this->toConsole(message);

    this->ui->logPlainTextEdit->appendHtml("<font color=\"red\">" + message + "</font>");


    // Отправляем сообщение об подключении нового пользователя
    this->toConsole("Закрытие подключеных сокетов окна.");

    for (int i = 0; i < this->clientsWebSockets.size(); i++)
    {
        this->toConsole("Пользователь " + this->clientsWebSockets[i].second + " отключен.");

        this->clientsWebSockets[i].first->close();
    }

    this->clientsWebSockets.clear();

    this->toConsole("Отключение веб-сервера.");

    this->connectionWebSocketServer->close();
    this->connectionWebSocketServer->deleteLater();

    this->toConsole("Закрытие графического окна.");

    delete this->ui;
}

// Стандартный ответ клиента при поиске сервера
const QString ServerWindow::SEARCH_CLIENT = "LANCHAT_SEARCH_CLIENT";

// Стандартный ответ сервера клиенту
const QString ServerWindow::SEARCH_SERVER = "LANCHAT_SEARCH_SERVER";

// Стандартный разделитель в ответе от сервера
const QString ServerWindow::MESSAGE_SEPARATOR = "[|!|]";

// Имя пользователя, если получатель - все пользователи
const QString ServerWindow::RECIEVER_ALL = "RECIEVER_ALL";

// Имя пользователя, если отправитель - сервер
const QString ServerWindow::SENDER_SERVER = "SENDER_SERVER";

// Отправитель сообщения клиент
const QString ServerWindow::SENDER_CLIENT = "SENDER_CLIENT";

// Занятое имя пользователя
const QString ServerWindow::USERNAME_ALREADY_EXISTS = "USERNAME_ALREADY_EXISTS";

// Подключен новый пользователь
const QString ServerWindow::USERNAME_CONNECTED = "USERNAME_CONNECTED";

// Пользователь отключен
const QString ServerWindow::USERNAME_DISCONNECTED = "USERNAME_DISCONNECTED";

// Имя пользователя не обнаружено
const QString ServerWindow::USERNAME_NOT_FOUND = "USERNAME_NOT_FOUND";

// Добавить имя пользователя в список
const QString ServerWindow::USERNAME_ADD_TO_LIST = "USERNAME_ADD_TO_LIST";

// Текстовое сообщение
const QString ServerWindow::MESSAGE_TEXT = "MESSAGE_TEXT";

// Автоматическое техническое сообщение
const QString ServerWindow::MESSAGE_TECHNICAL = "MESSAGE_TECHNICAL";

// Файловое сообщение
const QString ServerWindow::MESSAGE_BINARY = "MESSAGE_BINARY";

// Отправлен пустой файл
const QString ServerWindow::FILE_NOT = "FILE_NOT";

// Сервер отключен
const QString ServerWindow::SERVER_DISCONNECTED = "SERVER_DISCONNECTED";

// Возвращает найденный веб-сокет клиента
QWebSocket* ServerWindow::getClientWebSocketByUserName(const QString& userName)
{
    QWebSocket* foundClientWebSocket = nullptr;

    for (int i = 0; i < this->clientsWebSockets.size() && foundClientWebSocket == nullptr; i++)
        if (this->clientsWebSockets[i].second == userName)
            foundClientWebSocket = this->clientsWebSockets[i].first;

    return foundClientWebSocket;
}

// Возвращает индекс веб-сокета клиента в общем списке
int ServerWindow::getIndexOfClientWebSocket(QWebSocket* clientWebSocket)
{
    int foundIndex = -1;

    for (int i = 0; i < this->clientsWebSockets.size() && foundIndex < 0; i++)
        if (this->clientsWebSockets[i].first == clientWebSocket)
            foundIndex = i;

    return foundIndex;
}

// Возвращает адрес сервера, который находится в той же под сети
QString ServerWindow::findServerAddress(const QString& address)
{
    QString foundAddress = "";

    // Разбиваем полученный адрес на 4 части
    QStringList addressBytes = address.split(".");

    // Если получилось 4 байта, то адресс указан в корректном формате
    if (addressBytes.size() == 4)
        for (int i = 0; i < this->ui->addressesListWidget->count() && foundAddress.size() == 0; i++)
        {
            // Разбиваем взятый из списка адрес
            QStringList foundBytes = this->ui->addressesListWidget->item(i)->text().split(".");

            // Если получилось 4 байта, то адрес указан в корректном формате
            if (foundBytes.size() == 4)
                // Если первые 3 байта адреса клиента и сервера совпали, то они находятся в одной сети, запоминаем адрес сервера
                if (addressBytes[0] == foundBytes[0] && addressBytes[1] == foundBytes[1] && addressBytes[2] == foundBytes[2])
                {
                    foundAddress = this->ui->addressesListWidget->item(i)->text();
                    foundAddress = foundAddress.mid(0, foundAddress.indexOf(":"));
                }
        }

    return foundAddress;
}

// Выводит сообщение в консоль
void ServerWindow::toConsole(const QString &text)
{
    if (this->debug)
        qDebug() << "[" << this->metaObject()->className() << "]: "  << text;
}

// Возвращает, введено ли корректное имя пользователя
bool ServerWindow::isCorrectUsername(const QString& userName, const QString& connectionDateTime)
{
    bool check = userName.size() > 2 && userName.size() < 17;

    bool serverWebSocketRequest = userName == ServerWindow::SENDER_SERVER && connectionDateTime == this->startDateTime;

    if (check)
        check = userName != ServerWindow::SENDER_SERVER || serverWebSocketRequest;

    if (!serverWebSocketRequest)
        for (int i = 0; i < userName.size() && check; i++)
            check = ((userName[i] >= 'a' && userName[i] <= 'z') ||
                    (userName[i] >= 'A' && userName[i] <= 'Z') ||
                    (userName[i] >= '0' && userName[i] <= '9'));

    if (check)
        check = this->getClientWebSocketByUserName(userName) == nullptr;

    return check;
}

// Возвращает текущие даут и время
QString ServerWindow::getCurrentDateTime(const QString& separator)
{
    return QString::number(QDate::currentDate().day()) + "." +
           QString::number(QDate::currentDate().month()) + "." +
           QString::number(QDate::currentDate().year()) + separator +
           QString::number(QTime::currentTime().hour()) + "-" +
           QString::number(QTime::currentTime().minute()) + "-" +
           QString::number(QTime::currentTime().second()) + "-" +
           QString::number(QTime::currentTime().msec());
}

// Событие при подключении веб-соекта нового клиента
void ServerWindow::eventClientSocketConnected()
{
    // Получение веб-сокета нового клиента
    QWebSocket* newClientWebSocket = this->connectionWebSocketServer->nextPendingConnection();

    this->toConsole("Веб-сокет подключен: " + newClientWebSocket->requestUrl().toString());

    // Отбрасываем URL сервера
    QStringList parameters = newClientWebSocket->requestUrl().toString().split("?");

    // Связываем событие получения сообщения
    connect(newClientWebSocket, &QWebSocket::textMessageReceived, this, &ServerWindow::eventClientTextMessageRecieved);
    connect(newClientWebSocket, &QWebSocket::binaryMessageReceived, this, &ServerWindow::eventClientBinaryMessageRecieved);

    if (parameters.size() == 2)
    {
        // Получаем параметры: имя пользователя и время подключения
        parameters = parameters[1].split("=");

        if (parameters.size() == 2)
        {
            this->toConsole("Имя пользователя: " + parameters[0] + " Время подключения: " + parameters[1]);

            if (this->isCorrectUsername(parameters[0], parameters[1]))
            {
                this->toConsole("Состояние веб-сокета клиента: " + QString::number((short) newClientWebSocket->state()));

                // Связываем событие получения отключения веб-сокета клиента от сервера
                connect(newClientWebSocket, &QWebSocket::disconnected, this, &ServerWindow::eventClientSocketDisconnected);

                // Запоминаем веб-сокет нового клиента
                if (parameters[0] != ServerWindow::SENDER_SERVER)
                {
                    // Отправляем веб-сокету клиента имена всех уже подключенных пользователей
                    for (int i = 0; i < this->clientsWebSockets.size(); i++)
                        this->serverWebSocket->sendTextMessage(
                                    ServerWindow::SENDER_CLIENT + ServerWindow::MESSAGE_SEPARATOR +
                                    ServerWindow::MESSAGE_TECHNICAL + ServerWindow::MESSAGE_SEPARATOR +
                                    parameters[0] + ServerWindow::MESSAGE_SEPARATOR +
                                    ServerWindow::USERNAME_ADD_TO_LIST + ServerWindow::MESSAGE_SEPARATOR +
                                    this->clientsWebSockets[i].second);

                    this->clientsWebSockets.push_back(QPair(newClientWebSocket, parameters[0]));
                    this->ui->usersListWidget->addItem(parameters[0]);

                    // Отправляем сообщение об подключении нового пользователя
                    this->serverWebSocket->sendTextMessage(
                                ServerWindow::SENDER_CLIENT + ServerWindow::MESSAGE_SEPARATOR +
                                ServerWindow::MESSAGE_TECHNICAL + ServerWindow::MESSAGE_SEPARATOR +
                                ServerWindow::RECIEVER_ALL + ServerWindow::MESSAGE_SEPARATOR +
                                ServerWindow::USERNAME_CONNECTED + ServerWindow::MESSAGE_SEPARATOR +
                                parameters[0]);
                }
            }
            else
            {
                // Посылаем сообщение об отключении от сервера
                newClientWebSocket->sendTextMessage(
                            ServerWindow::SENDER_CLIENT + ServerWindow::MESSAGE_SEPARATOR +
                            ServerWindow::MESSAGE_TECHNICAL + ServerWindow::MESSAGE_SEPARATOR +
                            parameters[0] + ServerWindow::MESSAGE_SEPARATOR +
                            ServerWindow::USERNAME_ALREADY_EXISTS + ServerWindow::MESSAGE_SEPARATOR +
                            parameters[1]);

                // Посылаем команду отключения веб-сокета от сервера
                newClientWebSocket->close();
                newClientWebSocket->deleteLater();

                this->toConsole("Некорректное имя пользователя. Возможно, пользователь с таким именем уже подключен.");
            }
        }
        else
        {
            // Посылаем сообщение об отключении от сервера
            newClientWebSocket->sendTextMessage(
                        ServerWindow::SENDER_CLIENT + ServerWindow::MESSAGE_SEPARATOR +
                        ServerWindow::MESSAGE_TECHNICAL + ServerWindow::MESSAGE_SEPARATOR +
                        parameters[0] + ServerWindow::MESSAGE_SEPARATOR +
                        ServerWindow::USERNAME_NOT_FOUND + ServerWindow::MESSAGE_SEPARATOR +
                        parameters[1]);

            // Посылаем команду отключения веб-сокета от сервера
            newClientWebSocket->close();
            newClientWebSocket->deleteLater();

            this->toConsole("Не найдено имя пользователя в запросе веб-сокета клиента при подключении к серверу.");
        }
    }
    else
    {
        // Посылаем сообщение об отключении от сервера
        newClientWebSocket->sendTextMessage(
                    ServerWindow::SENDER_CLIENT + ServerWindow::MESSAGE_SEPARATOR +
                    ServerWindow::MESSAGE_TECHNICAL + ServerWindow::MESSAGE_SEPARATOR +
                    parameters[0] + ServerWindow::MESSAGE_SEPARATOR +
                    ServerWindow::USERNAME_NOT_FOUND + ServerWindow::MESSAGE_SEPARATOR +
                    parameters[1]);

        // Посылаем команду отключения веб-сокета от сервера
        newClientWebSocket->close();
        newClientWebSocket->deleteLater();

        this->toConsole("Не найдено имя пользователя в запросе веб-сокета клиента при подключении к серверу.");
    }
}

// Событие при получении текстового сообщения сервером
void ServerWindow::eventClientTextMessageRecieved(const QString& data)
{
    /*
     * Виды получаемых сообщений:
     * 1. Текстовое:
     * SENDER_SERVER MESSAGE_SEPARATOR message_type MESSAGE_SEPARATOR sender MESSAGE_SEPARATOR reciever MESSAGE_SEPARATOR text (5 элементов)
     *
     * 2. Файловое:
     * SENDER_SERVER MESSAGE_SEPARATOR message_type MESSAGE_SEPARATOR sender MESSAGE_SEPARATOR reciever
     * MESSAGE_SEPARATOR filename (5 элементов)
     *
     * 3. Ответ от сервера:
     * SENDER_SERVER MESSAGE_SEPARATOR message_type MESSAGE_SEPARATOR reciever MESSAGE_SEPARATOR command MESSAGE_SEPARATOR value (5 элементов)
     */

    // Парсим полученное сообщение
    QStringList parameters = data.split(ServerWindow::MESSAGE_SEPARATOR);

    this->toConsole("Получено сообщение.");

    // Если сообщение примерно соответствует шаблону, то выполняется поиск веб-сокета пользователя-получателя
    if (parameters.size() == 5)
    {
        this->toConsole("Сообщение соответствует шаблону.");
        this->toConsole("Отправитель (клиент/сервер): " + parameters[0]);

        if (parameters[0] == ServerWindow::SENDER_CLIENT)
        {
            this->toConsole("Требуется обработка на сервере.");

            QString newData = ServerWindow::SENDER_SERVER;

            for (int i = 1; i < 5; i++)
                newData += ServerWindow::MESSAGE_SEPARATOR + parameters[i];

            this->toConsole("Отправляется сообщение.");

            // Сообщение отправляется всем клиентам
            if (parameters[3] == ServerWindow::RECIEVER_ALL || parameters[2] == ServerWindow::RECIEVER_ALL)
            {
                this->toConsole("Сообщение отправляется всем пользователям.");

                for (int i = 0; i < this->clientsWebSockets.size(); i++)
                    this->clientsWebSockets[i].first->sendTextMessage(newData);
            }
            // Сообщение отправляется конкретному клиенту
            else
            {
                this->toConsole("Сообщение отправляется пользователю " + parameters[3]);

                QWebSocket* recieverClientWebSocket = this->getClientWebSocketByUserName(parameters[3]);
                QWebSocket* senderClientWebSocket = this->getClientWebSocketByUserName(parameters[2]);

                if (senderClientWebSocket != nullptr)
                    senderClientWebSocket->sendTextMessage(newData);

                if (recieverClientWebSocket != nullptr)
                    recieverClientWebSocket->sendTextMessage(newData);
            }

            // Вывод сообщений в лог сервера
            // Получено техническое сообщение от сервера
            if (parameters[1] == ServerWindow::MESSAGE_TECHNICAL)
            {
                this->toConsole("Это техническое сообщение от сервера.");

                // Пользователь с таким именем уже подключен
                if (parameters[3] == ServerWindow::USERNAME_ALREADY_EXISTS)
                {
                    QString message = "Пользователь с именем" + parameters[2] + "уже подключен к серверу.";

                    this->toConsole(message);

                    this->ui->logPlainTextEdit->appendHtml("<font color=\"red\">" + message + "</font>");
                }
                // Подключен новый пользователь
                else if (parameters[3] == ServerWindow::USERNAME_CONNECTED)
                {
                    QString message = "Подключен новый пользователь " + parameters[4] + ".";

                    this->toConsole(message);

                    this->ui->logPlainTextEdit->appendHtml("<font color=\"green\">" + message + "</font>");
                }
                // Пользователь отключен
                else if (parameters[3] == ServerWindow::USERNAME_DISCONNECTED)
                {
                    QString message = "Пользователь " + parameters[4] + " отключен.";

                    this->toConsole(message);

                    this->ui->logPlainTextEdit->appendHtml("<font color=\"red\">" + message + "</font>");
                }
            }
            // Получено текстовое сообщение от пользователя
            else if (parameters[1] == ServerWindow::MESSAGE_TEXT)
            {
                this->toConsole("Отправитель: " + parameters[2] + " Получатель: " +
                        QString(parameters[3] == ServerWindow::RECIEVER_ALL ? "Все" : parameters[3]) + " Сообщение: " + parameters[4]);

                this->ui->logPlainTextEdit->appendHtml(
                        "<font color=\"navy\">[" +
                        parameters[2] + "/" +
                        this->getCurrentDateTime() + "/" +
                        QString(parameters[3] == ServerWindow::RECIEVER_ALL ? "Все" : "Только " + parameters[3]) +
                        "]: </font><font color=\"black\"> " + parameters[4] + "</font>");
            }
            // Получено бинарное сообщение от пользователя
            else if (parameters[1] == ServerWindow::MESSAGE_BINARY)
            {
                this->toConsole("Полученные данные: Отправитель: " + parameters[2] + " Получатель: " +
                        QString(parameters[3] == ServerWindow::RECIEVER_ALL ? "Все" : parameters[3]) + " Имя файла: " + parameters[4]);

                this->ui->logPlainTextEdit->appendHtml(
                        "<font color=\"teal\">[" +
                        parameters[2] + "/" +
                        this->getCurrentDateTime() + "/" +
                        QString(parameters[3] == ServerWindow::RECIEVER_ALL ? "Все" : "Только " + parameters[3]) +
                        "]: Отправлен файл: " + parameters[4] + "</font>");
            }
        }
        else
            this->toConsole("Обработка на сервере не требуется.");
    }
    else
        this->toConsole("Сообщение не соответствует шаблону.");
}

// Событие при получении бинарного сообщения сервером
void ServerWindow::eventClientBinaryMessageRecieved(const QByteArray& data)
{
    /*
     * Шаблон бинарного сообщения:
     * SENDER_SERVER MESSAGE_SEPARATOR reciever MESSAGE_SEPARATOR filename MESSAGE_SEPARATOR data
     */
    QStringList parameters = QString::fromUtf8(data).split(ServerWindow::MESSAGE_SEPARATOR);

    // Если обнаружено минимум 5 аргументов, то шаблон запроса, вероятно, правильный
    if (parameters.size() >= 5)
    {
        this->toConsole("Корректное количество аргументов бинарного сообщения.");

        // Отправителем был клиент
        if (parameters[0] == ServerWindow::SENDER_CLIENT)
        {
            this->toConsole("Требуется обработка со стороны сервера.");

            // Поиск веб-сокета в списке веб-сокетов клиентов
            QWebSocket* foundClientWebSocket = this->getClientWebSocketByUserName(parameters[2]);

            // Если такой веб-сокет существует, то формируем новый запрос для веб-сокета найденного клиента
            if (foundClientWebSocket != nullptr)
            {
                this->toConsole("Найден веб-сокет клиента-получателя файла.");

                QByteArray newData = ServerWindow::SENDER_SERVER.toUtf8();

                for (int i = 0, j = 0; i < data.size(); i++, j++)
                    if (i >= parameters[0].size())
                        newData.push_back(data[i]);

                this->toConsole("Отправка файла клиенту-получателю.");

                foundClientWebSocket->sendBinaryMessage(newData);
            }
        }
        else
            this->toConsole("Обработка со стороны сервера не требуется.");
    }
}

// Событие при отключении веб-сокета существующего клиента
void ServerWindow::eventClientSocketDisconnected()
{
    // Получение веб-сокета клиента-отправителя
    QWebSocket* senderClientWebSocket = qobject_cast<QWebSocket*>(sender());

    this->toConsole("Веб-сокет клиента был отключен.");

    // Если веб-сокет валидный, то выполняется удаения данного веб-сокета из списка
    if (senderClientWebSocket)
    {
        QString userName = "";

        for (int i = 0; i < this->clientsWebSockets.size(); i++)
            // Если веб-сокет найден в списке, то он удаляется из него
            if (this->clientsWebSockets[i].first == senderClientWebSocket)
            {
                // Запоминаем имя пользователя
                if (userName.size() == 0)
                {
                    userName = this->clientsWebSockets[i].second;

                    this->serverWebSocket->sendTextMessage(
                                ServerWindow::SENDER_CLIENT + ServerWindow::MESSAGE_SEPARATOR +
                                ServerWindow::MESSAGE_TECHNICAL + ServerWindow::MESSAGE_SEPARATOR +
                                ServerWindow::RECIEVER_ALL + ServerWindow::MESSAGE_SEPARATOR +
                                ServerWindow::USERNAME_DISCONNECTED + ServerWindow::MESSAGE_SEPARATOR +
                                userName);

                    this->toConsole("Имя пользователя отключенного клиента: " + userName);
                }

                this->clientsWebSockets.removeAt(i--);
            }

        // Находим пользователя в списке пользователей и удаляем оттуда
        for (int i = 0; i < this->ui->usersListWidget->count(); i++)
        {
            // Получаем элемент из списка
            QListWidgetItem* itemToRemove = this->ui->usersListWidget->item(i);

            // Если имена совпадают, то извлекаем из списка элемент и удаляем его
            if (itemToRemove->text() == userName)
            {
                itemToRemove = this->ui->usersListWidget->takeItem(i);
                delete itemToRemove;
                i--;
            }
        }


        senderClientWebSocket->deleteLater();
    }
}

// Событие, возникающее при поиске сервера (получение данных в UDP-сокет)
void ServerWindow::eventSearchServer()
{
    QByteArray request;

    this->toConsole("Поиск запросов от клиентов на подключение...");

    while (this->searchUdpSocket.hasPendingDatagrams())
    {
        request.resize(this->searchUdpSocket.pendingDatagramSize());
        searchUdpSocket.readDatagram(request.data(), request.size());

        this->toConsole("Есть запросы для подлкючения.");

        // Параметры, полученные в UDP-сокет
        QStringList parameters = QString::fromUtf8(request).split(ServerWindow::MESSAGE_SEPARATOR);

        // Количество полученный параметров должно быть ровно два
        if (parameters.size() == 2)
        {
            this->toConsole("Корректное количество параметров запроса подключения.");

            // Найденная пара IPv4 с портом в списке адресов сервера
            QString foundAddress = this->findServerAddress(parameters[1]);

            /*
             * Если это был запрос о поиске сервера и полученный адрес клиента имеет корректный формат, то посылается сообщение формата:
             * COMMAND_CLIENT_SEARCH MESSAGE_SEPARATOR server_ip MESSAGE_SEPARATOR server_port
             * MESSAGE_SEPARATOR client_ip MESSAGE_SEPARATOR keyEncryptEnd (5 элементов)
             */
            if (parameters[0] == ServerWindow::SEARCH_SERVER && foundAddress.size() > 0)
            {
                this->toConsole("Запрос задан верно. Адрес сервера: " +
                                foundAddress + ":" + this->currentPort + " Адрес клиента: " + parameters[1]);

                this->searchUdpSocket.writeDatagram(QString(ServerWindow::SEARCH_CLIENT + ServerWindow::MESSAGE_SEPARATOR +
                                                      foundAddress + ServerWindow::MESSAGE_SEPARATOR +
                                                      this->currentPort + ServerWindow::MESSAGE_SEPARATOR +
                                                      parameters[1]).toUtf8(),
                                                   QHostAddress::Broadcast, ServerWindow::PORT_UDP_SERVER);
            }
            else
                this->toConsole("Запрос задан неверно. Адрес сервера: " + foundAddress + ":" + this->currentPort + "Адрес клиента: " + parameters[1]);
        }
        else
            this->toConsole("Некорректное количество параметров запроса подключения.");
    }
}

// Событие при нажатии на элемент списка пользователей
void ServerWindow::eventUsersListWidgetItemClicked(QListWidgetItem* item)
{
    if (item == this->currentUserListWidgetItem)
    {
        this->currentUserListWidgetItem = nullptr;
        this->ui->usersListWidget->setCurrentItem(nullptr);
    }
    else
        this->currentUserListWidgetItem = item;
}

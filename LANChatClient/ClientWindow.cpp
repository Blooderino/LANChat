#include "ClientWindow.h"
#include "ui_ClientWindow.h"

// Конструктор класса
ClientWindow::ClientWindow(bool debug, QWidget* parent) : QMainWindow(parent), ui(new Ui::ClientWindow)
{
    this->debug = debug;

    this->toConsole("Запуск графической оболочки.");

    this->ui->setupUi(this);
    this->serverUrl = "ws://";
    this->serverAddressFound = false;
    this->currentUserListWidgetItem = nullptr;

    // События для графического интерфейса
    connect(this->ui->authLoginLineEdit, &QLineEdit::textChanged, this, &ClientWindow::eventAuthLoginLineEditTextChanged);
    connect(this->ui->authSignInPushButton, &QPushButton::clicked, this, &ClientWindow::eventAuthSignInPushButtonClicked);
    connect(this->ui->actionsSendMessagePlainTextEdit, &QPlainTextEdit::textChanged, this, &ClientWindow::eventActionsSendMessagePlainTextEditTextChanged);
    connect(this->ui->actionsSendMessagePushButton, &QPushButton::clicked, this, &ClientWindow::eventActionsSendMessagePushButtonClicked);
    connect(this->ui->userListWidget, &QListWidget::itemClicked, this, &ClientWindow::eventUserListWidgetItemClicked);
    connect(this->ui->actionsSendFilePushButton, &QPushButton::clicked, this, &ClientWindow::eventActionsSendFilePushButtonClicked);

    this->toConsole("Подключение UDP-сокета...");

    // Открываем UDP-сокет для поиска севера в локальной сети
    this->searchUdpSocket.bind(ClientWindow::PORT_UDP_SERVER, QUdpSocket::ShareAddress);
    connect(&this->searchUdpSocket, &QUdpSocket::readyRead, this, &ClientWindow::eventSearchServer);

    this->toConsole("UDP-сокет покдлючен.");
    this->toConsole("Список всех доступных локальных IPv4:");

    // Ищем все доступные локальные IPv4 адреса. Найденные адреса посылаем серверу как запросы на подключение.
    this->doSendMessageToSearchServer();
}

// Деструктор класса
ClientWindow::~ClientWindow()
{
    this->clientWebSocket.close();
    delete this->ui;
}

// Стандартный ответ клиента при поиске сервера
const QString ClientWindow::SEARCH_CLIENT = "LANCHAT_SEARCH_CLIENT";

// Стандартный ответ сервера клиенту
const QString ClientWindow::SEARCH_SERVER = "LANCHAT_SEARCH_SERVER";

// Стандартный разделитель в ответе от сервера
const QString ClientWindow::MESSAGE_SEPARATOR = "[|!|]";

// Имя пользователя, если получатель - все пользователи
const QString ClientWindow::RECIEVER_ALL = "RECIEVER_ALL";

// Имя пользователя, если отправитель - сервер
const QString ClientWindow::SENDER_SERVER = "SENDER_SERVER";

// Отправитель сообщения клиент
const QString ClientWindow::SENDER_CLIENT = "SENDER_CLIENT";

// Занятое имя пользователя
const QString ClientWindow::USERNAME_ALREADY_EXISTS = "USERNAME_ALREADY_EXISTS";

// Подключен новый пользователь
const QString ClientWindow::USERNAME_CONNECTED = "USERNAME_CONNECTED";

// Пользователь отключен
const QString ClientWindow::USERNAME_DISCONNECTED = "USERNAME_DISCONNECTED";

// Добавить имя пользователя в список
const QString ClientWindow::USERNAME_ADD_TO_LIST = "USERNAME_ADD_TO_LIST";

// Текстовое сообщение
const QString ClientWindow::MESSAGE_TEXT = "MESSAGE_TEXT";

// Файловое сообщение
const QString ClientWindow::MESSAGE_BINARY = "MESSAGE_BINARY";

// Автоматическое техническое сообщение
const QString ClientWindow::MESSAGE_TECHNICAL = "MESSAGE_TECHNICAL";

// Отправлен пустой файл
const QString ClientWindow::FILE_NOT = "FILE_NOT";

// Сервер отключен
const QString ClientWindow::SERVER_DISCONNECTED = "SERVER_DISCONNECTED";

// Управляет активностью кнопки "Подключиться"
void ClientWindow::doAuthSignInPushButtonEnabled()
{
    this->ui->authSignInPushButton->setEnabled(
                this->isCorrectUsername() &&
                this->ui->authClientIPv4ValueLabel->text() != "Нет данных" &&
                this->ui->authServerIPv4ValueLabel->text() != "Нет данных" &&
                this->serverUrl.host().size() > 0 &&
                this->serverUrl.port() != -1);
}

// Устанавливает текст для адреса клиента и управляет активностью связанных полей
void ClientWindow::doAuthClientIPv4LabelsSetText(const QString& text)
{
    this->ui->authClientIPv4ValueLabel->setText(text);
    this->ui->authClientIPv4ValueLabel->setEnabled(text != "Нет данных");
    this->ui->authClientIPv4TextLabel->setEnabled(this->ui->authClientIPv4ValueLabel->isEnabled());
    this->doAuthSignInPushButtonEnabled();
}

// Устанавливает текст для адреса сервера и управляет активностью связанных полей
void ClientWindow::doAuthServerIPv4LabelsSetText(const QString& text)
{
    this->ui->authServerIPv4ValueLabel->setText(text);
    this->ui->authServerIPv4ValueLabel->setEnabled(text != "Нет данных");
    this->ui->authServerIPv4TextLabel->setEnabled(this->ui->authServerIPv4ValueLabel->isEnabled());
    this->doAuthSignInPushButtonEnabled();
}

// Выводит сообщение в консоль
void ClientWindow::toConsole(const QString &text)
{
    if (this->debug)
        qDebug() << "[" << this->metaObject()->className() << "]: "  << text;
}

// Посылает сообщения для поиска сервера в сети
void ClientWindow::doSendMessageToSearchServer()
{
    // Ищем все доступные локальные IPv4 адреса. Найденные адреса посылаем серверу как запросы на подключение.
    for (const QHostAddress& address: QNetworkInterface::allAddresses())
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress::LocalHost)
        {
            this->toConsole("Доступен адрес: " + address.toString() + " (отправка запроса на подключение).");

            this->searchUdpSocket.writeDatagram(QString(ClientWindow::SEARCH_SERVER + ClientWindow::MESSAGE_SEPARATOR + address.toString()).toUtf8(),
                                                QHostAddress::Broadcast, ClientWindow::PORT_UDP_CLIENT);
        }
}

// Возвращает, введено ли корректное имя пользователя
bool ClientWindow::isCorrectUsername()
{
    QString userName = this->ui->authLoginLineEdit->text();
    bool check = userName.size() > 2 && userName.size() < 17;


    for (int i = 0; i < userName.size() && check; i++)
        check = ((userName[i] >= 'a' && userName[i] <= 'z') ||
                (userName[i] >= 'A' && userName[i] <= 'Z') ||
                (userName[i] >= '0' && userName[i] <= '9'));


    if (check)
        check = userName != ClientWindow::SENDER_SERVER;

    return check;
}

// Включает основные элементы управления
void ClientWindow::doSetChatEnabled(bool check)
{
    this->ui->userListWidget->setEnabled(check);
    this->ui->actionsSendMessagePlainTextEdit->setEnabled(check);
}

// Возвращает текущие даут и время
QString ClientWindow::getCurrentDateTime(const QString& separator)
{
    return QString::number(QDate::currentDate().day()) + "." +
           QString::number(QDate::currentDate().month()) + "." +
           QString::number(QDate::currentDate().year()) + separator +
           QString::number(QTime::currentTime().hour()) + "-" +
           QString::number(QTime::currentTime().minute()) + "-" +
           QString::number(QTime::currentTime().second()) + "-" +
           QString::number(QTime::currentTime().msec());
}

// Возвращает папку для сохранения файлов
QString ClientWindow::getDirectorySaveFilesPath()
{
    return QDir::homePath() + "/LANChatFiles/";
}

// Событие, вызывающееся при изменении текста в окне ввода логина
void ClientWindow::eventAuthLoginLineEditTextChanged(const QString& text)
{
    this->doAuthSignInPushButtonEnabled();
}

// Событие при нажатии кнопки подключения к серверу
void ClientWindow::eventAuthSignInPushButtonClicked(bool clicked)
{
    // Блокируем кнопку подключения
    this->ui->authSignInPushButton->setEnabled(false);
    this->ui->authLoginLineEdit->setEnabled(false);

    this->connectionDateTime = this->getCurrentDateTime();
    this->clientWebSocket.open(QUrl(serverUrl.toString() + "/?" + this->ui->authLoginLineEdit->text() + "=" + this->connectionDateTime));

    this->toConsole("URL запроса веб-сокета клиента: " + this->clientWebSocket.requestUrl().toString());

    // Если веб-сокет подключен, то связываются события получения сообщения, разблокируется пользовательский интерфейс
    if (this->clientWebSocket.state() == QAbstractSocket::SocketState::ConnectingState)
    {
        this->toConsole("Веб-сокет клиента подключается.");

        connect(&this->clientWebSocket, &QWebSocket::connected, this, &ClientWindow::eventConnected);
        connect(&this->clientWebSocket, &QWebSocket::textMessageReceived, this, &ClientWindow::eventTextMessageRecieved);
        connect(&this->clientWebSocket, &QWebSocket::disconnected, this, &ClientWindow::eventDisconnected);
        connect(&this->clientWebSocket, &QWebSocket::binaryMessageReceived, this, &ClientWindow::eventBinaryTextMessageRecieved);
    }
    // Иначе происходит повторная попытка подключиться
    else
    {
        this->toConsole("Веб-сокет клиента не подключен.");

        this->doSendMessageToSearchServer();
        this->ui->authLoginLineEdit->setEnabled(true);
    }
}

// Событие, возникающее при завершении подключения веб-сокета клиента к серверу
void ClientWindow::eventConnected()
{
    this->doSetChatEnabled(this->clientWebSocket.state() == QAbstractSocket::SocketState::ConnectedState);
}

// Событие, возникающее при завершении подключения веб-сокета клиента к серверу
void ClientWindow::eventDisconnected()
{
    this->doSetChatEnabled(false);
    this->ui->authLoginLineEdit->setEnabled(true);
    this->doAuthClientIPv4LabelsSetText("Нет данных");
    this->doAuthServerIPv4LabelsSetText("Нет данных");
    this->serverAddressFound = false;
    this->ui->chatPlainTextEdit->appendHtml("<font color=\"red\">Вы были отключены от сервера.</font>");

    disconnect(&this->clientWebSocket, &QWebSocket::connected, this, &ClientWindow::eventConnected);
    disconnect(&this->clientWebSocket, &QWebSocket::textMessageReceived, this, &ClientWindow::eventTextMessageRecieved);
    disconnect(&this->clientWebSocket, &QWebSocket::disconnected, this, &ClientWindow::eventDisconnected);
    disconnect(&this->clientWebSocket, &QWebSocket::binaryMessageReceived, this, &ClientWindow::eventBinaryTextMessageRecieved);

    this->doSendMessageToSearchServer();
}

// Событие, возникающее при поиске сервера (получение данных в UDP-сокет)
void ClientWindow::eventSearchServer()
{
    // Ответ по UDP-сокету
    QByteArray answer;

    this->toConsole("Поиск ответа от сервера на подключение...");

    while (this->searchUdpSocket.hasPendingDatagrams())
    {
        // Получаем сообщение
        answer.resize(this->searchUdpSocket.pendingDatagramSize());
        this->searchUdpSocket.readDatagram(answer.data(), answer.size());

        // Формируем список параметров
        QStringList parameters = QString(answer).split(ClientWindow::MESSAGE_SEPARATOR);

        this->toConsole("Есть ответ для подлкючения.");

        // Если количество полученных параметров равно 4, то запрос верный
        if (parameters.size() == 4)
        {
            this->toConsole("Корректное количество параметров ответа подключения.");

            // Если получен ответ от искомого сервера и текущий веб-сокет клиента ещё не подключен к нему, то клиент подгатавливается к подключению
            if (parameters[0] == ClientWindow::SEARCH_CLIENT &&
                clientWebSocket.state() != QAbstractSocket::ConnectedState && !this->serverAddressFound)
            {
                this->toConsole("Запрос задан верно. Адрес сервера: " + parameters[1] + ":" + parameters[2] + " Адрес клиента: " + parameters[3]);

                this->serverUrl.setHost(parameters[1]);
                this->serverUrl.setPort(parameters[2].toInt());
                this->doAuthServerIPv4LabelsSetText(parameters[1] + ":" + parameters[2]);
                this->doAuthClientIPv4LabelsSetText(parameters[3]);
                this->serverAddressFound = true;
            }
            else
                this->toConsole("Запрос задан неверно или подключение уже существует. Адрес сервера: " +
                                parameters[1] + ":" + parameters[2] + " Адрес клиента: " + parameters[3]);
        }
        // Если количество полученных параметров равно 2, то ответ от сервера верный
        else if (parameters.size() == 2)
        {
            this->toConsole("Корректное количество параметров поиска клиента.");

            if (parameters[0] == ClientWindow::SEARCH_CLIENT && parameters[1] == ClientWindow::SEARCH_SERVER)
            {
                this->toConsole("Корректный ответ от сервера при поиске клиентов.");

                this->doSendMessageToSearchServer();
            }
            else
                this->toConsole("Некорректный ответ от сервера при поиске клиентов.");
        }
        else
            this->toConsole("Некорректное количество параметров ответа подключения.");
    }
}

// Событие, возникающее при получении сообщения
void ClientWindow::eventTextMessageRecieved(const QString& data)
{
    /*
     * Виды получаемых сообщений:
     * 1. Текстовое:
     * SENDER_SERVER MESSAGE_SEPARATOR message_type MESSAGE_SEPARATOR sender MESSAGE_SEPARATOR reciever MESSAGE_SEPARATOR text (5 элементов)
     *
     * 2. Файловое:
     * SENDER_SERVER MESSAGE_SEPARATOR message_type MESSAGE_SEPARATOR sender MESSAGE_SEPARATOR reciever MESSAGE_SEPARATOR filename (5 элементов)
     *
     * 3. Ответ от сервера:
     * SENDER_SERVER MESSAGE_SEPARATOR message_type MESSAGE_SEPARATOR reciever MESSAGE_SEPARATOR command MESSAGE_SEPARATOR value (5 элементов)
     */

    // Парсинг полученного сообщения
    QStringList parameters = data.split(ClientWindow::MESSAGE_SEPARATOR);

    this->toConsole("Получено текстовое сообщение.");

    // Если получено текстовое сообщение, то оно выводится на экран пользователя
    if (parameters.size() == 5)
    {
        this->toConsole("Текстовое ообщение соответствует шаблону.");

        // Сообщение пересылал сервер
        if (parameters[0] == ClientWindow::SENDER_SERVER)
        {
            this->toConsole("Сообщение готово к обработке в клиенте.");

            // Получено техническое сообщение от сервера
            if (parameters[1] == ClientWindow::MESSAGE_TECHNICAL)
            {
                this->toConsole("Это техническое текстовое сообщение от сервера.");

                // Пользователь с таким именем уже подключен
                if (parameters[2] == this->ui->authLoginLineEdit->text() &&
                    parameters[3] == ClientWindow::USERNAME_ALREADY_EXISTS &&
                    parameters[4] == this->connectionDateTime)
                {
                    QString message = "Пользователь с таким именем уже подключен к серверу. Выберите другое имя.";

                    this->toConsole(message);

                    this->ui->chatPlainTextEdit->appendHtml("<font color=\"red\">" + message + "</font>");
                }
                // Подключен новый пользователь
                else if (parameters[3] == ClientWindow::USERNAME_CONNECTED)
                {
                    bool check = parameters[4] != this->ui->authLoginLineEdit->text();
                    QString message = check ? "Подключен новый пользователь " + parameters[4] + "." : "Вы подключены к серверу.";

                    this->toConsole(message);

                    this->ui->chatPlainTextEdit->appendHtml("<font color=\"green\">" + message + "</font>");

                    this->toConsole("Добавление пользователя " + parameters[4] + " в список.");

                    if (check)
                    {
                        this->ui->userListWidget->addItem(parameters[4]);

                        this->toConsole("Пользователь " + parameters[4] + " добавлен в список.");
                    }
                    else
                        this->toConsole("Пользователь " + parameters[4] + " не был добавлен в список.");
                }
                // Пользователь отключен
                else if (parameters[3] == ClientWindow::USERNAME_DISCONNECTED)
                {
                    bool check = parameters[4] != this->ui->authLoginLineEdit->text();
                    QString message = check ? "Пользователь " + parameters[4] + " отключен." : "Вы отключены от сервера.";

                    this->toConsole(message);

                    this->ui->chatPlainTextEdit->appendHtml("<font color=\"red\">" + message + "</font>");

                    // Находим пользователя в списке пользователей и удаляем оттуда
                    for (int i = 0; i < this->ui->userListWidget->count() && check; i++)
                    {
                        // Получаем элемент из списка
                        QListWidgetItem* itemToRemove = this->ui->userListWidget->item(i);

                        // Если имена совпадают, то извлекаем из списка элемент и удаляем его
                        if (itemToRemove->text() == parameters[4])
                        {
                            itemToRemove = this->ui->userListWidget->takeItem(i);
                            delete itemToRemove;
                            i--;
                        }
                    }
                }
                // Добавить пользователя в список
                else if (parameters[3] == ClientWindow::USERNAME_ADD_TO_LIST)
                {
                    this->toConsole("Добавление пользователя " + parameters[4] + " в список.");

                    if (parameters[4] != this->ui->authLoginLineEdit->text())
                    {
                        this->ui->userListWidget->addItem(parameters[4]);

                        this->toConsole("Пользователь " + parameters[4] + " добавлен в список.");
                    }
                    else
                        this->toConsole("Пользователь " + parameters[4] + " не был добавлен в список.");
                }
            }
            // Получено текстовое сообщение от пользователя
            else if (parameters[1] == ClientWindow::MESSAGE_TEXT)
            {
                this->toConsole("Это текстовое соообщение от пользователя.");
                this->toConsole("Отправитель: " + parameters[2] + " Получатель: " +
                        QString(parameters[3] == ClientWindow::RECIEVER_ALL ? "Все" : parameters[3]) + " Сообщение: " + parameters[4]);

                this->ui->chatPlainTextEdit->appendHtml(
                        "<font color=\"navy\">[" +
                        parameters[2] + "/" +
                        this->getCurrentDateTime() + "/" +
                        QString(parameters[3] == ClientWindow::RECIEVER_ALL ? "Все" : ("Только " +
                                QString(parameters[2] == this->ui->authLoginLineEdit->text() ? parameters[3] : "Вам"))) +
                        "]: </font><font color=\"black\"> " + parameters[4] + "</font>");
            }
            // Получено бинарное сообщение от пользователя
            else if (parameters[1] == ClientWindow::MESSAGE_BINARY)
            {
                this->toConsole("Это текстовое соообщение от пользователя (отправляется перед отправкой бинарного).");
                this->toConsole("Полученные данные: Отправитель: " + parameters[2] + " Получатель: " +
                        QString(parameters[3] == ClientWindow::RECIEVER_ALL ? "Все" : parameters[3]) + " Имя файла: " + parameters[4]);

                this->ui->chatPlainTextEdit->appendHtml(
                        "<font color=\"teal\">[" +
                        parameters[2] + "/" +
                        this->getCurrentDateTime() + "/" +
                        QString(parameters[3] == ClientWindow::RECIEVER_ALL ? "Все" : ("Только " +
                                QString(parameters[2] == this->ui->authLoginLineEdit->text() ? parameters[3] : "Вам"))) + "]: " +
                        QString(parameters[2] == this->ui->authLoginLineEdit->text() ? "Отправлен файл: " : "Получен файл: ") + parameters[4] + "</font>");
            }
        }

    }
}

// Событие, возникающее при получении бинарного сообщения
void ClientWindow::eventBinaryTextMessageRecieved(const QByteArray& data)
{
    this->toConsole("Получено бинарное сообщение.");

    /*
     * Шаблон бинарного сообщения:
     * SENDER_SERVER MESSAGE_SEPARATOR sender MESSAGE_SEPARATOR reciever MESSAGE_SEPARATOR filename MESSAGE_SEPARATOR data (5 элементов)
     */
    QStringList parameters = QString::fromUtf8(data).split(ClientWindow::MESSAGE_SEPARATOR);

    // Если обнаружено минимум 5 аргументов, то шаблон запроса, вероятно, правильный
    if (parameters.size() >= 5)
    {
        this->toConsole("Корректное количество аргументов бинарного сообщения.");

        // Если сообщение пришло с сервера и получатель текущий клиент, то начинается сохранение полученного файла
        if (parameters[0] == ClientWindow::SENDER_SERVER && parameters[2] == this->ui->authLoginLineEdit->text())
        {
            this->toConsole("Бинарное сообщение готово к обработке со стороны клиента.");

            // Размер текстового запроса в бинарном сообщении
            int textPartSize =
                    parameters[0].size() + ClientWindow::MESSAGE_SEPARATOR.size() +
                    parameters[1].size() + ClientWindow::MESSAGE_SEPARATOR.size() +
                    parameters[2].size() + ClientWindow::MESSAGE_SEPARATOR.size() +
                    parameters[3].size() + ClientWindow::MESSAGE_SEPARATOR.size();

            // Байты файла для записи
            QByteArray dataToWrite;

            // Поиск байтов файла в бинарном сообщении
            for (int i = 0; i < data.size(); i++)
                if (i >= textPartSize)
                    dataToWrite.push_back(data[i]);

            this->toConsole("Сохранение полученного файла.");

            // Файлы будут сохраняться в домашнюю директорию
            QString directoryPath = this->getDirectorySaveFilesPath();
            QDir filesDirectory(directoryPath);

            // Если папки не существует - создаем её
            if (!filesDirectory.exists())
                filesDirectory.mkdir(directoryPath);

            // Разделяем имя и формат файла
            QString name = "", format = "";
            int lastPoint = parameters[3].lastIndexOf('.');

            for (int i = 0; i < parameters[3].size(); i++)
                if (i < lastPoint || lastPoint <= 0)
                    name += parameters[3][i];
                else
                    format += parameters[3][i];

            // Открываем файл с полученным именем плюс именем отправителя и временем получения и записываем в него данные
            QFile file(directoryPath + name + "_" + parameters[1] + "_" + this->getCurrentDateTime("_") + format);
            file.open(QIODevice::WriteOnly);
            file.write(dataToWrite);
            file.close();
        }
        else
            this->toConsole("Некорректный запрос на сохранение файла.");
    }
    else
        this->toConsole("Некорректное количество аргументов бинарного сообщения.");
}

// Событие при изменении текста в поле ввода сообщения
void ClientWindow::eventActionsSendMessagePlainTextEditTextChanged()
{
    QString text = this->ui->actionsSendMessagePlainTextEdit->document()->toPlainText();
    this->ui->actionsSendMessagePushButton->setEnabled(text.size() > 0 && text.size() <= ClientWindow::MESSAGE_MAX_SIZE);
}

// Событие при нажатии кнопки отправки сообщения
void ClientWindow::eventActionsSendMessagePushButtonClicked(bool clicked)
{
    QString message =
            ClientWindow::SENDER_CLIENT + ClientWindow::MESSAGE_SEPARATOR +
            ClientWindow::MESSAGE_TEXT + ClientWindow::MESSAGE_SEPARATOR +
            this->ui->authLoginLineEdit->text() + ClientWindow::MESSAGE_SEPARATOR +
            QString(this->currentUserListWidgetItem != nullptr ?
                this->currentUserListWidgetItem->text() : ClientWindow::RECIEVER_ALL) + ClientWindow::MESSAGE_SEPARATOR +
            this->ui->actionsSendMessagePlainTextEdit->document()->toPlainText();


    this->ui->actionsSendMessagePlainTextEdit->setPlainText("");
    this->clientWebSocket.sendTextMessage(message);
}

// Событие при нажатии на элемент списка пользователей
void ClientWindow::eventUserListWidgetItemClicked(QListWidgetItem* item)
{
    bool check = item != this->currentUserListWidgetItem;

    if (check)
        this->currentUserListWidgetItem = item;
    else
    {
        this->currentUserListWidgetItem = nullptr;
        this->ui->userListWidget->setCurrentItem(nullptr);
    }

    this->ui->actionsSendFilePushButton->setEnabled(check);
}


// Событие при нажатии кнопки отправки файла
void ClientWindow::eventActionsSendFilePushButtonClicked(bool clicked)
{
    this->toConsole("Открытие файла для отправления.");

    // Окно выбора файла
    QFileDialog dialog;
    dialog.resize(640, 480);

    // Открываем файл на чтение
    QString path = dialog.getOpenFileName(this, tr("Выбрать файл"), tr("Все файлы (*)"));
    QFile file(path);

    this->toConsole("Размер полученного файла: " + QString::number(file.size()));

    // Если файл не превышает заданное количество байт, то отправляет его выбранному пользователю
    if (file.size() <= ClientWindow::FILE_MAX_SIZE)
    {
        this->toConsole("Размер файла не превышает " + QString::number(ClientWindow::FILE_MAX_SIZE) + " байт.");

        file.open(QIODevice::ReadOnly);
        QByteArray data = file.readAll();
        QString fileName = QFileInfo(path).fileName();

        QString message =
                ClientWindow::SENDER_CLIENT + ClientWindow::MESSAGE_SEPARATOR +
                ClientWindow::MESSAGE_BINARY + ClientWindow::MESSAGE_SEPARATOR +
                this->ui->authLoginLineEdit->text() + ClientWindow::MESSAGE_SEPARATOR +
                this->currentUserListWidgetItem->text() + ClientWindow::MESSAGE_SEPARATOR +
                fileName;

        this->toConsole("Отправка файла пользователю.");

        this->clientWebSocket.sendTextMessage(message);
        this->clientWebSocket.sendBinaryMessage(
                    ClientWindow::SENDER_CLIENT.toUtf8() + ClientWindow::MESSAGE_SEPARATOR.toUtf8() +
                    this->ui->authLoginLineEdit->text().toUtf8() + ClientWindow::MESSAGE_SEPARATOR.toUtf8() +
                    this->currentUserListWidgetItem->text().toUtf8() + ClientWindow::MESSAGE_SEPARATOR.toUtf8() +
                    fileName.toUtf8() + ClientWindow::MESSAGE_SEPARATOR.toUtf8() +
                    data);
    }
    else
       this->toConsole("Размер файла превышает " + QString::number(ClientWindow::FILE_MAX_SIZE) + " байт.");

     file.close();
}

#ifndef CLIENTWINDOW_H
#define CLIENTWINDOW_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QListWidgetItem>
#include <QFileDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ClientWindow; }
QT_END_NAMESPACE

// Класс окна клиента чата
class ClientWindow : public QMainWindow
{
    Q_OBJECT

public:

    // Конструктор окна клиента чата
    ClientWindow(bool debug = true, QWidget* parent = nullptr);

    // Деструктор клиента чата
    ~ClientWindow();

    // Стандартный порт подключения к веб-сокету сервера
    static constexpr quint16 PORT_CONNECTION = 7200;

    // Стандартный порт подключения к UDP-сокету клиента
    static constexpr quint16 PORT_UDP_CLIENT = 7272;

    // Стандартный порт подключения к UDP-сокету сервера
    static constexpr quint16 PORT_UDP_SERVER = 2727;

    // Максимальный размер сообщения
    static constexpr int MESSAGE_MAX_SIZE = 1000;

    // Максимальный размер файла (30 Мбайт)
    static constexpr qint64 FILE_MAX_SIZE = 30 * 1024 * 1024;

    // Стандартный ответ клиента при поиске сервера
    const static QString SEARCH_CLIENT;

    // Стандартный ответ сервера клиенту
    const static QString SEARCH_SERVER;

    // Стандартный разделитель в ответе от сервера
    const static QString MESSAGE_SEPARATOR;

    // Команда клиенту отправить имя пользователя
    const static QString COMMAND_CLIENT_USERNAME;

    // Имя пользователя, если получатель - все пользователи
    const static QString RECIEVER_ALL;

    // Имя пользователя, если отправитель - сервер
    const static QString SENDER_SERVER;

    // Отправитель сообщения клиент
    const static QString SENDER_CLIENT;

    // Занятое имя пользователя
    const static QString USERNAME_ALREADY_EXISTS;

    // Подключен новый пользователь
    const static QString USERNAME_CONNECTED;

    // Пользователь отключен
    const static QString USERNAME_DISCONNECTED;

    // Добавить имя пользователя в список
    const static QString USERNAME_ADD_TO_LIST;

    // Сообщение от пользователя
    const static QString MESSAGE_TEXT;

    // Автоматическое техническое сообщение
    const static QString MESSAGE_TECHNICAL;

    // Файловое сообщение
    const static QString MESSAGE_BINARY;

    // Отправлен пустой файл
    const static QString FILE_NOT;

    // Сервер отключен
    const static QString SERVER_DISCONNECTED;

    // Управляет активностью кнопки подключения к сервера
    void doAuthSignInPushButtonEnabled();

    // Устанавливает текст для адреса сервера и управляет активностью связанных полей
    void doAuthServerIPv4LabelsSetText(const QString& text);

    // Устанавливает текст для адреса клиента и управляет активностью связанных полей
    void doAuthClientIPv4LabelsSetText(const QString& text);

    // Выводит сообщение в консоль
    void toConsole(const QString& text);

    // Посылает сообщения для поиска сервера в сети
    void doSendMessageToSearchServer();

    // Возвращает, введено ли корректное имя пользователя
    bool isCorrectUsername();

    // Включает основные элементы управления
    void doSetChatEnabled(bool check);

    // Возвращает текущие дату и время
    QString getCurrentDateTime(const QString& spearator = "/");

    // Возвращает папку для сохранения файлов
    QString getDirectorySaveFilesPath();

private Q_SLOTS:

    // Событие при изменении текста в поле ввода логина
    void eventAuthLoginLineEditTextChanged(const QString& text);

    // Событие при нажатии кнопки подключения к серверу
    void eventAuthSignInPushButtonClicked(bool clicked = false);

    // Событие, возникающее при получении текстового сообщения
    void eventTextMessageRecieved(const QString& data);

    // Событие, возникающее при получении бинарного сообщения
    void eventBinaryTextMessageRecieved(const QByteArray& data);

    // Событие, возникающее при завершении подключения веб-сокета клиента к серверу
    void eventConnected();

    // Событие, возникающее при поиске сервера (получение данных в UDP-сокет)
    void eventSearchServer();

    // Событие, возникающее при завершении подключения веб-сокета клиента к серверу
    void eventDisconnected();

    // Событие при изменении текста в поле ввода сообщения
    void eventActionsSendMessagePlainTextEditTextChanged();

    // Событие при нажатии кнопки отправки сообщения
    void eventActionsSendMessagePushButtonClicked(bool clicked = false);

    // Событие при нажатии на элемент списка пользователей
    void eventUserListWidgetItemClicked(QListWidgetItem* item);

    // Событие при нажатии кнопки отправки файла
    void eventActionsSendFilePushButtonClicked(bool clicked = false);

private:

    // Указатель на окно клиента чата
    Ui::ClientWindow* ui;

    // Веб-сокет клиента
    QWebSocket clientWebSocket;

    // UDP-сокет для поиска сервера
    QUdpSocket searchUdpSocket;

    // URL сервера
    QUrl serverUrl;

    // Найден ли адрес сервера
    bool serverAddressFound;

    // Дата и время подключения к серверу
    QString connectionDateTime;


    // Выбран режим подробных сообщений
    bool debug;

    // Текущий пользователь для отправки
    QListWidgetItem* currentUserListWidgetItem;
};
#endif // CLIENTWINDOW_H

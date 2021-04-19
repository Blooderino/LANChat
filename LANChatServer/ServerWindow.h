#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QTime>
#include <QDate>
#include <QListWidgetItem>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

QT_BEGIN_NAMESPACE
namespace Ui { class ServerWindow; }
QT_END_NAMESPACE

// Класс окна сервера чата
class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:

    // Конструктор класса
    ServerWindow(bool debug = true, quint16 port = ServerWindow::PORT_CONNECTION, QWidget *parent = nullptr);

    // Деструктор класса
    ~ServerWindow();

    // Стандартный порт подключения к веб-сокету сервера
    static constexpr quint16 PORT_CONNECTION = 7200;

    // Стандартный порт подключения к UDP-сокету клиента
    static constexpr quint16 PORT_UDP_CLIENT = 7272;

    // Стандартный порт подключения к UDP-сокету сервера
    static constexpr quint16 PORT_UDP_SERVER = 2727;

    // Стандартный ответ клиента при поиске сервера
    const static QString SEARCH_CLIENT;

    // Стандартный ответ сервера клиенту
    const static QString SEARCH_SERVER;

    // Стандартный разделитель в ответе от сервера
    const static QString MESSAGE_SEPARATOR;

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

    // Имя пользователя не обнаружено
    const static QString USERNAME_NOT_FOUND;

    // Добавить имя пользователя в список
    const static QString USERNAME_ADD_TO_LIST;

    // Текстовое сообщение
    const static QString MESSAGE_TEXT;

    // Автоматическое техническое сообщение
    const static QString MESSAGE_TECHNICAL;

    // Файловое сообщение
    const static QString MESSAGE_BINARY;

    // Отправлен пустой файл
    const static QString FILE_NOT;

    // Сервер отключен
    const static QString SERVER_DISCONNECTED;

    // Возвращает найденный веб-сокет клиента
    QWebSocket* getClientWebSocketByUserName(const QString& userName);

    // Возвращает индекс веб-сокета клиента в общем списке
    int getIndexOfClientWebSocket(QWebSocket* clientWebSocket);

    // Возвращает адрес сервера, который находится в той же под сети
    QString findServerAddress(const QString& address);

    // Выводит сообщение в консоль
    void toConsole(const QString& text);

    // Возвращает, введено ли корректное имя пользователя
    bool isCorrectUsername(const QString& userName, const QString& connectionDateTime);

    // Возвращает текущие даут и время
    QString getCurrentDateTime(const QString& spearator = "/");

private Q_SLOTS:

    // Событие при подключении веб-соекта нового клиента
    void eventClientSocketConnected();

    // Событие при получении текстового сообщения сервером
    void eventClientTextMessageRecieved(const QString& data);

    // Событие при получении бинарного сообщения сервером
    void eventClientBinaryMessageRecieved(const QByteArray& data);

    // Событие при отключении веб-сокета существующего клиента
    void eventClientSocketDisconnected();

    // Событие, возникающее при поиске сервера (получение данных в UDP-сокет)
    void eventSearchServer();

    // Событие при нажатии на элемент списка пользователей
    void eventUsersListWidgetItemClicked(QListWidgetItem* item);

private:

    // Текущий порт подключения веб-сокета
    QString currentPort;

    // Указатель на окно сервера чата
    Ui::ServerWindow *ui;

    // Веб-сокет сервера
    QWebSocketServer* connectionWebSocketServer;

    // Локальный веб-сокет для посылки сообщений клиентам
    QWebSocket* serverWebSocket;

    // Список веб-сокетов клиентов
    QList<QPair<QWebSocket*, QString>> clientsWebSockets;

    // UDP-сокет для поиска сервера
    QUdpSocket searchUdpSocket;

    // Конец ключа шифрования
    QString startDateTime;

    // Выбран режим подробных сообщений
    bool debug;

    // Текущий пользователь для отправки
    QListWidgetItem* currentUserListWidgetItem;
};
#endif // SERVERWINDOW_H

#include <QRandomGenerator> //RNG handler
#include <QMessageBox> //Send message
#include <QTouchEvent> //Touch screen handler
#include <QMovie> //Sprite .gif handler
#include <QTcpSocket> //Socket handler
#include <QEvent>
#include <QTouchEvent> //
#include <QDataStream> //Send & receive data
#include <QDebug> //Debug messages
#include "embedmonclient.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    clientSocket = new QTcpSocket(this);
    connectToServer("127.0.0.1", 9999); // IP is static iPv4 address for the server, CHANGE to 192.169.1.2 before deploying to BBB

    /*Setup background image*/
    QPixmap backgroundPixmap(":/images/battlebackground.png");
    QPalette palette;
    palette.setBrush(QPalette::Window, backgroundPixmap);
    this->setAutoFillBackground(true);
    this->setPalette(palette);

    /*Manual signal-slot connection for clientSocket*/
    connect(clientSocket, &QTcpSocket::connected, this, &Widget::onConnected);
    connect(clientSocket, &QTcpSocket::readyRead, this, &Widget::handleDataFromServer);
    connect(clientSocket, &QTcpSocket::errorOccurred, this, &Widget::onError);

    /*Manual signal-slot connection for buttons*/
    connect(ui->lattackP2, &QPushButton::clicked, this, &Widget::lattackP2_clicked);
    connect(ui->sattackP2, &QPushButton::clicked, this, &Widget::sattackP2_clicked);
    connect(ui->blockP2, &QPushButton::clicked, this, &Widget::blockP2_clicked);
    connect(ui->potionP2, &QPushButton::clicked, this, &Widget::potionP2_clicked);

    /*QMovie implementation */
    // Initialize QLabel pointers for the sprites
    player1Sprite = new QLabel(this);
    player2Sprite = new QLabel(this);
    //Load gif animations
    QMovie *player1Movie = new QMovie(":/sprites/RockyIdle_fr.gif");
    QMovie *player2Movie = new QMovie(":/sprites/FreezeIdle_fl.gif");
    //Set movie to Qlabel objects
    player1Sprite->setMovie(player1Movie);
    player2Sprite->setMovie(player2Movie);
    //Start animation
    player1Movie->start();
    player2Movie->start();
    player1Sprite->move(130, 150);
    player2Sprite->move(260, 150);

    /*Setup delay timer */
    delayTimer = new QTimer(this);
    connect(delayTimer, &QTimer::timeout, this, &Widget::playAnimations);

}

/*Function to initialize server connection */
void Widget::connectToServer(const QString &serverIP, quint16 serverPort) {
    qDebug() << "Attempting to connect to server...";
    clientSocket->connectToHost(serverIP, serverPort);
}

/* Once connected, display an initial message on the terminal */
void Widget::onConnected() {
    qDebug() << "Connected to server!";
}

/*Update health*/
void Widget::handleDataFromServer() {
    // Read the data from the server
    QByteArray data = clientSocket->readAll();
    if (data.size() >= static_cast<int>(2 * sizeof(int) + 2 * sizeof(Action))) {
        // Process the received data
        QDataStream stream(&data, QIODevice::ReadOnly);
        int player1ActionInt, player2ActionInt;
        stream >> player1.health >> player2.health >> player1ActionInt >> player2ActionInt;

        player1Action = static_cast<Action>(player1ActionInt);
        player2Action = static_cast<Action>(player2ActionInt);

        // Update health bars
        ui->healthbarP1->setValue(player1.health);
        ui->healthbarP2->setValue(player2.health);

        // Play the received animations
        playAnimations();
    } else {
        qWarning() << "Received incomplete data from the server";
    }
}

/*Send data to server*/
void Widget::sendDataToServer(const QByteArray &data) {
    // Check if there is a valid connection
    if (clientSocket && clientSocket->state() == QAbstractSocket::ConnectedState) {
        // Send the data to the server
        clientSocket->write(data);
    }
}

/*Error message conditional function */
void Widget::onError(QAbstractSocket::SocketError socketError) {
    qWarning() << "Socket error:" << socketError << clientSocket->errorString();
}

/*Touch Screen Implementation */
bool Widget::event(QEvent *event) {
    if (event->type() == QEvent::TouchBegin ||
        event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd) {
        handleTouchEvent(static_cast<QTouchEvent *>(event));
        return true;
    }
    return QWidget::event(event);
}

void Widget::handleTouchEvent(QTouchEvent *event) {
    const QList<QTouchEvent::TouchPoint> &touchPoints = event->touchPoints();

    for (const QTouchEvent::TouchPoint &touchPoint : touchPoints) {
        QPointF pos = touchPoint.pos();

        if (touchPoint.state() & Qt::TouchPointPressed) {
            /*) Check if the touch point is within the bounding rectangle of each QPushButton */
            if (ui->lattackP2->geometry().contains(pos.toPoint())) {
                ui->lattackP2->click();
            } else if (ui->sattackP2->geometry().contains(pos.toPoint())) {
                ui->sattackP2->click();
            } else if (ui->blockP2->geometry().contains(pos.toPoint())) {
                ui->blockP2->click();
            } else if (ui->potionP2->geometry().contains(pos.toPoint())) {
                ui->potionP2->click();
            }
        }
    }
}

/* Handler for server-client connection */
void Widget::lattackP2_clicked() {
    QByteArray actionData("lattackP2");
    sendDataToServer(actionData);
}

void Widget::sattackP2_clicked() {
    QByteArray actionData("sattackP2");
    sendDataToServer(actionData);
}

void Widget::blockP2_clicked() {
    QByteArray actionData("blockP2");
    sendDataToServer(actionData);
}

void Widget::potionP2_clicked() {
    QByteArray actionData("potionP2");
    sendDataToServer(actionData);
}

void Widget::updateHealthBars() {
    ui->healthbarP1->setValue(player1.health);
    ui->healthbarP2->setValue(player2.health);
}

void Widget::playAnimations() {
    // Stop the timer
    delayTimer->stop();

    // Update the QMovie instances with the attack animations or idle animations based on player actions
    QMovie *player1Movie;
    QMovie *player2Movie;

    if (player1Action == Action::LightAttack || player1Action == Action::StrongAttack) {
        player1Movie = new QMovie(":/sprites/RockyAttack_fr.gif");
    } else {
        player1Movie = new QMovie(":/sprites/RockyIdle_fr.gif");
    }

    if (player2Action == Action::LightAttack || player2Action == Action::StrongAttack) {
        player2Movie = new QMovie(":/sprites/FreezeAttack_fl.gif");
    } else {
        player2Movie = new QMovie(":/sprites/FreezeIdle_fl.gif");
    }

    player1Sprite->setMovie(player1Movie);
    player2Sprite->setMovie(player2Movie);

    // Start the animations
    player1Movie->start();
    player2Movie->start();

    // Reset the animations back to idle after a short delay (2000 ms = 2 seconds)
    QMetaObject::invokeMethod(this, "resetIdleAnimations", Qt::QueuedConnection, Q_ARG(int, 2000));
}

void Widget::resetIdleAnimations(int delay) {
    QTimer::singleShot(delay, [this] {
        QMovie *player1IdleMovie = new QMovie(":/sprites/RockyIdle_fr.gif");
        QMovie *player2IdleMovie = new QMovie(":/sprites/FreezeIdle_fl.gif");

        player1Sprite->setMovie(player1IdleMovie);
        player2Sprite->setMovie(player2IdleMovie);

        player1IdleMovie->start();
        player2IdleMovie->start();
    });
}

Widget::~Widget()
{
    delete ui;
}

#ifndef APPOINTMENT_CONTROLLER_H
#define APPOINTMENT_CONTROLLER_H

#include <QObject>
#include "../network/server_core.h"
#include "../network/client_handler.h"
#include <QJsonObject>

class AppointmentController : public QObject
{
    Q_OBJECT
public:
    explicit AppointmentController(ServerCore *core, QObject *parent = nullptr);

private:
    void handleGetAppointments(ClientHandler *client, const QJsonObject &body);
    void handleAddAppointment(ClientHandler *client, const QJsonObject &body);
    void handleUpdateStatus(ClientHandler *client, const QJsonObject &body);
};

#endif // APPOINTMENT_CONTROLLER_H

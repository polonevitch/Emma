#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <QThread>
#include <QTextStream>

class console : public QThread
{
public:
    console();
    void run() override;
    QString getLatestCommand();
    void writeMessage(QString msg);
    void stopThread();

protected:
    QString lastCommand;
    bool stop;
};

#endif // CONSOLE_H

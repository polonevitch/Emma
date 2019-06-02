#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <QThread>
#include <QTextStream>

class Counter : public QThread
{
    Q_OBJECT

public:
    Counter();

    int value() const { return m_value; }

public slots:
    void setValue(int value);

signals:
    void valueChanged(int newValue);

private:
    int m_value;
};

class console : public QThread
{
    Q_OBJECT

public:
    console();
    void run() override;
    QString getLatestCommand();
    void writeMessage(QString msg);
    void stopThread();

signals:
    void keyPressed();

protected:
    QString lastCommand;
    bool stop;
};

#endif // CONSOLE_H

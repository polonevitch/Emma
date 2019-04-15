#include "console.h"

console::console()
{
    stop = false;
}

void console::writeMessage(QString msg)
{
    QTextStream outS(stdout);
    outS << msg << endl;
}

void console::stopThread()
{
    stop = true;
    wait();
}

void console::run()
{
    stop = false;
    QTextStream inS(stdin);
    while(!stop)
        lastCommand = inS.readLine();
}

QString console::getLatestCommand()
{
    QString ret(lastCommand);
    lastCommand.clear();
    return ret;
}

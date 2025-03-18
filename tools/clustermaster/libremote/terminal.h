#pragma once

#include <util/generic/string.h>

#include <termios.h>

TString HiddenReadLineFromTty() {
    TString ret;

#ifdef _unix_
    if (!isatty(STDIN_FILENO))
        throw yexception() << "Stdin does not refer to terminal";

    struct termios oldTerminalInfo;
    memset(&oldTerminalInfo, 0, sizeof(oldTerminalInfo));

    tcgetattr(STDIN_FILENO, &oldTerminalInfo);

    struct termios newTerminalInfo(oldTerminalInfo);

    newTerminalInfo.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalInfo);

    char c = 0;
    while ('\n' != (read(STDIN_FILENO, &c, sizeof(c)), c))
        ret.append(c);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminalInfo);
#else
    throw yexception() << "Only UNIX platform is supported";
#endif

    return ret;
}

#ifndef SQSCRIPTER_H
#define SQSCRIPTER_H

/// \brief Initialize the scripter system with command and print procedures
void scripter_init(void commandProc(const char*), void (**printProc)(const char*));

/// \brief Show the scripter window
int scripter_show();


#endif

// command_handler.h
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

// Funções para tratar os comandos
const char* handle_start(const char* request);
const char* handle_try();
const char* handle_show_trials();
const char* handle_scoreboard();
const char* handle_debug();
const char* handle_quit();

#endif // COMMAND_HANDLER_H

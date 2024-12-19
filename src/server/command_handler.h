// command_handler.h
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "state.h"
#include "game.h"
#include "GS.h"
#include "./../common/verifications.h"

// Funções para tratar os comandos
const char* handle_start(const char* request);
const char* handle_try(const char* request);
const char* handle_show_trials(const char* request);
const char* handle_scoreboard();
const char* handle_debug(const char* request);
const char* handle_quit(const char* request);

#endif // COMMAND_HANDLER_H

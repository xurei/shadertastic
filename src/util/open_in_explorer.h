//
// Created by olivier on 26/08/26.
//

#ifndef SHADERTASTIC_OPEN_IN_EXPLORER_H
#define SHADERTASTIC_OPEN_IN_EXPLORER_H

#include <obs-module.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#include <spawn.h>
#include <sys/types.h>
extern char **environ;
#endif

bool open_in_explorer(const char *path);

#endif //SHADERTASTIC_OPEN_IN_EXPLORER_H

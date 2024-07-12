#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <lapi.h>
#include <lua.h>

#include "event.h"
#include "../player.h"

int command_text(Command *command);
int command_test(Command *command);

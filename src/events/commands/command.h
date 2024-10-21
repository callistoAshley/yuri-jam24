#pragma once

// defined in a separate header so we don't need to forward declare things
typedef enum
{
    CMD_Printf = 0,
    CMD_Text,
    CMD_Wait,
    CMD_Yield,
    CMD_Unimplemented,

    Command_Max_Val,
} Command;

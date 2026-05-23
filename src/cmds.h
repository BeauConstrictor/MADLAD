#ifndef CMDS_H
#define CMDS_H

#include "ed.h"

void cmd_run(ed_editor *ed, char *cmd);
void cmd_init();
void cmd_finished();

#endif // CMDS_H

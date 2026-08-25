#include <stdbool.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

#define C_FLAGS "-O2", "-Wall", "-Wextra"
#define BUILD_FOLDER "build/"

bool build_win(Nob_Cmd* cmd);
bool build_unix(Nob_Cmd* cmd);

int main(int argc, char** argv)
{
   NOB_GO_REBUILD_URSELF(argc, argv);
   nob_mkdir_if_not_exists(BUILD_FOLDER);
   char* cc;
   // default gcc
   cc = "gcc"; //TODO: add opts
   Nob_Cmd cmd = {0};
   nob_cmd_append(&cmd, cc, C_FLAGS);
#if _WIN32
   build_win(&cmd);
#else
   build_unix(&cmd);
#endif
   if (!nob_cmd_run(&cmd)) return 1;
   return 0;
}

bool build_win(Nob_Cmd* cmd)
{
   NOB_TODO("NOT IMPLEMENTED build_win()\n");
}

bool build_unix(Nob_Cmd* cmd)
{
   NOB_TODO("NOT IMPLEMENTED build_unix()\n");
}

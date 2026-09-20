#include <stdbool.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

#define C_FLAGS "-O2", "-Wall", "-Wextra"
#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"
#define PROJECT_LIB "libjoan"

bool build_win(Nob_Cmd* cmd, const char* cc);
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
   if (!build_win(&cmd, cc)) return 1;
#else
   build_unix(&cmd);
#endif
   printf("Done\n");
   return 0;
}

bool build_win(Nob_Cmd* cmd, const char* cc)
{
   nob_cmd_append(cmd, cc, C_FLAGS, "-DJN_BUILD_DLL", "Iinclude", "-c", SRC_FOLDER"bundle.c");
   if (!nob_cmd_run(&cmd)) return false;
   cmd->count = 0;
   nob_cmd_append(cmd, cc, C_FLAGS, "-shared", "-Wl,--output-def=" BUILD_FOLDER PROJECT_LIB ".def", "-Wl,--out-implib=" BUILD_FOLDER PROJECT_LIB ".a", "-Wl,--dll", "*.o", "-o", BUILD_FOLDER PROJECT_LIB ".dll", "-static-libgcc", "-static");
   if (!nob_cmd_run(&cmd)) return false;
   cmd->count = 0;
   nob_cmd_append(cmd, cc, C_FLAGS, "Iinclude", "-c", SRC_FOLDER "main.c");
   if (!nob_cmd_run(&cmd)) return false;
   cmd->count = 0;
   nob_cmd_append(cmd, cc, C_FLAGS, "main.o", "-o", BUILD_FOLDER "joan.exe", "-static-libgcc", "-static", "-L" BUILD_FOLDER, "-lm", "-ljoan");
   if (!nob_cmd_run(&cmd)) return false;
   cmd->count = 0;
   return true;
}

bool build_unix(Nob_Cmd* cmd)
{
   NOB_TODO("NOT IMPLEMENTED build_unix()\n");
}

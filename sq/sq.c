/*  see copyright notice in squirrel.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <Windows.h>
#include <io.h>

#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>

void PrintFunction(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar* s, ...)
{
    va_list vl;
    va_start(vl, s);
    vfprintf(stdout, s, vl);
    va_end(vl);
}

void ErrorFunction(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar* s, ...)
{
    va_list vl;
    va_start(vl, s);
    vfprintf(stderr, s, vl);
    va_end(vl);
}

SQInteger SQ_include_script(HSQUIRRELVM v) {
    sq_pushinteger(v, 1);
    return 1;
}

SQInteger register_global_func(HSQUIRRELVM v, SQFUNCTION f, const char* fname)
{
    sq_pushroottable(v);
    sq_pushstring(v, fname, -1);
    sq_newclosure(v, f, 0); //create a new function
    sq_newslot(v, -3, SQFalse);
    sq_pop(v, 1); //pops the root table
    return 0;
}

time_t get_file_modified_date(const char* szFilePath) {
    struct _stat fileStat;

    if (stat(szFilePath, &fileStat) == 0)
        return fileStat.st_mtime;

    return 0;
}


int main(int argc, char* argv[])
{
    char szScriptPath[128];
    strcpy(szScriptPath, argc > 1 ? argv[1] : "script.nut");

    int c = 0;
    while (1) {
        system("cls");
        printf("[#%d] Compiling \"%s\"... \n", c++, szScriptPath);

        HSQUIRRELVM v;
        v = sq_open(1024);
        sq_setprintfunc(v, PrintFunction, ErrorFunction);
        sq_pushroottable(v);

        sqstd_register_bloblib(v);
        sqstd_register_iolib(v);
        sqstd_register_systemlib(v);
        sqstd_register_mathlib(v);
        sqstd_register_stringlib(v);

        sqstd_seterrorhandlers(v);

        register_global_func(v, SQ_include_script, "IncludeScript");

        int res = compile_file(v, "script.nut");
        if (!res) {
            printf("Failed to read script.nut\n");
        }

        sq_close(v);

        time_t lastModified = get_file_modified_date(szScriptPath);
        for (;;) {
            if (lastModified < get_file_modified_date(szScriptPath))
                break;

            Sleep(1000);
        }
    }

}

SQInteger read_script_file(SQUserPointer file)
{
    int ret;
    char c;
    if ((ret = fread(&c, sizeof(c), 1, (FILE*)file) > 0))
        return c;
    return 0;
}

int compile_file(HSQUIRRELVM v, const char* fileName) {
    FILE* f = fopen(fileName, "rb");
    if (f) {
        sq_compile(v, read_script_file, f, fileName, 1);

        SQInteger oldTop = sq_gettop(v);
        sq_pushroottable(v);
        if (SQ_FAILED(sq_call(v, 1, SQFalse, SQTrue)))
        {
            sq_settop(v, oldTop);
            printf("Failed to call script function");
            return 0;
        }

        fclose(f);
        return 1;
    }
    return 0;
}

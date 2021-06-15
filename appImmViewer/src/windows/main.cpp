#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <windows.h>
#include <shellapi.h>

extern int    piMainFunc( const wchar_t *path, const wchar_t **args, int numArgs, void *instance );
int WINAPI wWinMain( HINSTANCE instance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    SetProcessDPIAware();

    // remove quotes from command line
    if( lpCmdLine )
    {
        if( lpCmdLine[0]==L'"' )
        {
            int i; for( i=0; lpCmdLine[i]; i++ ); lpCmdLine[i-1]=0; lpCmdLine++;
        }
    }

    int numArgs = 0;
    wchar_t **args = CommandLineToArgvW(GetCommandLineW(), &numArgs);

    wchar_t buffer[1024];
    GetCurrentDirectory(1024, buffer);

    // get executable file path
    GetModuleFileName( instance, buffer, 1024 );
    int ls=0; for( int i=0; buffer[i]; i++ ) if( buffer[i]==L'/' || buffer[i]==L'\\' ) ls=i; buffer[ls] = 0;

    SetCurrentDirectory(buffer);

    int res = piMainFunc(buffer, (const wchar_t**)args, numArgs, instance);

    LocalFree(args);

    return res;
}

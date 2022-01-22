#include <stdio.h>
#include <windows.h>
#include <objbase.h>

#define LOG_MESSAGE(...) printf("empty: " __VA_ARGS__);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    (void)hModule; // Unused
    (void)lpReserved; // Unused
    static BOOL didAllocConsole = FALSE;
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        didAllocConsole = AllocConsole();
        freopen("CONOUT$", "w", stdout);

        // FIXME: for my convenience, let's put the console in
        //        a handy location.
        HWND hwnd = GetConsoleWindow();
        RECT rect;
        GetWindowRect(hwnd, &rect);
        int left = 1930;
        int top = 10;
        int width = (int)(rect.right - rect.left);
        int height = (int)(rect.bottom - rect.top);
        MoveWindow(hwnd, left, top, width, height, TRUE);

        LOG_MESSAGE("DLL_PROCESS_ATTACH\n");
    } break;
    case DLL_PROCESS_DETACH: {
        LOG_MESSAGE("DLL_PROCESS_DETACH\n");
        if (didAllocConsole) {
            FreeConsole();
        }
    } break;
    // case DLL_THREAD_ATTACH: {
    //     printf(PREFIX "DLL_THREAD_ATTACH\n");
    //     printf(PREFIX "current thread: %u\n", (unsigned int)GetCurrentThreadId());
    // } break;
    // case DLL_THREAD_DETACH: {
    //     printf(PREFIX "DLL_THREAD_DETACH\n");
    //     printf(PREFIX "current thread: %u\n", (unsigned int)GetCurrentThreadId());
    // } break;
    }
    return TRUE;
}

// TEMP - until we have better definitions in place
typedef struct sScrClassDesc sScrClassDesc;
typedef struct tScrIter tScrIter;
typedef struct sScrMsg sScrMsg;
typedef struct sMultiParm sMultiParm;
typedef enum eScrTraceAction eScrTraceAction;
typedef struct IScriptMan IScriptMan;
typedef void (*MPrintfProc)(void);

DEFINE_GUID(IID_IScriptModule,                   0xd40000d4, 0x7b54, 0x12a3, 0x83, 0x48, 0x00, 0xaa, 0x00, 0xa8, 0x2b, 0x51);
DEFINE_GUID(IID_IScript,                         0xd00000d0, 0x7b50, 0x129f, 0x83, 0x48, 0x00, 0xaa, 0x00, 0xa8, 0x2b, 0x51);

#define IUNKNOWN_METHODS \
    STDMETHOD(QueryInterface)(THIS_ REFIID,void**) PURE; \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE; \
    STDMETHOD_(ULONG,Release)(THIS) PURE;

#define INTERFACE IScriptModule
DECLARE_INTERFACE_(IScriptModule, IUnknown)
{
    BEGIN_INTERFACE

    IUNKNOWN_METHODS

    STDMETHOD_(const char*,GetName)(THIS) PURE;
    STDMETHOD_(const sScrClassDesc*,GetFirstClass)(THIS_ tScrIter*) PURE;
    STDMETHOD_(const sScrClassDesc*,GetNextClass)(THIS_ tScrIter*) PURE;
    STDMETHOD_(void,EndClassIter)(THIS_ tScrIter*) PURE;

    END_INTERFACE

    int foo;
};
#undef INTERFACE

#define INTERFACE IScript
DECLARE_INTERFACE_(IScript, IUnknown)
{
    BEGIN_INTERFACE

    IUNKNOWN_METHODS

    STDMETHOD_(const char*,GetClassName)(void) PURE;
    STDMETHOD_(long,ReceiveMessage)(sScrMsg*,sMultiParm*,eScrTraceAction) PURE;

    END_INTERFACE

    ULONG refCount;
};
#undef INTERFACE

__declspec(dllexport) __stdcall 
int ScriptModuleInit(const char* pszName, 
                    IScriptMan* pScriptMan,
                    MPrintfProc pfnMPrintf,
                    IMalloc* pMalloc,
                    IScriptModule** pOutInterface)
{
    return FALSE;
}

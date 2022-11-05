
#ifdef WIN32

#define _WIN32_WINNT    0x0A01

#include <assert.h>
#include <windows.h>
#include <wincon.h>
#include <winbase.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

#include "mswinchild.h"

extern int option_mswin_cmd;

void CreateChildProcess (void);
void WriteToPipe (void);
void ReadFromPipe (void);
void ErrorExit (PTSTR);

static char *wchar_to_char (const wchar_t *w)
{
    char *r;
    size_t c, n;

    n = wcslen (w) + 2;
    r = (char *) malloc (n * 2);
    c = 0;
    wcstombs_s (&c, r, n * 2, w, _TRUNCATE);
    r[c] = '\0';

    return r;
}

static const char *mswin_error_to_text (long error)
{
    wchar_t *s = NULL;
    static char r[1024];
    char *p;
    int l;
    FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) & s, 0, NULL);
    if (!s)
        return "unknown error";
    p = wchar_to_char (s);
    LocalFree (s);
    l = strlen (p);
    if (l > sizeof (r) - 1)
        l = sizeof (r) - 1;
    memcpy (r, p, l);
    r[l] = '\0';
    free (p);
/* strip trailing whitespace, newlines, and carriage returns: */
    while (l >= 0 && ((unsigned char *) r)[l] <= ' ')
        r[l--] = '\0';
    return r;
}

void cterminal_cleanup (struct cterminal *c)
{
    if (c->con_handle) {
        CloseHandle (c->con_handle);
        c->con_handle = NULL;
    }
    if (c->process_handle) {
        CloseHandle (c->process_handle);
        c->process_handle = NULL;
    }
}

int child_exitted (MSWIN_HANDLE h)
{
    unsigned long exit_status = 0;
    if (!h)
        return 1;
    if (!GetExitCodeProcess (h, &exit_status))
        return 0;
    return exit_status != STILL_ACTIVE;
}

int kill_child_get_exit_status (int pid, MSWIN_HANDLE h, unsigned long *exit_status)
{
    int r = 0;
    if (h) {
        if (GetExitCodeProcess (h, exit_status))
            r = 1;
        TerminateProcess (h, 1);
    }
    return r;
}


static volatile long PipeSerialNumber = 1;

BOOL APIENTRY
MyCreatePipeEx (OUT LPHANDLE lpReadPipe,
                OUT LPHANDLE lpWritePipe, IN LPSECURITY_ATTRIBUTES lpPipeAttributes, IN DWORD nSize, DWORD dwReadMode, DWORD dwWriteMode)
/*++
Routine Description:
    The CreatePipeEx API is used to create an anonymous pipe I/O device.
    Unlike CreatePipe FILE_FLAG_OVERLAPPED may be specified for one or
    both handles.
    Two handles to the device are created.  One handle is opened for
    reading and the other is opened for writing.  These handles may be
    used in subsequent calls to ReadFile and WriteFile to transmit data
    through the pipe.
Arguments:
    lpReadPipe - Returns a handle to the read side of the pipe.  Data
        may be read from the pipe by specifying this handle value in a
        subsequent call to ReadFile.
    lpWritePipe - Returns a handle to the write side of the pipe.  Data
        may be written to the pipe by specifying this handle value in a
        subsequent call to WriteFile.
    lpPipeAttributes - An optional parameter that may be used to specify
        the attributes of the new pipe.  If the parameter is not
        specified, then the pipe is created without a security
        descriptor, and the resulting handles are not inherited on
        process creation.  Otherwise, the optional security attributes
        are used on the pipe, and the inherit handles flag effects both
        pipe handles.
    nSize - Supplies the requested buffer size for the pipe.  This is
        only a suggestion and is used by the operating system to
        calculate an appropriate buffering mechanism.  A value of zero
        indicates that the system is to choose the default buffering
        scheme.
Return Value:
    TRUE - The operation was successful.
    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.
--*/
{
    HANDLE ReadPipeHandle, WritePipeHandle;
    DWORD dwError;
    char PipeNameBuffer[MAX_PATH];

    //
    // Only one valid OpenMode flag - FILE_FLAG_OVERLAPPED
    //

    if ((dwReadMode | dwWriteMode) & (~FILE_FLAG_OVERLAPPED)) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    //
    //  Set the default timeout to 120 seconds
    //

    if (nSize == 0) {
        nSize = 4096;
    }

    snprintf (PipeNameBuffer, sizeof (PipeNameBuffer),
              "\\\\.\\Pipe\\RemoteExeAnon.%08lx.%08lx",
              (unsigned long) GetCurrentProcessId (), (unsigned long) InterlockedIncrement (&PipeSerialNumber)
        );

    ReadPipeHandle = CreateNamedPipeA (PipeNameBuffer, PIPE_ACCESS_INBOUND | dwReadMode, PIPE_TYPE_BYTE | PIPE_WAIT, 1, // Number of pipes
                                       nSize,   // Out buffer size
                                       nSize,   // In buffer size
                                       120 * 1000,      // Timeout in ms
                                       lpPipeAttributes);

    if (!ReadPipeHandle) {
        return FALSE;
    }

    WritePipeHandle = CreateFileA (PipeNameBuffer, GENERIC_WRITE, 0,    // No sharing
                                   lpPipeAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | dwWriteMode, NULL   // Template file
        );

    if (INVALID_HANDLE_VALUE == WritePipeHandle) {
        dwError = GetLastError ();
        CloseHandle (ReadPipeHandle);
        SetLastError (dwError);
        return FALSE;
    }

    *lpReadPipe = ReadPipeHandle;
    *lpWritePipe = WritePipeHandle;
    return (TRUE);
}

// Initializes the specified startup info struct with the required properties and
// updates its thread attribute list with the specified ConPTY handle
static HRESULT InitializeStartupInfoAttachedToConPTY(STARTUPINFOEX* siEx, HPCON hPC)
{
    size_t size;

    siEx->StartupInfo.cb = sizeof(STARTUPINFOEX);

    // Create the appropriately sized thread attribute list
    InitializeProcThreadAttributeList(NULL, 1, 0, &size);

    static LPPROC_THREAD_ATTRIBUTE_LIST l = NULL;
    l = (LPPROC_THREAD_ATTRIBUTE_LIST) realloc (l, size);
    ZeroMemory (l, size);

    // Set startup info's attribute list & initialize it
    siEx->lpAttributeList = l;
    if (!InitializeProcThreadAttributeList(siEx->lpAttributeList, 1, 0, (PSIZE_T)&size)) {
        fprintf (stderr, "InitializeProcThreadAttributeList error %s\n", mswin_error_to_text (GetLastError ()));
        exit (0);
    }

    // Set thread attribute list's Pseudo Console to the specified ConPTY
    if (!UpdateProcThreadAttribute(siEx->lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hPC, sizeof(HPCON), NULL, NULL)) {
        fprintf (stderr, "UpdateProcThreadAttribute error %s\n", mswin_error_to_text (GetLastError ()));
        exit (0);
    }
    return S_OK;
}

int cterminal_run_command (struct cterminal *c, struct cterminal_config *config, int dumb_terminal, const char *log_origin_host,
                           char *const argv[], char *errmsg)
{
    HANDLE stdin_rd = NULL;
    HANDLE stdout_wr = NULL;
    SECURITY_ATTRIBUTES saAttr;

    saAttr.nLength = sizeof (SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

#define ERR(s)  do { snprintf(errmsg, CTERMINAL_ERR_MSG_LEN, "%s: %s\n", s, mswin_error_to_text (GetLastError ())); return -1; } while (0)

    if (!MyCreatePipeEx (&c->cmd_fd_stdout, &stdout_wr, &saAttr, 0, FILE_FLAG_OVERLAPPED, 0))
        ERR ("StdoutRd CreatePipe");

    if (!SetHandleInformation (c->cmd_fd_stdout, HANDLE_FLAG_INHERIT, 0))
        ERR ("Stdout SetHandleInformation");

    if (!MyCreatePipeEx (&stdin_rd, &c->cmd_fd_stdin, &saAttr, 0, 0, FILE_FLAG_OVERLAPPED))
        ERR ("Stdin CreatePipe");

    if (!SetHandleInformation (c->cmd_fd_stdin, HANDLE_FLAG_INHERIT, 0))
        ERR ("Stdin SetHandleInformation");

    char cmdline[2048];
    char current_dir[1024] = "";
    GetCurrentDirectory (sizeof (current_dir), current_dir);
    if (option_mswin_cmd) {
        snprintf (cmdline, sizeof (cmdline), "CMD");
    } else {
        snprintf (cmdline, sizeof (cmdline), "\"%s\\%s\" %s", current_dir, "BUSYBOX64", "bash");
    }
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOEX siStartInfo;
    BOOL bSuccess = FALSE;

    ZeroMemory (&piProcInfo, sizeof (PROCESS_INFORMATION));

    ZeroMemory (&siStartInfo, sizeof (STARTUPINFO));

    COORD coord;
    HPCON con = 0;

    coord.X = config->col;
    coord.Y = config->row;

    if (CreatePseudoConsole(coord, stdin_rd, stdout_wr, 0, &con) != S_OK)
        ERR ("Stdin SetHandleInformation");

    // Prepare the StartupInfoEx structure attached to the ConPTY.
    InitializeStartupInfoAttachedToConPTY(&siStartInfo, con);

    bSuccess = CreateProcess (NULL, cmdline,  // command line 
                              NULL,     // process security attributes 
                              NULL,     // primary thread security attributes 
                              TRUE,     // handles are inherited 
                              EXTENDED_STARTUPINFO_PRESENT,        // creation flags 
                              NULL,     // use parent's environment 
                              NULL,     // use parent's current directory 
                              &siStartInfo.StartupInfo,     // STARTUPINFO pointer 
                              &piProcInfo);     // receives PROCESS_INFORMATION 

    if (!bSuccess)
        ERR ("CreateProcess");

    c->cmd_pid = piProcInfo.dwProcessId;
    c->process_handle = piProcInfo.hProcess;
    c->con_handle = con;
    assert (c->cmd_pid != 0);

    CloseHandle (piProcInfo.hThread);
    CloseHandle (stdout_wr);
    CloseHandle (stdin_rd);

    return 0;
}

void cterminal_tt_winsize (struct cterminal *c, MSWIN_HANDLE con, int columns, int rows)
{
    COORD coord;
    coord.X = columns;
    coord.Y = rows;
    if (ResizePseudoConsole (c->con_handle, coord) != S_OK)
        fprintf (stderr, "ResizePseudoConsole error %s\n", mswin_error_to_text (GetLastError ()));
}


#endif  /* WIN32 */



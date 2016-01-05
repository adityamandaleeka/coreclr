//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>

#include "windefs.h"
#include "twowaypipe.h"

///////////REMOVE
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

#define PIPE_NAME_FORMAT_STR "/tmp/clr-debug-pipe-%d-%llu-%s"

BOOL
PROCGetUniqueTimeValueForProcess(DWORD processId, UINT64 *uniqueTimeValue)
{
    if (uniqueTimeValue == nullptr)
    {
        _ASSERTE(!"uniqueTimeValue argument cannot be null!");
        return FALSE;
    }

    *uniqueTimeValue = 0;

#if defined(__APPLE__)

    struct kinfo_proc info = {};
    size_t size = sizeof(info);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, processId };
    int ret = ::sysctl(mib, sizeof(mib)/sizeof(*mib), &info, &size, NULL, 0);

    if (ret == 0)
    {
        timeval procStartTime = info.kp_proc.p_starttime;
        long secondsSinceEpoch = procStartTime.tv_sec;

        *uniqueTimeValue = secondsSinceEpoch;
        return TRUE;
    }
    else
    {
        _ASSERTE(!"Failed to get start time of a process.");
        return FALSE;
    }

#elif defined(HAVE_PROCFS_CTL)

    // Here we read /proc/<pid>/stat file to get the start time for the process.
    // The value it returns is expressed in jiffies since boot time.

    // Making something like: /proc/123/stat
    char statFileName[100];

    INDEBUG(int chars = )
    snprintf(statFileName, sizeof(statFileName), "/proc/%d/stat", processId);
    _ASSERTE(chars > 0 && chars <= sizeof(statFileName));

    FILE *statFile = fopen(statFileName, "r");
    if (statFile == NULL) 
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    char *line = NULL;
    size_t lineLen = 0;
    ssize_t read;

    if ((read = getline(&line, &lineLen, statFile)) == -1)
    {
        _ASSERTE(!"Failed to getline from the stat file for a process.");
        return FALSE;
    }

    // Contents of the stat file up to and including the start time.
    int pid;
    char execName[PATH_MAX];
    char procstate;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned int procFlags;
    unsigned long minflt, cminflt, majflt, cmajflt, utime, stime;
    long cutime, cstime, priority, nice, num_threads, itrealvalue;
    unsigned long long starttime;

    int sscanfRet = sscanf(line, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu \n",
               &pid, &execName, &procstate, &ppid, &pgrp, &session, &tty_nr, &tpgid, &procFlags, &minflt, &cminflt
               , &majflt, &cmajflt, &utime, &stime, &cutime, &cstime, &priority, &nice, &num_threads, &itrealvalue
               , &starttime);

    if (sscanfRet != 22)
    {
        _ASSERTE(!"Failed to parse stat file contents with sscanf.");
        return FALSE;
    }

    free(line); // We didn't allocate line, but as per contract of getline we should free it
    fclose(statFile);

    *uniqueTimeValue = starttime;
    return TRUE;

#else
    _ASSERTE(!"Not implemented on this platform.");
#endif
}

static void GetPipeName(char *name, DWORD id, const char *suffix)
{
    UINT64 uniqueProcessTimeValue;
    BOOL ret = PROCGetUniqueTimeValueForProcess(id, &uniqueProcessTimeValue);
    _ASSERTE(ret == TRUE);

    int chars = snprintf(name, PATH_MAX, PIPE_NAME_FORMAT_STR, id, uniqueProcessTimeValue, suffix);
    _ASSERTE(chars > 0 && chars < PATH_MAX);
}

// Creates a server side of the pipe. 
// Id is used to create pipes names and uniquely identify the pipe on the machine. 
// true - success, false - failure (use GetLastError() for more details)
bool TwoWayPipe::CreateServer(DWORD id)
{
    _ASSERTE(m_state == NotInitialized);
    if (m_state != NotInitialized)
        return false;

    m_id = id;
    char inPipeName[PATH_MAX];
    char outPipeName[PATH_MAX];
    GetPipeName(inPipeName, id, "in");
    GetPipeName(outPipeName, id, "out");

    //TODO: REVIEW if S_IRWXU | S_IRWXG is the right access level in prof use
    if (mkfifo(inPipeName, S_IRWXU | S_IRWXG) == -1)
    {
        return false;
    }

    if (mkfifo(outPipeName, S_IRWXU | S_IRWXG) == -1)
    {
        remove(inPipeName);
        return false;
    }    

    m_state = Created;
    return true;
}


// Connects to a previously opened server side of the pipe.
// Id is used to locate the pipe on the machine. 
// true - success, false - failure (use GetLastError() for more details)
bool TwoWayPipe::Connect(DWORD id)
{
    _ASSERTE(m_state == NotInitialized);
    if (m_state != NotInitialized)
        return false;

    m_id = id;
    char inPipeName[PATH_MAX];
    char outPipeName[PATH_MAX];
    //"in" and "out" are switched deliberately, because we're on the client
    GetPipeName(inPipeName, id, "out");
    GetPipeName(outPipeName, id, "in");

    // Pipe opening order is reversed compared to WaitForConnection()
    // in order to avaid deadlock.
    m_outboundPipe = open(outPipeName, O_WRONLY);
    if (m_outboundPipe == INVALID_PIPE)
    {
        return false;
    }

    m_inboundPipe = open(inPipeName, O_RDONLY);
    if (m_inboundPipe == INVALID_PIPE)
    {
        close(m_outboundPipe);
        m_outboundPipe = INVALID_PIPE;
        return false;
    }

    m_state = ClientConnected;
    return true;

}

// Waits for incoming client connections, assumes GetState() == Created
// true - success, false - failure (use GetLastError() for more details)
bool TwoWayPipe::WaitForConnection()
{
    _ASSERTE(m_state == Created);
    if (m_state != Created)
        return false;

    char inPipeName[PATH_MAX];
    char outPipeName[PATH_MAX];
    GetPipeName(inPipeName, m_id, "in");
    GetPipeName(outPipeName, m_id, "out");

    m_inboundPipe = open(inPipeName, O_RDONLY);
    if (m_inboundPipe == INVALID_PIPE)
    {
        return false;
    }

    m_outboundPipe = open(outPipeName, O_WRONLY);
    if (m_outboundPipe == INVALID_PIPE)
    {
        close(m_inboundPipe);
        m_inboundPipe = INVALID_PIPE;
        return false;
    }

    m_state = ServerConnected;
    return true;
}

// Reads data from pipe. Returns number of bytes read or a negative number in case of an error.
// use GetLastError() for more details
// UNIXTODO - mjm 9/6/15 - does not set last error on failure
int TwoWayPipe::Read(void *buffer, DWORD bufferSize)
{
    _ASSERTE(m_state == ServerConnected || m_state == ClientConnected);

    int totalBytesRead = 0;
    int bytesRead;
    int cb = bufferSize;

    while ((bytesRead = (int)read(m_inboundPipe, buffer, cb)) > 0)
    {
        totalBytesRead += bytesRead;
        _ASSERTE(totalBytesRead <= bufferSize);
        if (totalBytesRead >= bufferSize)
        {
            break;
        }
        buffer = (char*)buffer + bytesRead;
        cb -= bytesRead;
    }

    return bytesRead == -1 ? -1 : totalBytesRead;
}

// Writes data to pipe. Returns number of bytes written or a negative number in case of an error.
// use GetLastError() for more details
// UNIXTODO - mjm 9/6/15 - does not set last error on failure
int TwoWayPipe::Write(const void *data, DWORD dataSize)
{
    _ASSERTE(m_state == ServerConnected || m_state == ClientConnected);

    int totalBytesWritten = 0;
    int bytesWritten;
    int cb = dataSize;

    while ((bytesWritten = (int)write(m_outboundPipe, data, cb)) > 0)
    {
        totalBytesWritten += bytesWritten;
        _ASSERTE(totalBytesWritten <= dataSize);
        if (totalBytesWritten >= dataSize)
        {
            break;
        }
        data = (char*)data + bytesWritten;
        cb -= bytesWritten;
    }

    return bytesWritten == -1 ? -1 : totalBytesWritten;
}

// Disconnect server or client side of the pipe.
// true - success, false - failure (use GetLastError() for more details)
bool TwoWayPipe::Disconnect()
{

    if (m_outboundPipe != INVALID_PIPE)
    {
        close(m_outboundPipe);
        m_outboundPipe = INVALID_PIPE;
    }

    if (m_inboundPipe != INVALID_PIPE)
    {
        close(m_inboundPipe);
        m_inboundPipe = INVALID_PIPE;
    }    

    if (m_state == ServerConnected || m_state == Created)
    {

        char inPipeName[PATH_MAX];
        GetPipeName(inPipeName, m_id, "in");
        remove(inPipeName);

        char outPipeName[PATH_MAX];
        GetPipeName(outPipeName, m_id, "out");
        remove(outPipeName);
    }

    m_state = NotInitialized;
    return true;
}


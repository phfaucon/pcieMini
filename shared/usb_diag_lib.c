/* Jungo Connectivity Confidential. Copyright (c) 2020 Jungo Connectivity Ltd.  https://www.jungo.com */

/**************************************************************************
 * File - USB_DIAG_LIB.C
 *
 * Utility functions for communication with USB devices
 * using WinDriver's API.
 *
 * Note: This code sample is provided AS-IS and as a guiding sample only.
 **************************************************************************/

#include <stdio.h>
#include "windrvr.h"
#include "wdu_lib.h"
#include "status_strings.h"
#include "utils.h"

#ifdef _USE_SPECIFIC_KERNEL_DRIVER_
    #undef WD_Open
    #define WD_Open WD_OpenKernelHandle
    #if defined(UNIX)
        #undef WD_FUNCTION
        #define WD_FUNCTION(wFuncNum,h,pParam,dwSize,fWait) \
            ((ULONG)ioctl((int)(h), wFuncNum, pParam))
    #endif
#endif

#include "usb_diag_lib.h"

#if !defined(ERR)
    #define ERR printf
#endif

#define TRANSFER_TIMEOUT 30000 /* In msecs */
#define PERF_STREAM_BUFFER_SIZE 5120000 /* In bytes */
#define PERF_DEVICE_TRANSFER_SIZE 256*1024 /* In bytes */
#define PERF_PERFORMANCE_SAMPLE_TIME 10000 /* In msecs */
#define PERF_TRANSFER_ITERATIONS 1500

typedef struct
{
    HANDLE Handle;
    WDU_PIPE_INFO *pPipe;
    PVOID pContext;
    BOOL fStopped;
    HANDLE hThread;
    DWORD dwError;
    BOOL fStreamMode;
    DWORD dwBytesToTransfer;
    DWORD dwOptions;
    BOOL fPrint;
} USB_LISTEN_PIPE;

/* Function: pipeType2Str()
     Returns a string identifying the pipe type
   Parameters:
     pipeType [in] pipe type -- USB_PIPE_TYPE enum value
   Return Value:
     String containing the description of the pipe type */
char *pipeType2Str(ULONG pipeType)
{
    char *res = "unknown";

    switch (pipeType)
    {
        case PIPE_TYPE_CONTROL:
            res = "Control";
            break;
        case PIPE_TYPE_ISOCHRONOUS:
            res = "Isochronous";
            break;
        case PIPE_TYPE_BULK:
            res = "Bulk";
            break;
        case PIPE_TYPE_INTERRUPT:
            res = "Interrupt";
            break;
    }

    return res;
}

/* Input of command from user */
static char line[256];

/* Function: StopListeningToPipe()
     Stops listening to a USB device pipe
   Parameters:
     pListenPipe [in] pointer to the pipe to which to stop listening
   Return Value:
     None */
void StopListeningToPipe(USB_LISTEN_PIPE *pListenPipe)
{
    if (!pListenPipe->hThread)
        return;

    printf("Stop listening to pipe\n");
    pListenPipe->fStopped = TRUE;

    if (pListenPipe->fStreamMode)
        WDU_StreamClose(pListenPipe->Handle);
    else
        WDU_HaltTransfer(pListenPipe->Handle, pListenPipe->pPipe->dwNumber);

    ThreadWait(pListenPipe->hThread);
    pListenPipe->hThread = NULL;
}

/* Function: PipeListenHandler()
     Callback function that listens to a pipe continuously when there is data
     available on the pipe
   Parameters:
     pParam [in] pointer to the pipe to which to listen
   Return Value:
     None */
void DLLCALLCONV PipeListenHandler(void *pParam)
{
    USB_LISTEN_PIPE *pListenPipe = (USB_LISTEN_PIPE *)pParam;
    DWORD dwBufsize = pListenPipe->dwBytesToTransfer;
    PVOID buf;
    UINT64 qwTotalBytesTransferred = 0;
    TIME_TYPE streaming_time_start;

    buf = malloc(dwBufsize);
    if (!buf)
    {
        ERR("PipeListenHandler: Memory allocation failed\n");
        return;
    }
    if (!pListenPipe->fPrint)
        get_cur_time(&streaming_time_start);

    for (;;)
    {
        DWORD dwError;
        DWORD dwBytesTransferred;

        if (pListenPipe->fStreamMode)
        {
            dwError = WDU_StreamRead(pListenPipe->Handle, buf, dwBufsize,
                &dwBytesTransferred);
        }
        else
        {
            dwError = WDU_Transfer(pListenPipe->Handle,
                pListenPipe->pPipe->dwNumber, TRUE, pListenPipe->dwOptions, buf,
                dwBufsize, &dwBytesTransferred, NULL, TRANSFER_TIMEOUT);
        }
        qwTotalBytesTransferred += (UINT64)dwBytesTransferred;

        if (pListenPipe->fStopped)
            break;

        if (dwError)
        {
            pListenPipe->dwError = dwError;
            printf("Listen ended due to an error, press <Enter> to stop.\n");
            break;
        }

        if (pListenPipe->fPrint)
            DIAG_PrintHexBuffer(buf, dwBytesTransferred, TRUE);
    }
    if (!pListenPipe->fPrint)
        DIAG_PrintPerformance(qwTotalBytesTransferred, &streaming_time_start);

    free(buf);
}

/* Function: StartListeningToPipe()
     Starts listening to a USB device pipe
   Parameters:
     pListenPipe [in] pointer to the pipe to which to listen
   Return Value:
     None */
void StartListeningToPipe(USB_LISTEN_PIPE *pListenPipe)
{
    /* Start the running thread */
    pListenPipe->fStopped = FALSE;
    printf("Start listening to pipe\n");

    if (pListenPipe->fStreamMode)
    {
        pListenPipe->dwError = WDU_StreamStart(pListenPipe->Handle);
        if (pListenPipe->dwError)
        {
            ERR("StartListeningToPipe: WDU_StreamStart() failed. error 0x%lx "
                "(\"%s\")\n", pListenPipe->dwError,
                Stat2Str(pListenPipe->dwError));
            return;
        }
    }

    pListenPipe->dwError = ThreadStart(&pListenPipe->hThread, PipeListenHandler,
        (PVOID)pListenPipe);
}

/* Function: ListenToPipe()
     Listening to a USB device pipe
   Parameters:
     hDevice      [in] Handle to the USB device
     pPipe        [in] Pointer to the pipe for listening
     fStreamMode  [in] TRUE - Streaming mode
     dwBufferSize [in] Kernel buffer size on streaming mode
     fUserKeyWait [in] TRUE - Wait for user key before starting
   Return Value:
     None */
void ListenToPipe(HANDLE hDevice, WDU_PIPE_INFO *pPipe, BOOL fStreamMode,
    DWORD dwBufferSize, BOOL fUserKeyWait, BOOL fPrint)
{
    USB_LISTEN_PIPE listenPipe;

    BZERO(listenPipe);
    listenPipe.pPipe = pPipe;
    listenPipe.fStreamMode = fStreamMode;
    listenPipe.fPrint = fPrint;

    if (pPipe->type == PIPE_TYPE_ISOCHRONOUS)
    {
        listenPipe.dwBytesToTransfer = pPipe->dwMaximumPacketSize * 8;
        /* 8 minimum packets for high speed transfers */
        listenPipe.dwOptions |= USB_ISOCH_FULL_PACKETS_ONLY;
    }
    else
    {
        listenPipe.dwBytesToTransfer = pPipe->dwMaximumPacketSize * 36;
    }

    if (fStreamMode)
    {
        DWORD dwError;

        dwError = WDU_StreamOpen(hDevice, pPipe->dwNumber, dwBufferSize,
            listenPipe.dwBytesToTransfer, TRUE, listenPipe.dwOptions,
            TRANSFER_TIMEOUT, &listenPipe.Handle);
        if (dwError)
        {
            ERR("ListenToPipe: WDU_StreamOpen() failed. error 0x%lx (\"%s\")\n",
                dwError, Stat2Str(dwError));
            return;
        }
    }
    else
    {
        listenPipe.Handle = hDevice;
    }

    if (fUserKeyWait)
    {
        printf("Press <Enter> to start listening. While listening, press "
            "<Enter> to stop\n\n");
        getchar();
    }
    else
    {
        printf("Listening Started. While listening, press <Enter> to stop\n\n");
    }

    StartListeningToPipe(&listenPipe);
    if (listenPipe.dwError)
    {
        ERR("ListenToPipe: Error listening to pipe 0x%lx. error "
            "0x%lx (\"%s\")\n", pPipe->dwNumber, listenPipe.dwError,
            Stat2Str(listenPipe.dwError));
        return;
    }

    while (getchar() != 10); /* Waiting for <Enter> key */

    StopListeningToPipe(&listenPipe);
    if (listenPipe.dwError)
    {
        ERR("ListenToPipe: Transfer failed. error 0x%lx (\"%s\")\n",
            listenPipe.dwError, Stat2Str(listenPipe.dwError));
    }
}

/* Function: PrintPipe()
     Prints pipe information; (helper function)
   Parameters:
     pPipe [in] pointer to the pipe information to print
   Return Value:
     None */
static void PrintPipe(const WDU_PIPE_INFO *pPipe)
{
    printf("  pipe num. 0x%lx: packet size %ld, type %s, dir %s, "
        "interval %ld (ms)\n", pPipe->dwNumber, pPipe->dwMaximumPacketSize,
        pipeType2Str(pPipe->type), pPipe->direction == WDU_DIR_IN ? "In" :
        pPipe->direction == WDU_DIR_OUT ? "Out" : "In & Out",
        pPipe->dwInterval);
}

/* Function: PrintPipe0Info()
     Prints control pipe (pipe 0) information
   Parameters:
     pDevice [in] pointer to the USB device to which the control pipe belongs
   Return Value:
     None */
void PrintPipe0Info(WDU_DEVICE *pDevice)
{
    printf("Control pipe:\n");
    PrintPipe(&pDevice->Pipe0);
}

/* Function: PrintPipesInfo()
     Prints the pipes information for the specified alternate setting
   Parameters:
     pAltSet [in] pointer to the alternate setting information
   Return Value:
     None */
void PrintPipesInfo(WDU_ALTERNATE_SETTING *pAltSet)
{
    BYTE p;
    WDU_PIPE_INFO *pPipe;

    if (!pAltSet->Descriptor.bNumEndpoints)
    {
        printf("  no pipes are defined for this device other than the default "
            "pipe (number 0).\n");
        return;
    }

    printf("Alternate Setting: %d\n", pAltSet->Descriptor.bAlternateSetting);
    for (p = 0, pPipe = pAltSet->pPipes; p < pAltSet->Descriptor.bNumEndpoints;
        p++, pPipe++)
    PrintPipe(pPipe);
}

static void PrintDevicePipesInfo(WDU_DEVICE *pDevice)
{
    DWORD k;

    PrintPipe0Info(pDevice);
    /* Iterate over interfaces and print all pipes in their active alternate
     * settings */
    for (k = 0; k < pDevice->pActiveConfig->dwNumInterfaces; k++)
    {
        WDU_INTERFACE *pIfc = pDevice->pActiveInterface[k];

        if (!pIfc)
            break;

        printf("Interface %d\n",
            pIfc->pActiveAltSetting->Descriptor.bInterfaceNumber);
        PrintPipesInfo(pIfc->pActiveAltSetting);
    }
}

/* Function: PrintDevicePipesInfoByHandle()
     Prints the pipes information for all the active device pipes
   Parameters:
     hDevice [in] handle to the USB device
   Return Value:
     None */
void PrintDevicePipesInfoByHandle(HANDLE hDevice)
{
    WDU_DEVICE *pDevice;
    DWORD dwError;

    dwError = WDU_GetDeviceInfo(hDevice, &pDevice);
    if (dwError)
    {
        ERR("PrintDevicePipesInfoByHandle: WDU_GetDeviceInfo() failed. "
            "error 0x%lx (\"%s\")\n", dwError, Stat2Str(dwError));
        return;
    }

    PrintDevicePipesInfo(pDevice);
    WDU_PutDeviceInfo(pDevice);
}

/* Function: PrintEndpoints()
     Prints the endpoints (pipes) information for the specified alternate
     setting; (helper function for PrintDeviceConfigurations())
   Parameters:
     pAltSet [in] pointer to the alternate setting information
   Return Value:
     None */
static void PrintEndpoints(const WDU_ALTERNATE_SETTING *pAltSet)
{
    BYTE endp;
    WDU_ENDPOINT_DESCRIPTOR *pEndp;

    for (endp = 0; endp < pAltSet->Descriptor.bNumEndpoints; endp++)
    {
        pEndp = &pAltSet->pEndpointDescriptors[endp];
        printf("    end-point address: 0x%02x, attributes: 0x%x, max packet %d,"
            " Interval: %d\n", pEndp->bEndpointAddress, pEndp->bmAttributes,
            pEndp->wMaxPacketSize, pEndp->bInterval);
    }
}

/* Function: PrintDeviceConfigurations()
     Prints the device's configurations information
   Parameters:
     hDevice [in] handle to the USB device
   Return Value:
     None */
void PrintDeviceConfigurations(HANDLE hDevice)
{
    DWORD dwError;
    WDU_DEVICE *pDevice = NULL;
    DWORD ifc;
    UINT32 iConf;
    WDU_CONFIGURATION *pConf;
    WDU_INTERFACE *pInterface;
    WDU_ALTERNATE_SETTING *pAltSet;

    dwError = WDU_GetDeviceInfo(hDevice, &pDevice);
    if (dwError)
    {
        ERR("PrintDeviceConfigurations: WDU_GetDeviceInfo failed. error 0x%lx "
            "(\"%s\")\n", dwError, Stat2Str(dwError));
        goto Exit;
    }

    printf("This device has %d configurations:\n",
        pDevice->Descriptor.bNumConfigurations);
    for (iConf = 0; iConf < pDevice->Descriptor.bNumConfigurations; iConf++)
    {
        printf("  %d. Configuration value %d (has %ld interfaces)\n",
            iConf, pDevice->pConfigs[iConf].Descriptor.bConfigurationValue,
            pDevice->pConfigs[iConf].dwNumInterfaces);
    }
    iConf = 0;

    if (pDevice->Descriptor.bNumConfigurations > 1)
    {
        printf("Please enter the configuration index to display "
            "(dec - zero based): ");
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%d", &iConf);
        if (iConf >= pDevice->Descriptor.bNumConfigurations)
        {
            printf("ERROR: Invalid configuration index, valid values are "
                "0-%d\n", pDevice->Descriptor.bNumConfigurations);
            goto Exit;
        }
    }
    pConf = &pDevice->pConfigs[iConf];

    printf("The configuration indexed %d has %ld interface(s):\n", iConf,
        pConf->dwNumInterfaces);

    for (ifc = 0; ifc < pConf->dwNumInterfaces; ifc++)
    {
        DWORD alt;

        pInterface = &pConf->pInterfaces[ifc];
        printf("interface no. %d has %ld alternate settings:\n",
            pInterface->pAlternateSettings[0].Descriptor.bInterfaceNumber,
            pInterface->dwNumAltSettings);
        for (alt = 0; alt < pInterface->dwNumAltSettings; alt++)
        {
            pAltSet = &pInterface->pAlternateSettings[alt];

            printf("  alternate: %d, endpoints: %d, class: 0x%x, "
                "subclass: 0x%x, protocol: 0x%x\n",
                pAltSet->Descriptor.bAlternateSetting,
                pAltSet->Descriptor.bNumEndpoints,
                pAltSet->Descriptor.bInterfaceClass,
                pAltSet->Descriptor.bInterfaceSubClass,
                pAltSet->Descriptor.bInterfaceProtocol);

            PrintEndpoints(pAltSet);
        }
        printf("\n");
    }
    printf("\n");

Exit:
    if (pDevice)
        WDU_PutDeviceInfo(pDevice);
}



enum {
    MENU_RW_READ_PIPE = 1,
    MENU_RW_WRITE_PIPE,
    MENU_RW_LISTEN_PIPE,
    MENU_RW_LISTEN_PIPE_MEASURE,
    MENU_RW_STREAM_READ_SPEED,
    MENU_RW_STREAM_WRITE_SPEED,
    MENU_RW_CHANGE_MODE,
    MENU_RW_EXIT = 99,
};

/* Function: FindPipeInDevice()
     Finds a pipe in any of the device's active interfaces
   Parameters:
     pDevice      [in] pointer to the USB device
     dwPipeNumber [in] the pipe number to look for
   Return Value:
     a pointer to the requested pipe, or NULL if no matching pipe was found */
WDU_PIPE_INFO *FindPipeInDevice(WDU_DEVICE *pDevice, DWORD dwPipeNumber)
{
    DWORD i;

    if (dwPipeNumber == 0)
        return &pDevice->Pipe0;

    for (i = 0; i < pDevice->pActiveConfig->dwNumInterfaces; i++)
    {
        DWORD j;
        WDU_INTERFACE *pIfc = pDevice->pActiveInterface[i];
        WDU_ALTERNATE_SETTING *pAltSet;

        if (!pIfc)
            break;

        pAltSet = pIfc->pActiveAltSetting;

        for (j = 0; j < pAltSet->Descriptor.bNumEndpoints; j++)
        {
            if (pAltSet->pPipes[j].dwNumber == dwPipeNumber)
                return &pAltSet->pPipes[j];
        }
    }

    return NULL;
}

/* Function: ReadWritePipesMenu()
     Displays a menu to read/write from/to the device's pipes
   Parameters:
     hDevice [in] handle to the USB device
   Return Value:
     None */
void ReadWritePipesMenu(HANDLE hDevice)
{
    DWORD dwError;
    WDU_DEVICE *pDevice = NULL;
    WDU_PIPE_INFO *pPipe;
    BYTE SetupPacket[8];
    DWORD cmd, dwPipeNum, dwSize, dwBytesTransferred;
    VOID *pBuffer = NULL;
    WDU_STREAM_HANDLE stream;
    BOOL fStreamMode = TRUE;
    DWORD dwBufferSize = 0x20000;
    TIME_TYPE streaming_time_start;
#if defined(WIN32)
    BOOL fStreamAble = TRUE;
#else
    BOOL fStreamAble = FALSE;
#endif

    dwError = WDU_GetDeviceInfo(hDevice, &pDevice);
    if (dwError)
    {
        ERR("ReadWritePipesMenu: WDU_GetDeviceInfo() failed. error 0x%lx "
            "(\"%s\")\n", dwError, Stat2Str(dwError));
        return;
    }

    PrintDevicePipesInfo(pDevice);

    if (!fStreamAble)
        fStreamMode = FALSE;

    do {
        printf("\n");
        printf("Read/Write from/to device's pipes using %s\n", fStreamMode ?
            "Streaming Data Transfers" : "Single Blocking Transfers");
        printf("---------------------\n");
        printf("%d.  Read from pipe\n", MENU_RW_READ_PIPE);
        printf("%d.  Write to pipe\n", MENU_RW_WRITE_PIPE);
        printf("%d.  Listen to pipe (continuous read)\n", MENU_RW_LISTEN_PIPE);
        printf("%d.  Measure pipe speed (continuous read)\n",
            MENU_RW_LISTEN_PIPE_MEASURE);

        if (fStreamMode)
        {
            printf("%d.  Check streaming READ speed\n",
                MENU_RW_STREAM_READ_SPEED);
            printf("%d.  Check streaming WRITE speed\n",
                MENU_RW_STREAM_WRITE_SPEED);
        }

        /* Toggle menu from regular to streaming */
        if (fStreamAble)
        {
            printf("%d.  Switch to %s mode\n", MENU_RW_CHANGE_MODE,
                !fStreamMode ? "Streaming Data Transfers" : "Single Blocking "
                "Transfers");
        }

        printf("%d. Main menu\n", MENU_RW_EXIT);
        printf("Enter option: ");
        cmd = 0;
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%ld", &cmd);

        if (cmd == MENU_RW_EXIT)
            break;

        if (cmd == MENU_RW_CHANGE_MODE)
        {
            if (fStreamAble)
                fStreamMode = !fStreamMode;
            continue;
        }

        /* Make sure the option entered is available for the current mode */
        if (cmd < MENU_RW_READ_PIPE ||
            (!fStreamMode && cmd > MENU_RW_LISTEN_PIPE_MEASURE) ||
            cmd > MENU_RW_STREAM_WRITE_SPEED)
        {
            continue;
        }

        printf("Please enter the pipe number (hex): 0x");
        fgets(line, sizeof(line), stdin);
        dwPipeNum = 0;
        sscanf(line, "%lx", &dwPipeNum);

        /* Search for the pipe */
        pPipe = FindPipeInDevice(pDevice, dwPipeNum);
        if (!pPipe)
        {
            printf("The pipe number 0x%lx does not exist. Please try again.\n",
                dwPipeNum);
            continue;
        }

        switch (cmd)
        {
        case MENU_RW_READ_PIPE:
        case MENU_RW_WRITE_PIPE:
            if (!dwPipeNum || pPipe->type == PIPE_TYPE_CONTROL)
            {
                if (fStreamMode)
                {
                    ERR("Cannot perform stream transfer using control pipe.\n"
                        "please switch to Single Blocking Transfer mode "
                        "(option 6) or change the pipe number\n");
                    break;
                }
                printf("Please enter setup packet (hex - 8 bytes): ");
                DIAG_GetHexBuffer((PVOID)SetupPacket, 8);
            }

            printf("Please enter the size of the buffer (dec):  ");
            fgets(line, sizeof(line), stdin);
            sscanf(line, "%ld", &dwSize);
            if (dwSize)
            {
                pBuffer = malloc(dwSize);
                if (!pBuffer)
                {
                    ERR("Cannot alloc memory\n");
                    break;
                }

                if (cmd == MENU_RW_WRITE_PIPE)
                {
                    printf("Please enter the input buffer (hex): ");
                    DIAG_GetHexBuffer(pBuffer, dwSize);
                }
            }

            if (fStreamMode)
            {
                dwError = WDU_StreamOpen(hDevice, dwPipeNum, dwBufferSize,
                    dwSize, TRUE, 0, TRANSFER_TIMEOUT, &stream);
                if (dwError)
                {
                    ERR("ReadWritePipesMenu: WDU_StreamOpen() failed. "
                        "error 0x%lx (\"%s\")\n", dwError, Stat2Str(dwError));
                    goto End_transfer;
                }

                dwError = WDU_StreamStart(stream);
                if (dwError)
                {
                    ERR("ReadWritePipesMenu: WDU_StreamStart() failed. error "
                        "0x%lx (\"%s\")\n", dwError, Stat2Str(dwError));
                    goto End_transfer;
                }

                if (cmd == MENU_RW_READ_PIPE)
                {
                    dwError = WDU_StreamRead(stream, pBuffer, dwSize,
                        &dwBytesTransferred);
                }
                else
                {
                    dwError = WDU_StreamWrite(stream, pBuffer, dwSize,
                        &dwBytesTransferred);
                }
                if (dwError)
                {
                    BOOL fIsRunning;
                    DWORD dwLastError;
                    DWORD dwBytesInBuffer;

                    dwError = WDU_StreamGetStatus(stream, &fIsRunning,
                        &dwLastError, &dwBytesInBuffer);
                    if (!dwError)
                        dwError = dwLastError;
                }
            }
            else
            {
                dwError = WDU_Transfer(hDevice, dwPipeNum ? pPipe->dwNumber : 0,
                    cmd == MENU_RW_READ_PIPE, 0, pBuffer, dwSize,
                    &dwBytesTransferred, SetupPacket, TRANSFER_TIMEOUT);
            }

            if (dwError)
            {
                ERR("ReadWritePipesMenu: Transfer failed. error 0x%lx "
                    "(\"%s\")\n", dwError, Stat2Str(dwError));
            }
            else
            {
                printf("Transferred %ld bytes\n", dwBytesTransferred);
                if (cmd == MENU_RW_READ_PIPE && pBuffer)
                    DIAG_PrintHexBuffer(pBuffer, dwBytesTransferred, TRUE);
            }

End_transfer:
            if (pBuffer)
            {
                free(pBuffer);
                pBuffer = NULL;
            }

            if (fStreamMode && stream != NULL)
            {
                dwError = WDU_StreamClose(stream);
                if (dwError)
                {
                    ERR("ReadWritePipesMenu: WDU_StreamClose() failed. error "
                        "0x%lx (\"%s\")\n", dwError, Stat2Str(dwError));
                    break;
                }
            }
            break;

        case MENU_RW_LISTEN_PIPE:
        case MENU_RW_LISTEN_PIPE_MEASURE:
            if (!dwPipeNum || pPipe->type == PIPE_TYPE_CONTROL)
            {
                printf("Cannot listen to control pipes.\n");
                break;
            }

            ListenToPipe(hDevice, pPipe, fStreamMode, dwBufferSize, TRUE,
                cmd == MENU_RW_LISTEN_PIPE ? TRUE : FALSE);
            break;

       case MENU_RW_STREAM_READ_SPEED:
       case MENU_RW_STREAM_WRITE_SPEED:
            if (!fStreamMode)
                break; /* We shouldn't reach this line */

            if (!dwPipeNum || pPipe->type == PIPE_TYPE_CONTROL)
            {
                printf("Cannot perform stream transfer with control pipe\n");
                break;
            }

            printf("The size of the buffer to transfer(dec): %d\n",
                PERF_DEVICE_TRANSFER_SIZE);
            printf("The size of the internal Rx/Tx stream buffer (dec): %d\n",
                PERF_STREAM_BUFFER_SIZE);
            printf("Making the transfer of %d times the buffer size, please "
                "wait ...\n", PERF_TRANSFER_ITERATIONS);

            if (cmd == MENU_RW_STREAM_WRITE_SPEED)
            {
                /* Here you can fill pBuffer with the right data for the
                 * board */
            }

            dwError = WDU_StreamOpen(hDevice, dwPipeNum,
                PERF_STREAM_BUFFER_SIZE, PERF_DEVICE_TRANSFER_SIZE, TRUE, 0,
                TRANSFER_TIMEOUT, &stream);
            if (dwError)
            {
                ERR("ReadWritePipesMenu: WDU_StreamOpen() failed. error 0x%lx "
                    "(\"%s\")\n", dwError, Stat2Str(dwError));
                break;
            }

            pBuffer = malloc(PERF_DEVICE_TRANSFER_SIZE);
            if (!pBuffer)
            {
                ERR("Failed allocating memory\n");
                break;
            }

            dwError = WDU_StreamStart(stream);
            if (dwError)
            {
                ERR("ReadWritePipesMenu: WDU_StreamStart() failed. error "
                    "0x%lx (\"%s\")\n", dwError, Stat2Str(dwError));
                goto End_perf_test;
            }

            dwBytesTransferred = 0;
            get_cur_time(&streaming_time_start);
            do {
                DWORD dwBytesTransferredSingle;

                if (cmd == MENU_RW_STREAM_READ_SPEED)
                {
                    dwError = WDU_StreamRead(stream, pBuffer,
                        PERF_DEVICE_TRANSFER_SIZE, &dwBytesTransferredSingle);
                }
                else
                {
                    dwError = WDU_StreamWrite(stream, pBuffer,
                        PERF_DEVICE_TRANSFER_SIZE, &dwBytesTransferredSingle);
                }

                if (dwError)
                {
                    ERR("ReadWritePipesMenu: Transfer failed. error 0x%lx "
                        "(\"%s\")\n", dwError, Stat2Str(dwError));
                    goto End_perf_test;
                }

                dwBytesTransferred += dwBytesTransferredSingle;

            } while (dwBytesTransferred < PERF_DEVICE_TRANSFER_SIZE *
                PERF_TRANSFER_ITERATIONS);

            /* If write command, wait for all the data to be written */
            if (cmd == MENU_RW_STREAM_WRITE_SPEED)
            {
                dwError = WDU_StreamFlush(stream);
                if (dwError)
                {
                    ERR("ReadWritePipesMenu: Transfer failed. error 0x%lx "
                        "(\"%s\")\n", dwError, Stat2Str(dwError));
                    goto End_perf_test;
                }
            }

            DIAG_PrintPerformance(dwBytesTransferred, &streaming_time_start);

End_perf_test:
            if (pBuffer)
            {
                free(pBuffer);
                pBuffer = NULL;
            }

            dwError = WDU_StreamClose(stream);
            if (dwError)
            {
                ERR("ReadWritePipesMenu: WDU_StreamClose() failed. error "
                    "0x%lx (\"%s\")\n", dwError, Stat2Str(dwError));
            }

            break;
        }
    } while (1);

    if (pDevice)
        WDU_PutDeviceInfo(pDevice);
}

enum
{
    MENU_SUSPEND_ENABLE_DISABLE = 1,
    MENU_SUSPEND_EXIT = 99
};

/* Function: SelectiveSuspendMenu()
     Displays a menu to enter/exit Selective Suspend mode
   Parameters:
     hDevice [in] handle to the USB device
   Return Value:
     None */
void SelectiveSuspendMenu(WDU_DEVICE_HANDLE hDevice)
{
    DWORD dwError = 0, cmd;
    BOOL fSuspended = FALSE;

    do {
        printf("\n");
        printf("Toggle suspend mode\n");
        printf("-----------------------------\n");
        printf("%d. %s suspend mode\n", MENU_SUSPEND_ENABLE_DISABLE,
            fSuspended ? "Leave" : "Enter");
        printf("%d. Main menu\n", MENU_SUSPEND_EXIT);
        printf("Enter option: ");

        fgets(line, sizeof(line), stdin);
        sscanf(line, "%ld", &cmd);

        switch (cmd)
        {
        case MENU_SUSPEND_ENABLE_DISABLE:
            if (!fSuspended)
            {
                dwError = WDU_SelectiveSuspend(hDevice,
                    WDU_SELECTIVE_SUSPEND_SUBMIT);
            }

            /* Fall through */
        case MENU_SUSPEND_EXIT:
            if (fSuspended)
            {
                dwError = WDU_SelectiveSuspend(hDevice,
                    WDU_SELECTIVE_SUSPEND_CANCEL);
            }
        break;

        default:
            continue;
        }

        if (dwError)
        {
            ERR("DeviceDiagMenu: WDU_SelectiveSuspend() failed. error 0x%lx "
                "(\"%s\")\n", dwError, Stat2Str(dwError));
        }
        else
        {
            fSuspended = !fSuspended;
        }
    } while (cmd != MENU_SUSPEND_EXIT);
}

/* Function: PrintDeviceSerialNumberByHandle()
     Prints the device serial number if available
   Parameters:
     hDevice [in] handle to the USB device
   Return Value:
     None */
void PrintDeviceSerialNumberByHandle(HANDLE hDevice)
{
    WDU_DEVICE *pDevice;
    DWORD dwError;
    BYTE bSerialNum[0x100];
    DWORD dwSerialDescSize = 0;
    DWORD i;

    dwError = WDU_GetDeviceInfo(hDevice, &pDevice);
    if (dwError)
    {
        ERR("PrintDeviceSerialNumberByHandle: WDU_GetDeviceInfo() failed. "
            "error 0x%lx (\"%s\")\n", dwError, Stat2Str(dwError));
        return;
    }

    if (!pDevice->Descriptor.iSerialNumber)
    {
        printf("Serial number is not available\n");
        goto Exit;
    }

    printf("Serial number string descriptor index: [%d]\n",
        pDevice->Descriptor.iSerialNumber);

    dwError = WDU_GetStringDesc(hDevice, pDevice->Descriptor.iSerialNumber,
        bSerialNum, sizeof(bSerialNum), 0, &dwSerialDescSize);
    if (dwError)
    {
        ERR("PrintDeviceSerialNumberByHandle: WDU_GetStringDesc() failed. "
            "error 0x%lx (\"%s\")\n", dwError, Stat2Str(dwError));
        goto Exit;
    }

    printf("Serial number: length [%ld], [", dwSerialDescSize);
    for (i = 0; i < dwSerialDescSize; i++)
        printf("%c", bSerialNum[i]);
    printf("]\n");

Exit:
    WDU_PutDeviceInfo(pDevice);
}

/* Function: PrintDeviceProperties()
     Prints the device properties if available,
     depneding on OS
   Parameters:
     hDevice [in] handle to the USB device
   Return Value:
     None */
void PrintDeviceProperties(HANDLE hDevice)
{
    DWORD dwStatus = 0;
    DWORD dwSize, i;
#ifdef WIN32
    WCHAR cProperty[256];
#else
    CHAR cProperty[256];
#endif
    CHAR propertyNames[20][46] =
    { "WdDevicePropertyDeviceDescription", "WdDevicePropertyHardwareID",
    "WdDevicePropertyCompatibleIDs", "WdDevicePropertyBootConfiguration",
    "WdDevicePropertyBootConfigurationTranslated", "WdDevicePropertyClassName",
    "WdDevicePropertyClassGuid", "WdDevicePropertyDriverKeyName",
    "WdDevicePropertyManufacturer", "WdDevicePropertyFriendlyName",
    "WdDevicePropertyLocationInformation", "WdDevicePropertyPhysicalDeviceObjectName",
    "WdDevicePropertyBusTypeGuid", "WdDevicePropertyLegacyBusType",
    "WdDevicePropertyBusNumber", "WdDevicePropertyEnumeratorName",
    "WdDevicePropertyAddress","WdDevicePropertyUINumber",
    "WdDevicePropertyInstallState", "WdDevicePropertyRemovalPolicy" };

    for (i = 0; i < 20; i++)
    {
        dwSize = 256;
        BZERO(cProperty);
        dwStatus = WDU_GetDeviceRegistryProperty(hDevice, cProperty, &dwSize,
            (WD_DEVICE_REGISTRY_PROPERTY)i);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%-46s: ", propertyNames[i]);
            if (i > WdDevicePropertyPhysicalDeviceObjectName &&
                i != WdDevicePropertyEnumeratorName)
            {
                printf("%d\n", *cProperty);
            }
            else
            {
#ifdef WIN32
                printf("%ws\n", cProperty);
#else
                printf("%s\n", cProperty);
#endif
            }
        }
    }
}

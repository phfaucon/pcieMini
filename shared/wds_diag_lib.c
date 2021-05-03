/* Jungo Connectivity Confidential. Copyright (c) 2020 Jungo Connectivity Ltd.  https://www.jungo.com */

/******************************************************************************
*  File: wds_diag_lib.c - Implementation of shared WDS all devices' user-mode *
*        diagnostics API.                                                     *
*                                                                             *
*  Note: This code sample is provided AS-IS and as a guiding sample only.     *
*******************************************************************************/

#if !defined(__KERNEL__)

#include <stdio.h>
#include "wds_diag_lib.h"
#include "status_strings.h"
#include "diag_lib.h"
#include "wdc_defs.h"
#include "wds_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define WDS_DIAG_ERR printf

#define DEFAULT_PROCESS_NAME "Diagnostic program"

/* Unique identifier of the processes group to avoid getting messages from
 * processes made under WinDriver by other developers that use the same driver
 * name.
 * WinDriver developers are encouraged to change their driver name before
 * distribution to avoid this issue entirely. */
#define DEFAULT_PROCESS_GROUP_ID 0x12345678

/* Identifiers for shared interrupt IPC process */
#define DEFAULT_SHARED_INT_NAME "WinDriver IPC Shared Interrupt"

#define SAMPLE_BUFFER_DATA "This is a sample buffer data"

/*************************************************************
  Global variables
 *************************************************************/

/* User's input command */
static CHAR gsInput[256];

/* User's shared kernel buffer handle */
static WD_KERNEL_BUFFER *pSharedKerBuf = NULL; /* Static global pointer is used
                                                * only for sample simplicity */

/* -----------------------------------------------
    Shared Buffer
   ----------------------------------------------- */

/* Shared Buffer menu options */
enum {
    MENU_SB_ALLOC_CONTIG = 1,
    MENU_SB_ALLOC_NON_CONTIG,
    MENU_SB_FREE,
    MENU_SB_EXIT = DIAG_EXIT_MENU,
};

static void WDS_DIAG_SharedBufferFree(WD_KERNEL_BUFFER **ppKerBuf)
{
    DWORD dwStatus;

    if (!(*ppKerBuf))
        return;

    dwStatus = WDS_SharedBufferFree(*ppKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_SharedBufferFree: Failed freeing shared buffer "
            "memory. Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    *ppKerBuf = NULL;

    printf("Shared buffer memory freed\n");
}

void MenuSharedBuffer(void)
{
    DWORD option, size, dwStatus;
    WD_KERNEL_BUFFER *pKerBuf = NULL;

    do
    {
        DWORD dwOptions = 0;

        printf("\n");
        printf("Shared Buffer Operations\n");
        printf("    E.g. for communicating with Kernel-Plugin\n");
        printf("-----------\n");

        printf("%d. Allocate contiguous shared buffer\n", MENU_SB_ALLOC_CONTIG);
        printf("%d. Allocate non-contiguous shared buffer\n",
            MENU_SB_ALLOC_NON_CONTIG);
        printf("%d. Free shared buffer\n", MENU_SB_FREE);
        printf("%d. Exit menu\n", MENU_SB_EXIT);
        printf("\n");

        if (DIAG_INPUT_FAIL == DIAG_GetMenuOption(&option, MENU_SB_FREE))
        {
            continue;
        }

        switch (option)
        {
        case MENU_SB_ALLOC_CONTIG: /* Allocate contig shared buffer */
        case MENU_SB_ALLOC_NON_CONTIG: /* Allocate non-contig shared buffer */
            if (option == MENU_SB_ALLOC_NON_CONTIG)
                dwOptions |= KER_BUF_ALLOC_NON_CONTIG;
            else
                dwOptions |= KER_BUF_ALLOC_CONTIG;

            sprintf(gsInput, "Enter memory allocation size in bytes "
                "(32 bit uint)");
            size = 0;
            if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&size, gsInput,
                FALSE, 1, 0xFFFFFFFF))
            {
                continue;
            }

            /* Free shared buffer memory before trying the new allocation */
            WDS_DIAG_SharedBufferFree(&pKerBuf);

            dwStatus = WDS_SharedBufferAlloc(size, dwOptions, &pKerBuf);
            if (WD_STATUS_SUCCESS == dwStatus)
            {
                printf("Shared buffer allocated. User addr [0x%"UPRI"x], "
                    "kernel addr [0x%"KPRI"x], size [%lu(0x%lx)]\n",
                    pKerBuf->pUserAddr, pKerBuf->pKernelAddr, size, size);
            }
            else
            {
                WDS_DIAG_ERR("MenuSharedBuffer: Failed allocating shared "
                    "buffer memory. size [%ld], Error [0x%lx - %s]\n", size,
                    dwStatus, Stat2Str(dwStatus));
            }
            break;

        case MENU_SB_FREE: /* Free shared buffer memory */
            WDS_DIAG_SharedBufferFree(&pKerBuf);
            break;
        }
    } while (MENU_SB_EXIT != option);

    /* Free shared buffer before exiting */
    WDS_DIAG_SharedBufferFree(&pKerBuf);
}

/* -----------------------------------------------
    IPC - Inter process Communication
   ----------------------------------------------- */

static void ipc_msg_event_cb(WDS_IPC_MSG_RX *pIpcRxMsg, void *pData)
{
    printf("\n\nReceived an IPC message:\n"
        "msgID [0x%lx], msgData [0x%llx] from process [0x%lx]\n",
        pIpcRxMsg->dwMsgID, pIpcRxMsg->qwMsgData, pIpcRxMsg->dwSenderUID);

    /* Important: Acquiring and using any resource (E.g. kernel/DMA buffer)
     * should be done from a deferred procedure to avoid jamming the IPC
     * incoming messages.
     * Notice you can pass private context at WDS_IpcRegister() and use it here
     * (pData) for signalling local thread for example.
     *
     * The following implementation is for sample purposes only! */

    switch (pIpcRxMsg->dwMsgID)
    {
    case IPC_MSG_KERNEL_BUFFER_READY:
        {
            DWORD dwStatus;
            WD_KERNEL_BUFFER *pKerBuf = NULL;
            DWORD sample_buffer_len;

            printf("\nThis is a shared kernel buffer, getting it...\n");

            dwStatus = WDS_SharedBufferGet((DWORD)pIpcRxMsg->qwMsgData,
                &pKerBuf);
            if (WD_STATUS_SUCCESS != dwStatus)
            {
                WDS_DIAG_ERR("ipc_msg_event_cb: Failed getting shared kernel "
                    "buffer. Error [0x%lx - %s]\n", dwStatus,
                    Stat2Str(dwStatus));
                return;
            }

            printf("Got a shared kernel buffer. UserAddr [0x%"UPRI"x], "
                "KernelAddr [0x%"KPRI"x], size [%llu]\n", pKerBuf->pUserAddr,
                pKerBuf->pKernelAddr, pKerBuf->qwBytes);

            /* Here we read SAMPLE_BUFFER_DATA from the received buffer */
            sample_buffer_len = (DWORD)strlen(SAMPLE_BUFFER_DATA);
            if (pKerBuf->qwBytes > sample_buffer_len + 1)
            {
                printf("Sample data from kernel buffer [%s]\n",
                    (char *)pKerBuf->pUserAddr);
            }
            else
            {
                printf("Kernel buffer was too short for sample data\n");
            }

            /* For sample purpose we immediately release the buffer */
            WDS_SharedBufferFree(pKerBuf);
        }
        break;

    case IPC_MSG_CONTIG_DMA_BUFFER_READY:
        {
            DWORD dwStatus;
            WD_DMA *pDma = NULL;

            printf("\nThis is a DMA buffer, getting it...\n");

            dwStatus = WDC_DMABufGet((DWORD)pIpcRxMsg->qwMsgData, &pDma);
            if (WD_STATUS_SUCCESS != dwStatus)
            {
                WDS_DIAG_ERR("ipc_msg_event_cb: Failed getting DMA buffer. "
                    "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
                return;
            }

            printf("Got a DMA buffer. UserAddr [%p], "
                "pPhysicalAddr [0x%"PRI64"x], size [%ld(0x%lx)]\n",
                pDma->pUserAddr, pDma->Page[0].pPhysicalAddr,
                pDma->Page[0].dwBytes, pDma->Page[0].dwBytes);

            /* For sample purpose we immediately release the buffer */
            WDC_DMABufUnlock(pDma);
        }
        break;

    default:
        printf("Unknown IPC type. msgID [0x%lx], msgData [0x%llx] from "
            "process [0x%lx]\n\n", pIpcRxMsg->dwMsgID, pIpcRxMsg->qwMsgData,
            pIpcRxMsg->dwSenderUID);
    }
}

static void ipc_shared_int_msg_event_cb(WDS_IPC_MSG_RX *pIpcRxMsg, void *pData)
{
    printf("Shared Interrupt via IPC arrived:\nmsgID [0x%lx], msgData [0x%llx]"
        " from process [0x%lx]\n\n", pIpcRxMsg->dwMsgID, pIpcRxMsg->qwMsgData,
        pIpcRxMsg->dwSenderUID);
}

/* Register process to IPC service */
static DWORD WDS_DIAG_IpcRegister(void)
{
    DWORD dwSubGroupID = 0;
    DWORD dwStatus;

    sprintf(gsInput, "Enter process SubGroup ID (hex)");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwSubGroupID, gsInput,
        TRUE, 0, 0xFFFFFFFF))
    {
        return 0;
    }

    dwStatus = WDS_IpcRegister(DEFAULT_PROCESS_NAME, DEFAULT_PROCESS_GROUP_ID,
        dwSubGroupID, WD_IPC_ALL_MSG, ipc_msg_event_cb, NULL /* Your cb ctx */);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcRegister: Failed registering process to IPC. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return 0;
    }

    printf("Registration completed successfully\n");
    return dwSubGroupID;
}

/* Enable Shared Interrupts via IPC */
static DWORD WDS_DIAG_IpcSharedIntsEnable(void)
{
    DWORD dwSubGroupID = 0;
    DWORD dwStatus;

    if (WDS_IsSharedIntsEnabledLocally())
    {
        WDS_DIAG_ERR("%s: Shared interrupts already enabled locally.\n",
            __FUNCTION__);
        return WD_OPERATION_ALREADY_DONE;
    }

    sprintf(gsInput, "Enter shared interrupt's SubGroup ID (hex)");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwSubGroupID, gsInput,
        TRUE, 0, 0xFFFFFFFF))
    {
        return 0;
    }

    /* WDS_SharedIntEnable() is called in this sample with pFunc=
    ipc_shared_int_msg_event_cb. This will cause a shared interrupt to invoke
    both this callback and the callback passed to WDS_IpcRegister() in
    WDS_DIAG_IpcRegister(). To disable the "general" IPC callback, pass
    pFunc=NULL in the above mentioned call.
    Note you can replace pFunc here with your own callback especially designed
    to handle interrupts */
    dwStatus = WDS_SharedIntEnable(DEFAULT_SHARED_INT_NAME,
        DEFAULT_PROCESS_GROUP_ID, dwSubGroupID, WD_IPC_ALL_MSG,
        ipc_shared_int_msg_event_cb, NULL /* Your cb ctx */);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("%s: Failed enabling shared interrupts via IPC. "
            "Error [0x%lx - %s]\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        return 0;
    }

    printf("Shared interrupts via IPC enabled successfully\n");
    return dwSubGroupID;
}

static void WDS_DIAG_IpcScanProcs(void)
{
    DWORD dwStatus;
    WDS_IPC_SCAN_RESULT ipcScanResult;
    DWORD i;

    dwStatus = WDS_IpcScanProcs(&ipcScanResult);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcScanProcs: Failed scanning registered "
            "processes. Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return;
    }

    if (ipcScanResult.dwNumProcs)
    {
        printf("Found %ld processes in current group\n",
            ipcScanResult.dwNumProcs);
        for (i = 0; i < ipcScanResult.dwNumProcs; i++)
        {
            printf("  %lu) Name: %s, SubGroup ID: 0x%lx, UID: 0x%lx\n", i + 1,
                ipcScanResult.procInfo[i].cProcessName,
                ipcScanResult.procInfo[i].dwSubGroupID,
                ipcScanResult.procInfo[i].hIpc);
        }
    }
    else
    {
        printf("No processes found in current group\n");
    }
}

static void WDS_DIAG_IpcKerBufRelease(void)
{
    DWORD dwStatus;

    if (!pSharedKerBuf)
        return;

    /* Notice that once a buffer that was acquired by a different process is
     * freed, its kernel resources are kept as long as the other processes did
     * not release the buffer. */
    dwStatus = WDS_SharedBufferFree(pSharedKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcKerBufRelease: Failed freeing shared "
            "buffer. Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    pSharedKerBuf = NULL;

    printf("Kernel buffer freed\n");
}

static void WDS_DIAG_IpcKerBufAllocAndShare(void)
{
    DWORD size = 0;
    DWORD dwStatus;
    DWORD dwOptions = KER_BUF_ALLOC_CONTIG;
    DWORD sample_buffer_len;

    /* If kernel buffer was allocated in the past, release it */
    WDS_DIAG_IpcKerBufRelease();

    sprintf(gsInput, "Enter new kernel buffer size to allocate and share with "
        "current group");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&size, gsInput, TRUE, 1,
        0xFFFFFFFF))
    {
        return;
    }

    dwStatus = WDS_SharedBufferAlloc(size, dwOptions, &pSharedKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcKerBufAllocAndShare: Failed allocating "
            "shared kernel buffer. size [%lu], Error [0x%lx - %s]\n", size,
            dwStatus, Stat2Str(dwStatus));
        return;
    }

    printf("Successful kernel buffer allocation. UserAddr [0x%"UPRI"x], "
        "KernelAddr [0x%"KPRI"x], size [%lu]\n", pSharedKerBuf->pUserAddr,
        pSharedKerBuf->pKernelAddr, size);

    /* Here we write SAMPLE_BUFFER_DATA to the new allocated buffer */
    sample_buffer_len = (DWORD)strlen(SAMPLE_BUFFER_DATA);
    if (size > sample_buffer_len + 1)
    {
        memcpy((PVOID)pSharedKerBuf->pUserAddr, SAMPLE_BUFFER_DATA,
            sample_buffer_len);
        ((char *)pSharedKerBuf->pUserAddr)[sample_buffer_len] = '\0';
        printf("Sample data written to kernel buffer\n");
    }
    else
    {
        printf("Kernel buffer is too short for sample data\n");
    }

    dwStatus = WDS_IpcMulticast(IPC_MSG_KERNEL_BUFFER_READY,
        WDS_SharedBufferGetGlobalHandle(pSharedKerBuf));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcAllocAndShareBuf: Failed sending message. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return;
    }

    printf("Kernel buffer shared successfully\n");
}

/* IPC menu options */
enum {
    MENU_IPC_REGISTER = 1,
    MENU_IPC_UNREGISTER,
    MENU_IPC_GET_GROUP_IDS,
    MENU_IPC_SEND_UID_UNICAST,
    MENU_IPC_SEND_SUBGROUP_MULTICAST,
    MENU_IPC_SEND_MULTICAST,
    MENU_IPC_ENABLE_SHARED_INTS,
    MENU_IPC_LOCAL_DISABLE_SHARED_INTS,
    MENU_IPC_GLOBAL_DISABLE_SHARED_INTS,
    MENU_IPC_KER_BUF_ALLOC_AND_SHARE,
    MENU_IPC_KER_BUF_RELEASE,
    MENU_IPC_EXIT = DIAG_EXIT_MENU,
};

static void WDS_DIAG_IpcSend(DWORD ipc_menu_option)
{
    DWORD recipientID;
    DWORD messageID;
    UINT64 messageData;
    DWORD dwStatus;

    if (ipc_menu_option == MENU_IPC_SEND_UID_UNICAST ||
        ipc_menu_option == MENU_IPC_SEND_SUBGROUP_MULTICAST)
    {
        sprintf(gsInput, "Enter recipient%sID (hex)",
            ipc_menu_option == MENU_IPC_SEND_SUBGROUP_MULTICAST ?
            "(s) SubGroup " : " U");
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&recipientID,
            gsInput, TRUE, 0, 0xFFFFFFFF))
        {
            return;
        }
    }

    sprintf(gsInput, "Enter your message ID (32Bit hex)");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&messageID, gsInput, TRUE,
        0, 0xFFFFFFFF))
    {
        return;
    }

    sprintf(gsInput, "Enter your message (64Bit hex)");
    if (DIAG_INPUT_SUCCESS != DIAG_InputUINT64((PVOID)&messageData, gsInput,
        TRUE, 0, 0xFFFFFFFF))
    {
        return;
    }

    switch (ipc_menu_option)
    {
    case MENU_IPC_SEND_UID_UNICAST:
        dwStatus = WDS_IpcUidUnicast(recipientID, messageID, messageData);
        break;

    case MENU_IPC_SEND_SUBGROUP_MULTICAST:
        dwStatus = WDS_IpcSubGroupMulticast(recipientID, messageID,
            messageData);
        break;

    case MENU_IPC_SEND_MULTICAST:
        dwStatus = WDS_IpcMulticast(messageID, messageData);
        break;
    }

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcSend: Failed sending message. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return;
    }

    printf("Message sent successfully\n");
}

void MenuIpc(void)
{
    DWORD option;
    DWORD dwSubGroupID = 0;

    do
    {
        BOOL isIpcRegistered = WDS_IsIpcRegistered();

        printf("\n");
        printf("IPC management menu - ");
        if (isIpcRegistered)
            printf("Registered with SubGroup ID 0x%lx\n", dwSubGroupID);
        else
            printf("Unregistered\n");
        printf("--------------\n");

        if (!isIpcRegistered)
        {
            printf("%d. Register processes\n", MENU_IPC_REGISTER);
        }
        else
        {
            printf("%d. Un-Register process\n", MENU_IPC_UNREGISTER);
            printf("%d. Find current registered group processes\n",
                MENU_IPC_GET_GROUP_IDS);
            printf("%d. Unicast- Send message to a single process by "
                "unique ID\n", MENU_IPC_SEND_UID_UNICAST);
            printf("%d. Multicast - Send message to a subGroup\n",
                MENU_IPC_SEND_SUBGROUP_MULTICAST);
            printf("%d. Multicast- Send message to all processes in current "
                "group\n", MENU_IPC_SEND_MULTICAST);
            printf("%d. Enable Shared Interrupts via IPC \n",
                MENU_IPC_ENABLE_SHARED_INTS);
            printf("%d. Locally Disable Shared Interrupts via IPC \n",
                MENU_IPC_LOCAL_DISABLE_SHARED_INTS);
            printf("%d. Globally Disable Shared Interrupts via IPC \n",
                MENU_IPC_GLOBAL_DISABLE_SHARED_INTS);
            printf("%d. Allocate and share a kernel buffer with all processes "
                "in current group\n", MENU_IPC_KER_BUF_ALLOC_AND_SHARE);
            printf("%d. Free shared kernel buffer\n", MENU_IPC_KER_BUF_RELEASE);
        }
        printf("%d. Exit\n", MENU_IPC_EXIT);

        if (DIAG_INPUT_FAIL == DIAG_GetMenuOption(&option,
            isIpcRegistered ? MENU_IPC_KER_BUF_RELEASE : MENU_IPC_REGISTER))
        {
            continue;
        }

        switch (option)
        {
        case MENU_IPC_EXIT: /* Exit menu */
            break;

        case MENU_IPC_REGISTER:
            if (isIpcRegistered)
                printf("Process already registered\n");
            else
                dwSubGroupID = WDS_DIAG_IpcRegister();
            break;

        case MENU_IPC_UNREGISTER:
            if (isIpcRegistered)
                WDS_IpcUnRegister();
            printf("Process unregistered successfully\n");
            break;

        case MENU_IPC_GET_GROUP_IDS:
            WDS_DIAG_IpcScanProcs();
            break;

        case MENU_IPC_SEND_UID_UNICAST:
        case MENU_IPC_SEND_SUBGROUP_MULTICAST:
        case MENU_IPC_SEND_MULTICAST:
            WDS_DIAG_IpcSend(option);
            break;

        case MENU_IPC_ENABLE_SHARED_INTS:
            WDS_DIAG_IpcSharedIntsEnable();
            break;
        case MENU_IPC_GLOBAL_DISABLE_SHARED_INTS:
            /* After global disable, shared interrupts must be disabled
             * locally */
            if (WDS_SharedIntDisableGlobal() == WD_STATUS_SUCCESS)
                printf("\nShared ints successfully disabled globally\n");
            else
                printf("\nShared ints already disabled globally\n");
        case MENU_IPC_LOCAL_DISABLE_SHARED_INTS:
            if (WDS_SharedIntDisableLocal() == WD_STATUS_SUCCESS)
                printf("\nShared ints successfully disabled locally\n");
            else
                printf("\nShared ints already disabled locally\n");
            break;
        case MENU_IPC_KER_BUF_ALLOC_AND_SHARE:
            WDS_DIAG_IpcKerBufAllocAndShare();
            break;

        case MENU_IPC_KER_BUF_RELEASE:
            WDS_DIAG_IpcKerBufRelease();
            break;
        }
    } while (MENU_IPC_EXIT != option);
}

DWORD WDS_DIAG_IpcSendDmaContigToGroup(WD_DMA *pDma)
{
    DWORD dwStatus;

    if (!pDma)
    {
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Error - DMA ctx is NULL\n");
        return WD_INVALID_PARAMETER;
    }

    if (!(pDma->dwOptions & DMA_KERNEL_BUFFER_ALLOC))
    {
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Error - Sharing SG DMA is "
            "not supported\n");
        return WD_INVALID_PARAMETER;
    }

    dwStatus = WDS_IpcMulticast(IPC_MSG_CONTIG_DMA_BUFFER_READY,
        WDC_DMAGetGlobalHandle(pDma));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Failed sending message. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    printf("DMA contiguous buffer handle sent successfully\n");
    return WD_STATUS_SUCCESS;
}

#endif /* !defined(__KERNEL__) */


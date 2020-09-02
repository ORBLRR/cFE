/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/*
**  File:
**    cfe_es_api.c
**
**  Purpose:
**    This file implements the cFE Executive Services API functions.
**
**  References:
**     Flight Software Branch C Coding Standard Version 1.0a
**     cFE Flight Software Application Developers Guide
**
**  Notes:
**
*/

/*
** Required header files.
*/
#include "private/cfe_private.h"
#include "cfe_es.h"
#include "cfe_es_apps.h"
#include "cfe_es_global.h"
#include "cfe_es_events.h"
#include "cfe_es_cds.h"
#include "cfe_es_cds_mempool.h"
#include "cfe_es_task.h"
#include "cfe_psp.h"
#include "cfe_es_log.h"


#include <string.h>
#include <stdio.h>
#include <stdarg.h>


/*
** Function: CFE_ES_GetResetType - See API and header file for details
*/
int32 CFE_ES_GetResetType(uint32 *ResetSubtypePtr)
{
    if ( ResetSubtypePtr != NULL )
    {
       *ResetSubtypePtr = CFE_ES_ResetDataPtr->ResetVars.ResetSubtype;
    }

    return(CFE_ES_ResetDataPtr->ResetVars.ResetType);

} /* End of CFE_ES_GetResetType() */


/*
** Function: CFE_ES_ResetCFE - See API and header file for details
*/
int32 CFE_ES_ResetCFE(uint32 ResetType)
{
    int32 ReturnCode;

    if ( ResetType == CFE_PSP_RST_TYPE_PROCESSOR )
    {
       /*
       ** Increment the processor reset count
       */
       CFE_ES_ResetDataPtr->ResetVars.ProcessorResetCount++;

       /*
       ** Before doing a Processor reset, check to see
       ** if the maximum number has been exceeded
       */
       if ( CFE_ES_ResetDataPtr->ResetVars.ProcessorResetCount >
            CFE_ES_ResetDataPtr->ResetVars.MaxProcessorResetCount )
       {
           CFE_ES_WriteToSysLog("POWER ON RESET due to max proc resets (Commanded).\n");

           /*
           ** Log the reset in the ER Log. The log will be wiped out, but it's good to have
           ** the entry just in case something fails.
           */
           CFE_ES_WriteToERLog(CFE_ES_LogEntryType_CORE, CFE_PSP_RST_TYPE_POWERON,
                                         CFE_PSP_RST_SUBTYPE_RESET_COMMAND,
                                         "POWER ON RESET due to max proc resets (Commanded).");
           /*
           ** Call the BSP reset routine
           */
           CFE_PSP_Restart(CFE_PSP_RST_TYPE_POWERON);
       }
       else
       {
           CFE_ES_WriteToSysLog("PROCESSOR RESET called from CFE_ES_ResetCFE (Commanded).\n");

           /*
           ** Update the reset variables
           */
           CFE_ES_ResetDataPtr->ResetVars.ES_CausedReset = true;

           /*
           ** Log the reset in the ER Log
           */
           CFE_ES_WriteToERLog(CFE_ES_LogEntryType_CORE, CFE_PSP_RST_TYPE_PROCESSOR,
                                       CFE_PSP_RST_SUBTYPE_RESET_COMMAND,
                                       "PROCESSOR RESET called from CFE_ES_ResetCFE (Commanded).");
           /*
           ** Call the BSP reset routine
           */
           CFE_PSP_Restart(CFE_PSP_RST_TYPE_PROCESSOR);

       } /* end if */

       /*
       ** If the BSP routine is not implemented,
       ** it will return.
       */
       ReturnCode = CFE_ES_NOT_IMPLEMENTED;
    }
    else if ( ResetType == CFE_PSP_RST_TYPE_POWERON )
    {
       CFE_ES_WriteToSysLog("POWERON RESET called from CFE_ES_ResetCFE (Commanded).\n");

       /*
       ** Log the reset in the ER Log. The log will be wiped out, but it's good to have
       ** the entry just in case something fails.
       */
       CFE_ES_WriteToERLog(CFE_ES_LogEntryType_CORE, CFE_PSP_RST_TYPE_POWERON,
                                       CFE_PSP_RST_SUBTYPE_RESET_COMMAND,
                                       "POWERON RESET called from CFE_ES_ResetCFE (Commanded).");

       /*
       ** Call the BSP reset routine
       */
       CFE_PSP_Restart(CFE_PSP_RST_TYPE_POWERON);

       /*
       ** If the BSP routine is not implemented,
       ** it will return.
       */
       ReturnCode = CFE_ES_NOT_IMPLEMENTED;
    }
    else
    {
       CFE_ES_WriteToSysLog("ES ResetCFE: Invalid Reset Type: %d.\n",(int)ResetType);
       ReturnCode = CFE_ES_BAD_ARGUMENT;
    }

    return(ReturnCode);

} /* End of CFE_ES_ResetCFE() */

/*
** Function: CFE_ES_RestartApp - See API and header file for details
*/
int32 CFE_ES_RestartApp(uint32 AppID)
{
    int32 ReturnCode = CFE_SUCCESS;
    CFE_ES_AppRecord_t *AppRecPtr;

    AppRecPtr = CFE_ES_LocateAppRecordByID(AppID);
    if ( AppRecPtr != NULL )
    {

       CFE_ES_LockSharedData(__func__,__LINE__);

       /*
       ** Check to see if the App is an external cFE App.
       */
       if ( AppRecPtr->Type == CFE_ES_AppType_CORE )
       {
          CFE_ES_SysLogWrite_Unsync ("CFE_ES_RestartApp: Cannot Restart a CORE Application: %s.\n",
                             AppRecPtr->StartParams.Name );
          ReturnCode = CFE_ES_ERR_APPID;
       }
       else if ( AppRecPtr->AppState != CFE_ES_AppState_RUNNING )
       {
          CFE_ES_SysLogWrite_Unsync ("CFE_ES_RestartApp: Cannot Restart Application %s, It is not running.\n",
                              AppRecPtr->StartParams.Name);
          ReturnCode = CFE_ES_ERR_APPID;
       }
       else
       {
          CFE_ES_SysLogWrite_Unsync("CFE_ES_RestartApp: Restart Application %s Initiated\n",
                             AppRecPtr->StartParams.Name);
          AppRecPtr->ControlReq.AppControlRequest = CFE_ES_RunStatus_SYS_RESTART;
       }

       CFE_ES_UnlockSharedData(__func__,__LINE__);
    }
    else /* App ID is not valid */
    {
       ReturnCode = CFE_ES_ERR_APPID;

       CFE_ES_WriteToSysLog("CFE_ES_RestartApp: Invalid Application ID received, AppID = %lu\n",
                            CFE_ES_ResourceID_ToInteger(AppID));

    } /* end if */

    return(ReturnCode);

} /* End of CFE_ES_RestartApp() */

/*
** Function: CFE_ES_ReloadApp - See API and header file for details
*/
int32 CFE_ES_ReloadApp(uint32 AppID, const char *AppFileName)
{
    int32 ReturnCode = CFE_SUCCESS;
    os_fstat_t FileStatus;
    CFE_ES_AppRecord_t *AppRecPtr = CFE_ES_LocateAppRecordByID(AppID);

    if ( AppRecPtr == NULL )
    {
        return CFE_ES_ERR_APPID;
    }

    CFE_ES_LockSharedData(__func__,__LINE__);

    /*
    ** Check to see if the App is an external cFE App.
    */
    if ( AppRecPtr->Type == CFE_ES_AppType_CORE )
    {
       CFE_ES_SysLogWrite_Unsync ("CFE_ES_DeleteApp: Cannot Reload a CORE Application: %s.\n",
       AppRecPtr->StartParams.Name );
       ReturnCode = CFE_ES_ERR_APPID;
    }
    else if ( AppRecPtr->AppState != CFE_ES_AppState_RUNNING )
    {
       CFE_ES_SysLogWrite_Unsync ("CFE_ES_ReloadApp: Cannot Reload Application %s, It is not running.\n",
                              AppRecPtr->StartParams.Name);
       ReturnCode = CFE_ES_ERR_APPID;
    }
    else
    {
       /*
       ** Check to see if the file exists
       */
       if (OS_stat(AppFileName, &FileStatus) == OS_SUCCESS)
       {
           CFE_ES_SysLogWrite_Unsync("CFE_ES_ReloadApp: Reload Application %s Initiated. New filename = %s\n",
                                AppRecPtr->StartParams.Name, AppFileName);
           strncpy((char *)AppRecPtr->StartParams.FileName, AppFileName, OS_MAX_PATH_LEN);
           AppRecPtr->ControlReq.AppControlRequest = CFE_ES_RunStatus_SYS_RELOAD;
       }
       else
       {
           CFE_ES_SysLogWrite_Unsync ("CFE_ES_ReloadApp: Cannot Reload Application %s, File %s does not exist.\n",
                                  AppRecPtr->StartParams.Name,
                                  AppFileName);
           ReturnCode = CFE_ES_FILE_IO_ERR;
       }
    }

    CFE_ES_UnlockSharedData(__func__,__LINE__);

    return(ReturnCode);

} /* End of CFE_ES_ReloadApp() */

/*
** Function: CFE_ES_DeleteApp - See API and header file for details
*/
int32 CFE_ES_DeleteApp(uint32 AppID)
{
    int32 ReturnCode = CFE_SUCCESS;
    CFE_ES_AppRecord_t *AppRecPtr = CFE_ES_LocateAppRecordByID(AppID);

    if ( AppRecPtr == NULL )
    {
        return CFE_ES_ERR_APPID;
    }

    CFE_ES_LockSharedData(__func__,__LINE__);

    /*
    ** Check to see if the App is an external cFE App.
    */
    if ( AppRecPtr->Type == CFE_ES_AppType_CORE )
    {
       CFE_ES_SysLogWrite_Unsync ("CFE_ES_DeleteApp: Cannot Delete a CORE Application: %s.\n",
       AppRecPtr->StartParams.Name );
       ReturnCode = CFE_ES_ERR_APPID;
    }
    else if ( AppRecPtr->AppState != CFE_ES_AppState_RUNNING )
    {
       CFE_ES_SysLogWrite_Unsync ("CFE_ES_DeleteApp: Cannot Delete Application %s, It is not running.\n",
                              AppRecPtr->StartParams.Name);
       ReturnCode = CFE_ES_ERR_APPID;
    }
    else
    {
       CFE_ES_SysLogWrite_Unsync("CFE_ES_DeleteApp: Delete Application %s Initiated\n",
                             AppRecPtr->StartParams.Name);
       AppRecPtr->ControlReq.AppControlRequest = CFE_ES_RunStatus_SYS_DELETE;
    }

    CFE_ES_UnlockSharedData(__func__,__LINE__);

    return(ReturnCode);

} /* End of CFE_ES_DeleteApp() */

/*
** Function: CFE_ES_ExitApp - See API and header file for details
*/
void CFE_ES_ExitApp(uint32 ExitStatus)
{
    int32   ReturnCode;
    CFE_ES_AppRecord_t *AppRecPtr;

    CFE_ES_LockSharedData(__func__,__LINE__);

    /*
     * This should only be called with a valid ExitStatus, anything else is invalid 
     * and indicates a bug in the caller.
     */

    if (ExitStatus == CFE_ES_RunStatus_UNDEFINED || 
            ExitStatus >= CFE_ES_RunStatus_MAX)
    {
        CFE_ES_SysLogWrite_Unsync ("CFE_ES_ExitApp: Called with invalid status (%u).\n",
                (unsigned int)ExitStatus);

        /* revert to the ERROR status */
        ExitStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    AppRecPtr = CFE_ES_GetAppRecordByContext();
    if ( AppRecPtr != NULL )
    {
       /*
        * Set the status in the global table.
        *
        * The passed-in status should only be stored if there was no already-pending
        * request from a ground command or other source, such as an exception, etc.
        *
        * If a control request is already pending, it is assumed that this exit is
        * part of an orderly shutdown caused by that request, and therefore it
        * should not be overwritten here.
        */
       if (AppRecPtr->ControlReq.AppControlRequest == CFE_ES_RunStatus_APP_RUN)
       {
           AppRecPtr->ControlReq.AppControlRequest = ExitStatus;
       }

       /*
       ** Check to see if the App is an external cFE App.
       */
       if ( AppRecPtr->Type == CFE_ES_AppType_CORE )
       {
          /*
          ** A core app should only call this function with one of two ExitStatus codes.
          */
          if ( ExitStatus == CFE_ES_RunStatus_CORE_APP_INIT_ERROR )
          {
              CFE_ES_SysLogWrite_Unsync ("CFE_ES_ExitApp: CORE Application %s Had an Init Error.\n",
                                     AppRecPtr->StartParams.Name );

              /*
              ** Unlock the ES Shared data before calling ResetCFE
              */
              CFE_ES_UnlockSharedData(__func__,__LINE__);


              /*
              ** Do a Processor Reset the cFE
              */
              ReturnCode = CFE_ES_ResetCFE(CFE_PSP_RST_TYPE_PROCESSOR);

              /*
              ** The CFE_ES_ResetCFE function does not normally return,
              ** but it may return during unit testing. If it does,
              ** log the return code (even if it claims CFE_SUCCESS).
              */
              CFE_ES_WriteToSysLog("CFE_ES_ExitApp: CORE Application Init Error Processor Reset, RC = 0x%08X\n",
                                   (unsigned int) ReturnCode);

              return;

          }
          else if ( ExitStatus == CFE_ES_RunStatus_CORE_APP_RUNTIME_ERROR )
          {
              CFE_ES_SysLogWrite_Unsync ("CFE_ES_ExitApp: CORE Application %s Had a Runtime Error.\n",
                                     AppRecPtr->StartParams.Name );

              /*
              ** Unlock the ES Shared data before killing the main task
              */
              CFE_ES_UnlockSharedData(__func__,__LINE__);

              /*
              ** Exit this task
              */
              OS_TaskExit();

              /*
              ** Code will not return, except under unit test
              */
              return;
          }
          else
          {
              CFE_ES_SysLogWrite_Unsync ("CFE_ES_ExitApp, Cannot Exit CORE Application %s\n",
                                    AppRecPtr->StartParams.Name );
          }

       }
       else /* It is an external App */
       {

          CFE_ES_SysLogWrite_Unsync ("Application %s called CFE_ES_ExitApp\n",
                                 AppRecPtr->StartParams.Name);

          AppRecPtr->AppState = CFE_ES_AppState_STOPPED;

          /*
          ** Unlock the ES Shared data before suspending the app
          */
          CFE_ES_UnlockSharedData(__func__,__LINE__);

          /*
          ** Suspend the Application until ES kills it.
          ** It might be better to have a way of suspending the app in the OS
          */
          while(1)
          {
             OS_TaskDelay(500);
          }

       } /* end if */

   } /* end if ReturnCode == CFE_SUCCESS */

   CFE_ES_UnlockSharedData(__func__,__LINE__);

} /* End of CFE_ES_ExitApp() */

/*
** Function: CFE_ES_RunLoop - See API and header file for details
*/
bool CFE_ES_RunLoop(uint32 *RunStatus)
{
    bool   ReturnCode;
    CFE_ES_AppRecord_t *AppRecPtr;

    /*
     * call CFE_ES_IncrementTaskCounter() so this is
     * recorded as task activity for outgoing telemetry.
     *
     * This will update the counter for whatever task context
     * is calling this API, which is expected to be the main
     * task of the app.  This can be done outside of any lock
     * because each task has its own counter which is only updated
     * by itself.
     */
    CFE_ES_IncrementTaskCounter();

    /*
     * This API should generally only be called with the status as CFE_ES_RunStatus_APP_RUN.
     * Anything else gets an immediate "false" return which should cause the caller to
     * break out of its main loop.  There is no need to take the lock or do any other
     * accounting in that case.
     *
     * Note that the RunStatus really doesn't add much value here, so this also allows
     * this function to be called with NULL, with the possibility of phasing this out
     * entirely.
     */
    if ( RunStatus != NULL && *RunStatus != CFE_ES_RunStatus_APP_RUN )
    {
        return false;
    }

    CFE_ES_LockSharedData(__func__,__LINE__);

    /*
    ** Get App Record
    */
    AppRecPtr = CFE_ES_GetAppRecordByContext();
    if ( AppRecPtr != NULL )
    {
        /*
         ** App state must be RUNNING (no-op if already set to running)
         */
        if (AppRecPtr->AppState < CFE_ES_AppState_RUNNING)
        {
            AppRecPtr->AppState = CFE_ES_AppState_RUNNING;
        }

        /*
         * Check if the control request is also set to "RUN"
         * Anything else should also return false, so the the loop will exit.
         */
        if ( AppRecPtr->ControlReq.AppControlRequest == CFE_ES_RunStatus_APP_RUN )
        {
            ReturnCode = true;
        }
        else
        {
            /*
             * Just in case, also output the status, just in case the app looks at this.
             */
            if (RunStatus != NULL)
            {
                *RunStatus = AppRecPtr->ControlReq.AppControlRequest;
            }
            ReturnCode = false;
        }
    }
    else
    {
        /*
         * Cannot do anything without the AppID
         */
        CFE_ES_SysLogWrite_Unsync("CFE_ES_RunLoop Error: Cannot get AppID for the caller\n");
        ReturnCode = false;

    } /* end if Status == CFE_SUCCESS */

    CFE_ES_UnlockSharedData(__func__,__LINE__);

    return(ReturnCode);

} /* End of CFE_ES_RunLoop() */

/*
** Function: CFE_ES_WaitForSystemState - See API and header file for details
*/
int32 CFE_ES_WaitForSystemState(uint32 MinSystemState, uint32 TimeOutMilliseconds)
{
    int32 Status;
    CFE_ES_AppRecord_t *AppRecPtr;
    uint32 RequiredAppState;
    uint32 WaitTime;
    uint32 WaitRemaining;

    /*
     * Calling app is assumed to have completed its own initialization up to the point
     * it is waiting for.
     *
     * Determine the implicit app state based on the system state it is indicating
     */
    CFE_ES_LockSharedData(__func__,__LINE__);

    /*
    ** Get App Record
    */
    AppRecPtr = CFE_ES_GetAppRecordByContext();
    if ( AppRecPtr != NULL )
    {
        RequiredAppState = CFE_ES_AppState_EARLY_INIT;
        /*
         * If a core app waits for anything above "CORE_READY" then it is assumed to be RUNNING
         *
         * External apps have additional finer-grained sync:
         *  - SYSTEM_STATE_APPS_INIT requires that all apps are at least up to LATE_INIT
         *  - SYSTEM_STATE_OPERATIONAL requires that all apps are RUNNING
         *  - SYSTEM_STATE_SHUTDOWN requires that all apps are STOPPED (in concept anyway)
         */
        if (AppRecPtr->Type == CFE_ES_AppType_CORE)
        {
            if (MinSystemState >= CFE_ES_SystemState_CORE_READY)
            {
                RequiredAppState = CFE_ES_AppState_RUNNING;
            }
        }
        else if (MinSystemState >= CFE_ES_SystemState_SHUTDOWN)
        {
            RequiredAppState = CFE_ES_AppState_STOPPED;
        }
        else if (MinSystemState >= CFE_ES_SystemState_OPERATIONAL)
        {
            RequiredAppState = CFE_ES_AppState_RUNNING;
        }
        else if (MinSystemState >= CFE_ES_SystemState_APPS_INIT)
        {
            RequiredAppState = CFE_ES_AppState_LATE_INIT;
        }

        /*
         * NOTE -- a call to "CFE_ES_WaitForSystemState()" implies that the calling app MUST also
         * be in the requisite state.  This is hooked into here to avoid needing to update all existing
         * apps to add an explicit state change call, but it makes sense because if this was not done an app could
         * be waiting for itself (which will always time out).
         */
        if (AppRecPtr->AppState < RequiredAppState)
        {
            AppRecPtr->AppState = RequiredAppState;
        }

    }
    CFE_ES_UnlockSharedData(__func__,__LINE__);

    /*
     * Do the actual delay loop.
     *
     * This is only dependent on the main (startup) task updating the global variable
     * to be at least the state requested.
     */
    WaitRemaining = TimeOutMilliseconds;
    while (CFE_ES_Global.SystemState < MinSystemState)
    {
        /* TBD: Very Crude timing here, but not sure if it matters,
         * as this is only done during startup, not real work */
        if (WaitRemaining > CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC)
        {
            WaitTime = CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC;
        }
        else if (WaitRemaining > 0)
        {
            WaitTime = WaitRemaining;
        }
        else
        {
            Status = CFE_ES_OPERATION_TIMED_OUT;
            break;
        }

        OS_TaskDelay(WaitTime);
        WaitRemaining -= WaitTime;
    }

    return Status;

} /* End of CFE_ES_WaitForSystemState() */

/*
** Function: CFE_ES_WaitForStartupSync - See API and header file for details
*/
void CFE_ES_WaitForStartupSync(uint32 TimeOutMilliseconds)
{
    CFE_ES_WaitForSystemState(CFE_ES_SystemState_OPERATIONAL, TimeOutMilliseconds);
}

/*
** Function: - See API and header file for details
*/
int32 CFE_ES_RegisterApp(void)
{
   int32 Result;

   CFE_ES_LockSharedData(__func__,__LINE__);

   /*
   ** Register the task
   */
   Result = OS_TaskRegister();

   if (Result == OS_SUCCESS)
   {
       Result = CFE_SUCCESS;
   }
   else
   {
       /*
       ** Cannot create a syslog entry here because it requires the task to
       ** be registered
       */
       Result = CFE_ES_ERR_APP_REGISTER;
   }

   /*
   ** Set the default exception environment
   */
   CFE_PSP_SetDefaultExceptionEnvironment();

   CFE_ES_UnlockSharedData(__func__,__LINE__);

   return(Result);


} /* End of CFE_ES_RegisterApp() */

/*
** Function: CFE_ES_GetAppIDByName - See API and header file for details
*/
int32 CFE_ES_GetAppIDByName(uint32 *AppIdPtr, const char *AppName)
{
   int32 Result = CFE_ES_ERR_APPNAME;
   CFE_ES_AppRecord_t *AppRecPtr;
   uint32 i;

   /*
    * ensure the output value is set to a safe value,
    * in case the does not check the return code.
    */
   *AppIdPtr = 0;

   CFE_ES_LockSharedData(__func__,__LINE__);

   /*
   ** Search the ES Application table for an app with a matching name.
   */
   AppRecPtr = CFE_ES_Global.AppTable;
   for ( i = 0; i < CFE_PLATFORM_ES_MAX_APPLICATIONS; i++ )
   {
      if ( CFE_ES_AppRecordIsUsed(AppRecPtr) )
      {
         if ( strncmp(AppName, (char *)AppRecPtr->StartParams.Name, OS_MAX_API_NAME) == 0 )
         {
            *AppIdPtr = CFE_ES_AppRecordGetID(AppRecPtr);
            Result = CFE_SUCCESS;
            break;
         }
      }

      ++AppRecPtr;
   } /* end for */

   CFE_ES_UnlockSharedData(__func__,__LINE__);

   return(Result);

} /* End of CFE_ES_GetAppIDByName() */


/*
** Function: CFE_ES_GetAppID  - See API and header file for details
*/
int32 CFE_ES_GetAppID(uint32 *AppIdPtr)
{
   int32  Result;

   CFE_ES_LockSharedData(__func__,__LINE__);

   Result = CFE_ES_GetAppIDInternal(AppIdPtr);

   CFE_ES_UnlockSharedData(__func__,__LINE__);

   return(Result);

} /* End of CFE_ES_GetAppID() */

/*
** Function: CFE_ES_GetTaskID  - See API and header file for details
*/
int32 CFE_ES_GetTaskID(uint32 *TaskIdPtr)
{
    int32 Result;
    CFE_ES_TaskRecord_t *TaskRecPtr;

    CFE_ES_LockSharedData(__func__,__LINE__);
    TaskRecPtr = CFE_ES_GetTaskRecordByContext();
    if (TaskRecPtr == NULL)
    {
        *TaskIdPtr = 0;
        Result = CFE_ES_ERR_TASKID;
    }
    else
    {
        *TaskIdPtr = CFE_ES_TaskRecordGetID(TaskRecPtr);
        Result = CFE_SUCCESS;
    }
    CFE_ES_UnlockSharedData(__func__,__LINE__);
    return Result;
}

/*
** Function: CFE_ES_GetAppName - See API and header file for details
*/
int32 CFE_ES_GetAppName(char *AppName, uint32 AppId, uint32 BufferLength)
{
   int32 Result;
   CFE_ES_AppRecord_t *AppRecPtr;

   /*
   ** Get App Record
   */
   AppRecPtr = CFE_ES_LocateAppRecordByID(AppId);

   CFE_ES_LockSharedData(__func__,__LINE__);

   /*
    * confirm that the app record is a match,
    * which must be done while locked.
    */
   if (CFE_ES_AppRecordIsMatch(AppRecPtr, AppId))
   {
       strncpy(AppName, (char *)AppRecPtr->StartParams.Name, BufferLength - 1);
       AppName[BufferLength - 1] = '\0';
       Result = CFE_SUCCESS;
   }
   else
   {
      Result = CFE_ES_ERR_APPID;
   }

   CFE_ES_UnlockSharedData(__func__,__LINE__);

   /*
    * Appeasement for poorly-behaved callers:
    *
    * There is a fair amount of existing code that calls this function but
    * does not correctly check the return code.  Although these callers are
    * incorrect, this at least ensures that if the output buffer will be
    * appropriately null terminated (empty string) in the failure case.
    */
   if (Result != CFE_SUCCESS)
   {
       AppName[0] = 0;
   }

   return(Result);

} /* End of CFE_ES_GetAppName() */


/*
** Function: CFE_ES_GetAppInfo - See API and header file for details
*/
int32 CFE_ES_GetAppInfo(CFE_ES_AppInfo_t *AppInfo, uint32 AppId)
{
   int32  ReturnCode = CFE_SUCCESS;
   CFE_ES_AppRecord_t *AppRecPtr;

   if ( AppInfo == NULL )
   {
      CFE_ES_WriteToSysLog("CFE_ES_GetAppInfo: Invalid Parameter ( Null Pointer )\n");
      return CFE_ES_ERR_BUFFER;
   }

   memset(AppInfo, 0, sizeof(*AppInfo));

   /*
   ** Get App Record
   */
   AppRecPtr = CFE_ES_LocateAppRecordByID(AppId);
   if ( AppRecPtr == NULL )
   {
       CFE_ES_WriteToSysLog("CFE_ES_GetAppInfo: App ID Invalid: %lu\n",
               (unsigned long)AppId);
       return CFE_ES_ERR_APPID;
    }

   /*
    * Note - cannot check if the AppID is active here,
    * as the table is not locked.  The internal function
    * should lock and check.
    */
   ReturnCode = CFE_ES_GetAppInfoInternal(AppRecPtr, AppInfo);
   if (ReturnCode != CFE_SUCCESS)
   {
      CFE_ES_WriteToSysLog("CFE_ES_GetAppInfo: App ID Not Active: %lu\n",
              CFE_ES_ResourceID_ToInteger(AppId));
   }

   return(ReturnCode);

} /* End of CFE_ES_GetAppInfo() */

/*
** Function: CFE_ES_GetTaskInfo - See API and header file for details
*/
int32 CFE_ES_GetTaskInfo(CFE_ES_TaskInfo_t *TaskInfo, uint32 TaskId)
{
   CFE_ES_TaskRecord_t *TaskRecPtr;
   int32  ReturnCode;

   if ( TaskInfo == NULL )
   {
      CFE_ES_WriteToSysLog("CFE_ES_GetTaskInfo: Invalid Parameter ( Null Pointer )\n");
      return CFE_ES_ERR_BUFFER;
   }

   memset(TaskInfo, 0, sizeof(*TaskInfo));

   /*
   ** Get Task Record
   */
   TaskRecPtr = CFE_ES_LocateTaskRecordByID(TaskId);
   if ( TaskRecPtr == NULL )
   {
      CFE_ES_WriteToSysLog("CFE_ES_GetTaskInfo: Task ID Not Valid: %lu\n",
              CFE_ES_ResourceID_ToInteger(TaskId));
      return CFE_ES_ERR_TASKID;
   }

   /*
    * Note - cannot check if the TaskID is active here,
    * as the table is not locked.  The internal function
    * should lock and check.
    */
   ReturnCode = CFE_ES_GetTaskInfoInternal(TaskRecPtr, TaskInfo);
   if (ReturnCode != CFE_SUCCESS)
   {
      CFE_ES_WriteToSysLog("CFE_ES_GetTaskInfo: Task ID Not Active: %lu\n",
              CFE_ES_ResourceID_ToInteger(TaskId));
   }

   return(ReturnCode);

} /* End of CFE_ES_GetTaskInfo() */


/*
** Function: CFE_ES_CreateChildTask - See API and header file for details
*/
int32 CFE_ES_CreateChildTask(uint32 *TaskIdPtr,
                        const char   *TaskName,
                        CFE_ES_ChildTaskMainFuncPtr_t   FunctionPtr,
                        uint32 *StackPtr,
                        uint32  StackSize,
                        uint32  Priority,
                        uint32  Flags)
{

   int32          Result;
   CFE_ES_AppRecord_t *AppRecPtr;
   CFE_ES_TaskRecord_t *TaskRecPtr;
   int32          ReturnCode;
   uint32         TaskId;
   uint32         ChildTaskId;
   uint32         ParentTaskId;
   uint32         OsalId;

   /*
   ** Validate some of the arguments
   */
   if ( TaskIdPtr == NULL )
   {
      if (TaskName == NULL)
      {
          CFE_ES_WriteToSysLog("CFE_ES_CreateChildTask: Task Id and Name Pointer Parameters are NULL.\n");
          ReturnCode = CFE_ES_BAD_ARGUMENT;
      }
      else
      {
          CFE_ES_WriteToSysLog("CFE_ES_CreateChildTask: Task Id Pointer Parameter is NULL for Task '%s'.\n",TaskName);
          ReturnCode = CFE_ES_BAD_ARGUMENT;
      }
   }
   else if ( TaskName == NULL )
   {
      CFE_ES_WriteToSysLog("CFE_ES_CreateChildTask: TaskName Parameter is NULL\n");
      ReturnCode = CFE_ES_BAD_ARGUMENT;
   }
   else if ( FunctionPtr == NULL )
   {
      CFE_ES_WriteToSysLog("CFE_ES_CreateChildTask: Function Pointer Parameter is NULL for Task '%s'\n",TaskName);
      ReturnCode = CFE_ES_BAD_ARGUMENT;
   }
   else
   {

      CFE_ES_LockSharedData(__func__,__LINE__);

      /*
      ** Get the App Record of the calling Application
      */
      AppRecPtr = CFE_ES_GetAppRecordByContext();
      if (AppRecPtr == NULL)
      {
          CFE_ES_SysLogWrite_Unsync("CFE_ES_CreateChildTask: Invalid calling context when creating Task '%s'\n",TaskName);
          ReturnCode = CFE_ES_ERR_APPID;
      }
      else  /* else AppId is valid */
      {
         /*
         ** First, Make sure the Calling Task is a cFE Main task.
         ** TaskID must be the same as the Parent Task ID.
         */
         TaskId = OS_TaskGetId();
         ParentTaskId = AppRecPtr->TaskInfo.MainTaskId;
         if ( TaskId == ParentTaskId )
         {
            /*
            ** Truncate the priority if needed
            */
            if ( Priority >  255  )
            {
                Priority = 255;
            }

            /*
            ** Step 2: Create the new task using the OS API call
            */
            Result = OS_TaskCreate(&OsalId, TaskName, FunctionPtr, StackPtr,
                                StackSize, Priority, OS_FP_ENABLED );

            /*
            ** Step 3: Record the task information in the task table
            */
            if ( Result == OS_SUCCESS )
            {
               ChildTaskId = OsalId;
               TaskRecPtr = CFE_ES_LocateTaskRecordByID(ChildTaskId);

               CFE_ES_TaskRecordSetUsed(TaskRecPtr, ChildTaskId);
               TaskRecPtr->AppId = CFE_ES_AppRecordGetID(AppRecPtr);
               strncpy((char *)TaskRecPtr->TaskName,TaskName,OS_MAX_API_NAME);
               TaskRecPtr->TaskName[OS_MAX_API_NAME - 1] = '\0';
               CFE_ES_Global.RegisteredTasks++;

               *TaskIdPtr = ChildTaskId;
               ReturnCode = CFE_SUCCESS;
            }
            else
            {
               CFE_ES_SysLogWrite_Unsync("CFE_ES_CreateChildTask: Error calling OS_TaskCreate for Task '%s' RC = 0x%08X\n",TaskName,(unsigned int)Result);
               ReturnCode = CFE_ES_ERR_CHILD_TASK_CREATE;
            }
         }
         else
         {
            CFE_ES_SysLogWrite_Unsync("CFE_ES_CreateChildTask: Error: Cannot call from a Child Task (for Task '%s').\n",TaskName);
            ReturnCode = CFE_ES_ERR_CHILD_TASK_CREATE;

         } /* end if Calling task is a main task */

      }/* end If AppID is valid */

      CFE_ES_UnlockSharedData(__func__,__LINE__);

   } /* end if parameter checking */

   return(ReturnCode);

} /* End of CFE_ES_CreateChildTask() */


/*
** Function: CFE_ES_RegisterChildTask - See API and header file for details
*/
int32 CFE_ES_RegisterChildTask(void)
{
   int32 Result;
   int32 ReturnCode;

   CFE_ES_LockSharedData(__func__,__LINE__);

   /*
   ** Register the task with the OS
   */
   Result = OS_TaskRegister();

   if (Result != OS_SUCCESS)
   {
       /*
       ** Cannot create a syslog entry here because it requires the task to
       ** be registered
       */
       ReturnCode = CFE_ES_ERR_CHILD_TASK_REGISTER;
   }
   else
   {
       ReturnCode = CFE_SUCCESS;
   }

   /*
   ** Set the default exception environment
   */
   CFE_PSP_SetDefaultExceptionEnvironment();

   CFE_ES_UnlockSharedData(__func__,__LINE__);

   return(ReturnCode);

} /* End of CFE_ES_RegisterChildTask() */

/*
** Function: CFE_ES_IncrementTaskCounter - See API and header file for details
*/
void CFE_ES_IncrementTaskCounter(void)
{
    CFE_ES_TaskRecord_t *TaskRecPtr;
    uint32 TaskID;

    /*
     * Note this locates a task record but intentionally does _not_
     * lock the global data structure.  Locking is avoided for
     * efficiency reasons.
     *
     * As tasks can only increment their own counters, there is no risk
     * of concurrent access to the same counter.
     *
     * Because the global data is not locked, only minimal validation
     * is performed.
     */
    TaskID = OS_TaskGetId();
    TaskRecPtr = CFE_ES_LocateTaskRecordByID(TaskID);
    if (TaskRecPtr != NULL)
    {
        TaskRecPtr->ExecutionCounter++;
    }

} /* End of CFE_ES_IncrementTaskCounter() */



/*
** Function: CFE_ES_DeleteChildTask - See API and header file for details
*/
int32 CFE_ES_DeleteChildTask(uint32 TaskId)
{
    CFE_ES_TaskRecord_t *TaskRecPtr;
    CFE_ES_AppRecord_t *AppRecPtr;
    uint32  i;
    bool    TaskIsMain = false;
    int32   ReturnCode = CFE_SUCCESS;
    int32   OSReturnCode;

    /*
    ** Make sure the task ID is within range
    */
    TaskRecPtr = CFE_ES_LocateTaskRecordByID(TaskId);
    if (TaskRecPtr != NULL)
    {
       CFE_ES_LockSharedData(__func__,__LINE__);

       /*
       ** Make sure the task is active/valid
       */
       if (CFE_ES_TaskRecordIsMatch(TaskRecPtr, TaskId))
       {
          /*
          ** Search for this task ID in the ES App Table to make sure
          ** it is not a cFE App Main Task
          */
          TaskIsMain = false;
          AppRecPtr = CFE_ES_Global.AppTable;
          for ( i = 0; i < CFE_PLATFORM_ES_MAX_APPLICATIONS; i++ )
          {
             if ( CFE_ES_AppRecordIsUsed(AppRecPtr) )
             {
                if ( AppRecPtr->TaskInfo.MainTaskId == TaskId )
                {
                   /*
                   ** Error, the task Id is an App Main Task ID
                   */
                   TaskIsMain = true;
                   break;
                } /* end if */
             } /* end if */
             ++AppRecPtr;
          } /* end for */

          if ( TaskIsMain == false )
          {
             /*
             ** Can delete the Task
             */
             OSReturnCode = OS_TaskDelete(TaskId);
             if ( OSReturnCode == OS_SUCCESS )
             {
                /*
                ** Invalidate the task table entry
                */
                CFE_ES_TaskRecordSetFree(TaskRecPtr);
                CFE_ES_Global.RegisteredTasks--;

                /*
                ** Report the task delete
                */
                CFE_ES_SysLogWrite_Unsync("CFE_ES_DeleteChildTask Task %lu Deleted\n",
                        CFE_ES_ResourceID_ToInteger(TaskId));
                ReturnCode = CFE_SUCCESS;
             }
             else
             {
                CFE_ES_SysLogWrite_Unsync("CFE_ES_DeleteChildTask Error: Error Calling OS_TaskDelete: Task %lu, RC = 0x%08X\n",
                        CFE_ES_ResourceID_ToInteger(TaskId), (unsigned int)OSReturnCode);
                ReturnCode = CFE_ES_ERR_CHILD_TASK_DELETE;
             }
          }
          else
          {
             /*
             ** Error: The task is a cFE Application Main task
             */
             CFE_ES_SysLogWrite_Unsync("CFE_ES_DeleteChildTask Error: Task %lu is a cFE Main Task.\n",
                     CFE_ES_ResourceID_ToInteger(TaskId));
             ReturnCode = CFE_ES_ERR_CHILD_TASK_DELETE_MAIN_TASK;
          } /* end if TaskMain == false */
       }
       else
       {
          /*
          ** Task ID is not in use, so it is invalid
          */
          CFE_ES_SysLogWrite_Unsync("CFE_ES_DeleteChildTask Error: Task ID is not active: %lu\n",
                  CFE_ES_ResourceID_ToInteger(TaskId));
          ReturnCode = CFE_ES_ERR_TASKID;

       } /* end if */

       CFE_ES_UnlockSharedData(__func__,__LINE__);

    }
    else
    {
       /*
       ** Task ID is invalid ( too large )
       */
       CFE_ES_WriteToSysLog("CFE_ES_DeleteChildTask Error: Invalid Task ID: %lu\n",
               CFE_ES_ResourceID_ToInteger(TaskId));
       ReturnCode = CFE_ES_ERR_TASKID;

    }
    return(ReturnCode);

} /* End of CFE_ES_DeleteTask() */

/*
** Function: CFE_ES_ExitChildTask
**
** Purpose:  Stop execution of a child task.
**
*/
void CFE_ES_ExitChildTask(void)
{
   CFE_ES_AppRecord_t *AppRecPtr;
   CFE_ES_TaskRecord_t *TaskRecPtr;

   CFE_ES_LockSharedData(__func__,__LINE__);

   /*
   ** Check to see if this is being called from a cFE Application's
   ** main task.
   */
   TaskRecPtr = CFE_ES_GetTaskRecordByContext();
   if ( TaskRecPtr != NULL )
   {
       AppRecPtr = CFE_ES_LocateAppRecordByID(TaskRecPtr->AppId);

       if ( CFE_ES_AppRecordIsMatch(AppRecPtr, TaskRecPtr->AppId) &&
               !CFE_ES_TaskRecordIsMatch(TaskRecPtr, AppRecPtr->TaskInfo.MainTaskId))
      {
            /*
            ** Invalidate the task table entry
            */
            CFE_ES_TaskRecordSetFree(TaskRecPtr);
            CFE_ES_Global.RegisteredTasks--;


            CFE_ES_UnlockSharedData(__func__,__LINE__);


            /*
            ** Call the OS AL routine
            */
            OS_TaskExit();
            /*
            ** Does not return from OS_TaskExit, except under unit test
            */
            return;
      }
      else
      {
         CFE_ES_SysLogWrite_Unsync("CFE_ES_ExitChildTask Error: Cannot Call from a cFE App Main Task. ID = %lu\n",
                 CFE_ES_ResourceID_ToInteger(CFE_ES_TaskRecordGetID(TaskRecPtr)));
      }
   }
   else
   {
      CFE_ES_SysLogWrite_Unsync("CFE_ES_ExitChildTask called from invalid task context\n");
   } /* end if GetAppId */

   CFE_ES_UnlockSharedData(__func__,__LINE__);

} /* End of CFE_ES_ExitChildTask() */


/*
** Function: CFE_ES_WriteToSysLog - See API and header file for details
*/
int32 CFE_ES_WriteToSysLog(const char *SpecStringPtr, ...)
{
    char          TmpString[CFE_ES_MAX_SYSLOG_MSG_SIZE];
    int32         ReturnCode;
    va_list       ArgPtr;

    va_start(ArgPtr, SpecStringPtr);
    CFE_ES_SysLog_vsnprintf(TmpString, sizeof(TmpString), SpecStringPtr, ArgPtr);
    va_end(ArgPtr);

    /*
     * Append to the syslog buffer, which must be done while locked.
     * Only one thread can actively write into the buffer at time.
     */
    CFE_ES_LockSharedData(__func__, __LINE__);
    ReturnCode = CFE_ES_SysLogAppend_Unsync(TmpString);
    CFE_ES_UnlockSharedData(__func__, __LINE__);

    /* Output the entry to the console */
    OS_printf("%s",TmpString);

    return(ReturnCode);

} /* End of CFE_ES_WriteToSysLog() */


/*
** Function: CFE_ES_CalculateCRC - See API and header file for details
*/
uint32 CFE_ES_CalculateCRC(const void *DataPtr, uint32 DataLength, uint32 InputCRC, uint32 TypeCRC)
{
    uint32  i;
    int16  Index;
    int16  Crc = 0;
    const uint8 *BufPtr;
    uint8  ByteValue;

    static const uint16 CrcTable[256]=
    {

		    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
		    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
		    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
		    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
		    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
		    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
		    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
		    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
		    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
		    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
		    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
		    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
		    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
		    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
		    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
		    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
		    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
		    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
		    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
		    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
		    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
		    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
		    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
		    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
		    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
		    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
		    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
		    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
		    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
		    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
		    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
		    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040

    };

    switch(TypeCRC)
    {
      case CFE_MISSION_ES_CRC_32:
           CFE_ES_WriteToSysLog("CFE ES Calculate CRC32 not Implemented\n");
           break;

      case CFE_MISSION_ES_CRC_16:
           Crc    =  (int16 )( 0xFFFF & InputCRC );
           BufPtr = (const uint8 *)DataPtr;

           for ( i = 0 ; i < DataLength ; i++,  BufPtr++)
           {
              /*
               * It is assumed that the supplied buffer is in a
               * directly-accessible memory space that does not
               * require special logic to access
               */
              ByteValue = *BufPtr;
              Index = ( ( Crc ^ ByteValue) & 0x00FF);
              Crc = ( (Crc >> 8 ) & 0x00FF) ^ CrcTable[Index];
           }
           break;

      case CFE_MISSION_ES_CRC_8:
           CFE_ES_WriteToSysLog("CFE ES Calculate CRC8 not Implemented\n");
           break;

      default:
           break;
    }
    return(Crc);

} /* End of CFE_ES_CalculateCRC() */


/*
** Function: CFE_ES_RegisterCDS
**
** Purpose:  Allocate a data block for a Critical Data Store.
**
*/
int32 CFE_ES_RegisterCDS(CFE_ES_CDSHandle_t *CDSHandlePtr, int32 BlockSize, const char *Name)
{
    int32   Status;
    size_t  NameLen = 0;
    uint32  ThisAppId;

    char    AppName[OS_MAX_API_NAME] = {"UNKNOWN"};
    char    CDSName[CFE_ES_CDS_MAX_FULL_NAME_LEN] = {""};

    /* Check to make sure calling application is legit */
    Status = CFE_ES_GetAppID(&ThisAppId);

    if ( Status != CFE_SUCCESS )  /* Application ID was invalid */
    {
         CFE_ES_WriteToSysLog("CFE_CDS:Register-Bad AppId context\n");
    }
    else if (CFE_ES_Global.CDSVars.MemPoolSize == 0)
    {
        CFE_ES_WriteToSysLog("CFE_CDS:Register-CDS not available\n");
        Status = CFE_ES_NOT_IMPLEMENTED;
    }
    else
    {
        /* Assume we can't make a CDS and return a bad handle for now */
        *CDSHandlePtr = CFE_ES_CDS_BAD_HANDLE;

        /* Make sure specified CDS name is not too long or too short */
        NameLen = strlen(Name);
        if ((NameLen > CFE_MISSION_ES_CDS_MAX_NAME_LENGTH) || (NameLen == 0))
        {
           Status = CFE_ES_CDS_INVALID_NAME;

           /* Perform a buffer overrun safe copy of name for debug log message */

           strncpy(CDSName, Name, CFE_MISSION_ES_CDS_MAX_NAME_LENGTH);
           CDSName[CFE_MISSION_ES_CDS_MAX_NAME_LENGTH-1] = '\0';
           CFE_ES_WriteToSysLog("CFE_CDS:Register-CDS Name (%s) is too long\n", CDSName);
        }
        else
        {
           /* Modify specified name to be processor specific name */
           /* of the form "AppName.Name"                          */
           CFE_ES_FormCDSName(CDSName, Name, ThisAppId);

           /* Make sure the specified size is acceptable */
           if (BlockSize == 0)
           {
              Status = CFE_ES_CDS_INVALID_SIZE;
              CFE_ES_WriteToSysLog("CFE_CDS:Register-CDS %s has size of zero\n", Name);
           }
           else
           {
              /* Create CDS and designate it as NOT being a Critical Table */
              Status = CFE_ES_RegisterCDSEx(CDSHandlePtr, BlockSize, CDSName, false);
           }
        }
    }

    /* On Error conditions, notify ground of screw up */
    if (Status < 0)
    {
        /* Translate AppID of caller into App Name */
        CFE_ES_GetAppName(AppName, ThisAppId, OS_MAX_API_NAME);

        CFE_EVS_SendEventWithAppID(CFE_ES_CDS_REGISTER_ERR_EID,
                                   CFE_EVS_EventType_ERROR,
                                   ThisAppId,
                                   "%s Failed to Register CDS '%s', Status=0x%08X",
                                   AppName, Name, (unsigned int)Status);
    }

    return Status;
} /* End of CFE_ES_RegisterCDS */

/*
** Function: CFE_ES_CopyToCDS
**
** Purpose:  Copies a data block to a Critical Data Store.
**
*/
int32 CFE_ES_CopyToCDS(CFE_ES_CDSHandle_t Handle, void *DataToCopy)
{
    int32 Status;

    Status = CFE_ES_CDSBlockWrite(CFE_ES_Global.CDSVars.Registry[Handle].MemHandle, DataToCopy);

    return Status;
} /* End of CFE_ES_CopyToCDS() */

/*
** Function: CFE_ES_RestoreFromCDS
**
** Purpose:  Restores a data block from a Critical Data Store.
**
*/
int32 CFE_ES_RestoreFromCDS(void *RestoreToMemory, CFE_ES_CDSHandle_t Handle)
{
    int32 Status;

    Status = CFE_ES_CDSBlockRead(RestoreToMemory, CFE_ES_Global.CDSVars.Registry[Handle].MemHandle);

    return Status;
} /* End of CFE_ES_RestoreFromCDS() */

/* end of file */


int32 CFE_ES_RegisterGenCounter(uint32 *CounterIdPtr, const char *CounterName)
{
   int32 ReturnCode = CFE_ES_BAD_ARGUMENT;
   uint32 CheckPtr;
   int32 Status;
   uint32 i;

   Status = CFE_ES_GetGenCounterIDByName(&CheckPtr, CounterName);

   if ((CounterIdPtr != NULL) && (CounterName != NULL) && (Status != CFE_SUCCESS))
   {
      for ( i = 0; i < CFE_PLATFORM_ES_MAX_GEN_COUNTERS; i++ )
      {
         if ( CFE_ES_Global.CounterTable[i].RecordUsed == false )
         {
            strncpy((char *)CFE_ES_Global.CounterTable[i].CounterName,CounterName,OS_MAX_API_NAME);

            CFE_ES_Global.CounterTable[i].RecordUsed = true;
            CFE_ES_Global.CounterTable[i].Counter = 0;
            *CounterIdPtr = i;
            break;
         }
      }
      if (i < CFE_PLATFORM_ES_MAX_GEN_COUNTERS)
      {
         ReturnCode = CFE_SUCCESS;
      }
   }

   return ReturnCode;

}

/*
** Function: CFE_ES_DeleteGenCounter
**
** Purpose:  Delete a Generic Counter.
**
*/
int32 CFE_ES_DeleteGenCounter(uint32 CounterId)
{

   int32 Status = CFE_ES_BAD_ARGUMENT;

   if(CounterId < CFE_PLATFORM_ES_MAX_GEN_COUNTERS)
   {
      CFE_ES_Global.CounterTable[CounterId].RecordUsed = false;
      CFE_ES_Global.CounterTable[CounterId].Counter = 0;
      Status = CFE_SUCCESS;
   }

   return Status;

} /* End of CFE_ES_DeleteGenCounter() */

/*
** Function: CFE_ES_IncrementGenCounter
**
** Purpose:  Increment a Generic Counter.
**
*/
int32 CFE_ES_IncrementGenCounter(uint32 CounterId)
{
   int32 Status = CFE_ES_BAD_ARGUMENT;

   if((CounterId < CFE_PLATFORM_ES_MAX_GEN_COUNTERS) &&
      (CFE_ES_Global.CounterTable[CounterId].RecordUsed == true))
   {
      CFE_ES_Global.CounterTable[CounterId].Counter++;
      Status = CFE_SUCCESS;
   }
   return Status;

} /* End of CFE_ES_IncrementGenCounter() */

/*
** Function: CFE_ES_SetGenCount
**
** Purpose:  Sets a Generic Counter's count.
**
*/
int32 CFE_ES_SetGenCount(uint32 CounterId, uint32 Count)
{
   int32 Status = CFE_ES_BAD_ARGUMENT;

   if((CounterId < CFE_PLATFORM_ES_MAX_GEN_COUNTERS) &&
      (CFE_ES_Global.CounterTable[CounterId].RecordUsed == true))
   {
      CFE_ES_Global.CounterTable[CounterId].Counter = Count;
      Status = CFE_SUCCESS;
   }
   return Status;
} /* End of CFE_ES_SetGenCount() */

/*
** Function: CFE_ES_GetGenCount
**
** Purpose:  Gets the value of a Generic Counter.
**
*/
int32 CFE_ES_GetGenCount(uint32 CounterId, uint32 *Count)
{
   int32 Status = CFE_ES_BAD_ARGUMENT;

   if((CounterId < CFE_PLATFORM_ES_MAX_GEN_COUNTERS) &&
      (CFE_ES_Global.CounterTable[CounterId].RecordUsed == true) &&
      (Count != NULL ))
   {
      *Count = CFE_ES_Global.CounterTable[CounterId].Counter;
      Status = CFE_SUCCESS;
   }
   return Status;
} /* End of CFE_ES_GetGenCount() */

int32 CFE_ES_GetGenCounterIDByName(uint32 *CounterIdPtr, const char *CounterName)
{

   int32 Result = CFE_ES_BAD_ARGUMENT;
   uint32   i;

   /*
   ** Search the ES Generic Counter table for a counter with a matching name.
   */
   for ( i = 0; i < CFE_PLATFORM_ES_MAX_GEN_COUNTERS; i++ )
   {
      if ( CFE_ES_Global.CounterTable[i].RecordUsed == true )
      {
         if ( strncmp(CounterName, (char *)CFE_ES_Global.CounterTable[i].CounterName, OS_MAX_API_NAME) == 0 )
         {
            if(CounterIdPtr != NULL)
            {
               *CounterIdPtr = i;
               Result = CFE_SUCCESS;
            }
            break;
         }
      }
   } /* end for */

   return(Result);

} /* End of CFE_ES_GetGenCounterIDByName() */


/***************************************************************************************
** Private API functions
*/

/**
 * A conversion function to obtain an index value correlating to an AppID
 * This is a zero based value that can be used for indexing into a table.
 */
int32 CFE_ES_AppID_ToIndex(uint32 AppId, uint32 *Idx)
{
    if (AppId >= CFE_PLATFORM_ES_MAX_APPLICATIONS)
    {
        return CFE_ES_ERR_APPID;
    }

    /*
     * Currently this is a direct/simple pass through.
     * Will evolve in a future rev to make it more safe.
     */
    *Idx = AppId;
    return CFE_SUCCESS;
}

/**
 * A conversion function to obtain an index value correlating to an TaskID
 * This is a zero based value that can be used for indexing into a table.
 * 
 * Task IDs come from OSAL, so this is currently a wrapper around the OSAL converter.
 * This is an alias for consistency with the ES AppID paradigm.
 */
int32 CFE_ES_TaskID_ToIndex(uint32 TaskID, uint32 *Idx)
{
    if (OS_ConvertToArrayIndex(TaskID, Idx) != OS_SUCCESS)
    {
        return CFE_ES_ERR_TASKID;
    }

    return CFE_SUCCESS;
}

/*
 * Note - this gets the table entry pointer but does not dereference or
 * otherwise check/validate said pointer, as that would have to be done while
 * locked.
 */
CFE_ES_AppRecord_t *CFE_ES_LocateAppRecordByID(uint32 AppID)
{
    CFE_ES_AppRecord_t *AppRecPtr;
    uint32 Idx;

    if (CFE_ES_AppID_ToIndex(AppID, &Idx) == CFE_SUCCESS)
    {
        AppRecPtr = &CFE_ES_Global.AppTable[Idx];
    }
    else
    {
        AppRecPtr = NULL;
    }

    return AppRecPtr;
}

/*
 * Note - this gets the table entry pointer but does not dereference or
 * otherwise check/validate said pointer, as that would have to be done while
 * locked.
 */
CFE_ES_TaskRecord_t *CFE_ES_LocateTaskRecordByID(uint32 TaskID)
{
    CFE_ES_TaskRecord_t *TaskRecPtr;
    uint32 Idx;

    if (CFE_ES_TaskID_ToIndex(TaskID, &Idx) == CFE_SUCCESS)
    {
        TaskRecPtr = &CFE_ES_Global.TaskTable[Idx];
    }
    else
    {
        TaskRecPtr = NULL;
    }

    return TaskRecPtr;
}

/*
 * This function does additional validation on the task record
 * and should only be called when global data is locked.
 */
CFE_ES_TaskRecord_t *CFE_ES_GetTaskRecordByContext(void)
{
    CFE_ES_TaskRecord_t *TaskRecPtr;
    uint32 TaskID;

    /*
    ** Use the OS task ID to get the ES task record
    */
    TaskID = OS_TaskGetId();
    TaskRecPtr = CFE_ES_LocateTaskRecordByID(TaskID);

    /*
     * Confirm that the entry is actually a match (this requires/assumes
     * the global data is locked).
     *
     * If not a match, return NULL.
     */
    if (!CFE_ES_TaskRecordIsMatch(TaskRecPtr, TaskID))
    {
        TaskRecPtr = NULL;
    }

    return TaskRecPtr;
}

CFE_ES_AppRecord_t *CFE_ES_GetAppRecordByContext(void)
{
    CFE_ES_AppRecord_t *AppRecPtr;
    CFE_ES_TaskRecord_t *TaskRecPtr;

    /*
    ** Step 1: Get the task record
    */
    TaskRecPtr = CFE_ES_GetTaskRecordByContext();
    if (TaskRecPtr != NULL)
    {
        /*
        ** Step 2: get the Application ID for the current task
        */
        AppRecPtr = CFE_ES_LocateAppRecordByID(TaskRecPtr->AppId);

        /*
         * Confirm that the entry is actually a match (this requires/assumes
         * the global data is locked).
         *
         * If not a match, return NULL.
         */
        if (!CFE_ES_AppRecordIsMatch(AppRecPtr, TaskRecPtr->AppId))
        {
            AppRecPtr = NULL;
        }
    }
    else
    {
        AppRecPtr = NULL;
    }

    return AppRecPtr;
}



/*
** Function: CFE_ES_GetAppIDInternal
**
** Purpose:  Return the Caller's cFE Application ID. This internal version is needed
**            so there are not nested calls to the ES Shared Data mutex lock.
**
*/
int32 CFE_ES_GetAppIDInternal(uint32 *AppIdPtr)
{
    CFE_ES_AppRecord_t *AppRecPtr;
    int32 Result;

    AppRecPtr = CFE_ES_GetAppRecordByContext();

    if (AppRecPtr != NULL)
    {
        *AppIdPtr = CFE_ES_AppRecordGetID(AppRecPtr);
        Result = CFE_SUCCESS;
    }
    else
    {
        *AppIdPtr = 0;
        Result = CFE_ES_ERR_APPID;
    }

    return Result;

} /* End of CFE_ES_GetAppIDInternal() */


/******************************************************************************
**  Function:  CFE_ES_LockSharedData()
**
**  Purpose:
**    ES internal function to take the Shared Data Mutex and handle
**    error conditions.
**
**  Arguments:
**    FunctionName   - the name of the function containing the code that generated the error.
**    LineNumber     - the file line number of the code that generated the error.
**
**  Return:
**    None
*/
void CFE_ES_LockSharedData(const char *FunctionName, int32 LineNumber)
{

    int32   Status;
    uint32  AppId;

    Status = OS_MutSemTake(CFE_ES_Global.SharedDataMutex);
    if (Status != OS_SUCCESS)
    {
        CFE_ES_GetAppIDInternal(&AppId);

        /*
         * NOTE: this is going to write into a buffer that itself
         * is _supposed_ to be protected by this same mutex.
         */
        CFE_ES_SysLogWrite_Unsync("ES SharedData Mutex Take Err Stat=0x%x,App=%lu,Func=%s,Line=%d\n",
                (unsigned int)Status,CFE_ES_ResourceID_ToInteger(AppId),FunctionName,(int)LineNumber);

    }/* end if */

    return;

}/* end CFE_ES_LockSharedData */

/******************************************************************************
**  Function:  CFE_ES_UnlockSharedData()
**
**  Purpose:
**    ES internal function to Release the shared data mutex and handle error
**    conditions.
**
**  Arguments:
**    FunctionName   - the name of the function containing the code that generated the error.
**    LineNumber     - the file line number of the code that generated the error.
**
**  Return:
**    None
*/
void CFE_ES_UnlockSharedData(const char *FunctionName, int32 LineNumber)
{
    int32   Status;
    uint32  AppId;

    Status = OS_MutSemGive(CFE_ES_Global.SharedDataMutex);
    if (Status != OS_SUCCESS)
    {

        CFE_ES_GetAppIDInternal(&AppId);

        /*
         * NOTE: this is going to write into a buffer that itself
         * is _supposed_ to be protected by this same mutex.
         */
        CFE_ES_SysLogWrite_Unsync("ES SharedData Mutex Give Err Stat=0x%x,App=%lu,Func=%s,Line=%d\n",
                (unsigned int)Status,CFE_ES_ResourceID_ToInteger(AppId),FunctionName,(int)LineNumber);

    }/* end if */

    return;

}/* end CFE_ES_UnlockSharedData */

/******************************************************************************
**  Function:  CFE_ES_ProcessAsyncEvent()
**
**  Purpose:
**    Called by the PSP to notify CFE ES that an asynchronous event occurred.
*/
void CFE_ES_ProcessAsyncEvent(void)
{
    /* This just wakes up the background task to log/handle the event. */
    CFE_ES_BackgroundWakeup();
}

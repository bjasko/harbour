/*
 * $Id$
 */

/*
   Event logging system
*/

#include "hbapi.h"
#include "hblogdef.ch"

#if defined( HB_OS_WIN_32 )
#include "windows.h"

static HANDLE s_RegHandle;

#elif ( defined( HB_OS_UNIX ) || defined( HB_OS_LINUX ) ) && !defined( __WATCOMC__ )

#include <syslog.h>

#endif

HB_FUNC( HB_SYSLOGOPEN )
{
   #if defined( HB_OS_WIN_32 )
      #if (WINVER >= 0x0400)
      /* Ok, we compiled under NT, but we must not use this function
         when RUNNING on a win98. */
      if( hb_iswinnt() )
      {
         s_RegHandle = RegisterEventSource(NULL, (LPCTSTR) hb_parcx(1));
         hb_retl( TRUE );
      }
      else {
         hb_retl( FALSE );
      }
      #else
         s_RegHandle = NULL;
         hb_retl( FALSE );
      #endif
   #elif defined( HB_OS_UNIX ) && !defined( __WATCOMC__ )
      openlog( hb_parcx(1), LOG_NDELAY | LOG_NOWAIT | LOG_PID, LOG_USER );
      hb_retl( TRUE );
   #else
      hb_retl( FALSE );
   #endif
}

HB_FUNC( HB_SYSLOGCLOSE )
{
   #if defined( HB_OS_WIN_32 )
      #if (WINVER >= 0x0400)
      if( hb_iswinnt() )
      {
         DeregisterEventSource( s_RegHandle);
         hb_retl( TRUE );
      }
      else
      {
         hb_retl( FALSE );
      }
      #else
         hb_retl( FALSE );
      #endif
   #elif defined( HB_OS_UNIX ) && !defined( __WATCOMC__ )
      closelog();
      hb_retl( TRUE );
   #else
      hb_retl( FALSE );
   #endif
}

HB_FUNC( HB_SYSLOGMESSAGE )
{
   #if defined( HB_OS_WIN_32 )
      #if (WINVER >= 0x0400)
      WORD logval;
      if( hb_iswinnt() )
      {
         LPTSTR lpMsg = HB_TCHAR_CONVTO( hb_parcx( 1 ) );
         switch( hb_parni( 2 ) )
         {
            case HB_LOG_CRITICAL: logval = EVENTLOG_ERROR_TYPE; break;
            case HB_LOG_ERROR: logval = EVENTLOG_ERROR_TYPE; break;
            case HB_LOG_WARN: logval = EVENTLOG_WARNING_TYPE; break;
            case HB_LOG_INFO: logval = EVENTLOG_INFORMATION_TYPE; break;
            default:
               logval = EVENTLOG_AUDIT_SUCCESS;
         }
         hb_retl( ReportEvent(s_RegHandle,         /* event log handle */
                              logval,              /* event type */
                              0,                   /* category zero */
                              (DWORD) hb_parnl(3), /* event identifier */
                              NULL,                /* no user security identifier */
                              1,                   /* one substitution string */
                              0,                   /* no data */
                              (LPCTSTR *) &lpMsg,  /* pointer to string array */
                              NULL                 /* pointer to data */
                             ) ? TRUE : FALSE );
         HB_TCHAR_FREE( lpMsg );
      }
      else
      {
         hb_retl( FALSE );
      }

      #else
         hb_retl( FALSE );
      #endif
   #elif defined( HB_OS_UNIX ) && !defined( __WATCOMC__ )
      int logval;

      switch( hb_parni(2) )
      {
         case HB_LOG_CRITICAL: logval = LOG_CRIT; break;
         case HB_LOG_ERROR: logval = LOG_ERR; break;
         case HB_LOG_WARN: logval = LOG_WARNING; break;
         case HB_LOG_INFO: logval = LOG_INFO; break;
         default:
            logval = LOG_DEBUG;
      }

      syslog( logval, "[%lX]: %s", hb_parnl(3), hb_parcx(1) );
      hb_retl( TRUE );
   #else
      hb_retl( FALSE );
   #endif
}

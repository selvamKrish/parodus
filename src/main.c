/**
 * Copyright 2016 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
 
#include <string.h>
#include "stdlib.h"
#include "config.h"
#include "conn_interface.h"
#include "parodus_log.h"
#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#else
#include "signal.h"
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
#ifndef INCLUDE_BREAKPAD
static void sig_handler(int sig);
#endif

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char **argv)
{
#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#else
    signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGBUS, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGFPE, sig_handler);
	signal(SIGILL, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGALRM, sig_handler);
#endif	
    ParodusCfg *cfg;

    /* TODO not ideal, but it fixes a more major problem for now. */
    cfg = get_parodus_cfg();
    memset(cfg,0,sizeof(ParodusCfg));
    
    ParodusInfo("********** Starting component: Parodus **********\n "); 
    setDefaultValuesToCfg(cfg);
    if (0 != parseCommandLine(argc,argv,cfg)) {
		abort();
	}
    getAuthToken(cfg);
     
    createSocketConnection( NULL);
    
    return 0;
}

const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.PARODUS";
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
#ifndef INCLUDE_BREAKPAD
static void sig_handler(int sig)
{
    ParodusInfo("Signal %d (%s) received!\n", sig, strerror(sig));

    switch (sig) {
        case SIGUSR1:
        case SIGCHLD:
        case SIGPIPE:
        case SIGALRM:
            signal(sig, sig_handler);/* reset it to this function */
        case SIGUSR2:
        break;

        default:
            clean_up_parodus_cfg(get_parodus_cfg());
            exit(0);
        break;
    }
}
#endif

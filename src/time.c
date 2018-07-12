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

#include "time.h"
#include "parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void getCurrentTime(struct timespec *timer)
{
	if(timer != NULL)
    {
        clock_gettime(CLOCK_REALTIME, timer);
    }
}

uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer)
{
    uint64_t systime = 0;
    if(timer != NULL)
    {
    clock_gettime(CLOCK_REALTIME, timer);       
    ParodusPrint("timer->tv_sec : %lu\n",timer->tv_sec);
    ParodusPrint("timer->tv_nsec : %lu\n",timer->tv_nsec);
    systime = (uint64_t)timer->tv_sec * 1000000L + timer->tv_nsec/ 1000;
    }
    return systime;	
}

long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
	long msec = -1;
    if(starttime != NULL && finishtime != NULL)
    {
    msec=(finishtime->tv_sec-starttime->tv_sec)*1000;
	msec+=(finishtime->tv_nsec-starttime->tv_nsec)/1000000;
    }
	return msec;
}


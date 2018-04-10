/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
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
/**
 * @file nopoll_handlers.c
 *
 * @description This describes nopoll handler functions.
 *
 */

#include "ParodusInternal.h"
#include "nopoll_handlers.h"
#include "connection.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

pthread_mutex_t g_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cond=PTHREAD_COND_INITIALIZER;
ParodusMsg *ParodusMsgQ = NULL;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief listenerOnMessage_queue function to add messages to the queue
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */
void listenerOnMessage_queue(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data)
{
    UNUSED(ctx);
    UNUSED(conn);
    UNUSED(user_data);

    ParodusMsg *message;
    message = (ParodusMsg *)malloc(sizeof(ParodusMsg));

    if(message)
    {
        message->msg = msg;
        message->payload = (void *)nopoll_msg_get_payload (msg);
        message->len = nopoll_msg_get_payload_size (msg);
        message->next = NULL;

        nopoll_msg_ref(msg);

        pthread_mutex_lock (&g_mutex);		
        ParodusPrint("mutex lock in producer thread\n");

        if(ParodusMsgQ == NULL)
        {
            ParodusMsgQ = message;
            ParodusPrint("Producer added message\n");
            pthread_cond_signal(&g_cond);
            pthread_mutex_unlock (&g_mutex);
            ParodusPrint("mutex unlock in producer thread\n");
        }
        else
        {
            ParodusMsg *temp = ParodusMsgQ;
            while(temp->next)
            {
                temp = temp->next;
            }
            temp->next = message;
            pthread_mutex_unlock (&g_mutex);
        }
    }
    else
    {
        //Memory allocation failed
        ParodusError("Memory allocation is failed\n");
    }
    ParodusPrint("*****Returned from listenerOnMessage_queue*****\n");
}

/**
 * @brief listenerOnPingMessage function to create WebSocket listener to receive heartbeat ping messages
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn Websocket connection object
 * @param[in] msg The ping message received from the server
 * @param[out] user_data data which is to be sent
 */
void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data)
{
    UNUSED(ctx);
    UNUSED(user_data);

    noPollPtr payload = NULL;
    payload = (noPollPtr ) nopoll_msg_get_payload(msg);

    if ((payload!=NULL)) 
    {
        ParodusInfo("Ping received with payload %s, opcode %d\n",(char *)payload, nopoll_msg_opcode(msg));
        if (nopoll_msg_opcode(msg) == NOPOLL_PING_FRAME) 
        {
            nopoll_conn_send_frame (conn, nopoll_true, nopoll_true, NOPOLL_PONG_FRAME, strlen(payload), payload, 0);
            heartBeatTimer = 0;
            ParodusPrint("Sent Pong frame and reset HeartBeat Timer\n");
        }
    }
}

void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
    UNUSED(ctx);
    UNUSED(conn);
    char * closeReason = NULL;

    ParodusPrint("listenerOnCloseMessage(): mutex lock in producer thread\n");

    closeReason = nopoll_conn_get_close_reason(conn) ;
     ParodusInfo("close reason received, status=%d, message=%s\n",nopoll_conn_get_close_status (conn),closeReason);

    if( closeReason && !get_global_reconnect_status())
    {
		ParodusInfo("Reconnect detected, setting Reconnect reason %s\n",closeReason);
        set_global_reconnect_reason(closeReason);
        set_global_reconnect_status(true);
    }
    else if(nopoll_conn_get_close_status (conn) == 1006 )
    {
    	ParodusInfo("Reconnect detected, setting Reconnect reason as Unknown\n");
        set_global_reconnect_reason("Unknown");
    }

    pthread_mutex_lock (&close_mut);
    close_retry = true;
    pthread_mutex_unlock (&close_mut);
    ParodusPrint("listenerOnCloseMessage(): mutex unlock in producer thread\n");
}


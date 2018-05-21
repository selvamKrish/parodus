#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <wrp-c.h>
#include "crud_tasks.h"
#include "crud_internal.h"
#include "upstream.h"
#include "subscription.h"

/* To get service_name from CRUD destination field. 
	Ex: To get service_name "printer" from destination "mac:111111111111/parodus/subscribe/printer" */
char *get_service_name_from_destination(char *destination)
{
    char *token, *temStr;
    char * service_name = NULL;
    if(destination != NULL)
    {
        if(NULL != strtok(destination , "/"))
        {
            temStr = strtok(NULL , "");
            if(temStr != NULL)
            {
                token = strtok(temStr , "/");
                while(token != NULL)
                {
                    token = strtok(NULL, "/");
                    if(0 == strcmp(token, "subscribe"))
                    {
                        service_name = strtok(NULL, "/");
                        break;
                    }
                }
            }
        }
    }
    return service_name;
}

int processCrudRequest( wrp_msg_t *reqMsg, wrp_msg_t **responseMsg)
{
    wrp_msg_t *resp_msg = NULL;
    char *str= NULL;
    int ret = -1;
    char *destVal = NULL;
    char *destination = NULL;
    
    resp_msg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(resp_msg, 0, sizeof(wrp_msg_t));
    
    resp_msg->msg_type = reqMsg->msg_type;
    resp_msg->u.crud.transaction_uuid = strdup(reqMsg->u.crud.transaction_uuid);
    resp_msg->u.crud.source = strdup(reqMsg->u.crud.dest);
    resp_msg->u.crud.dest = strdup(reqMsg->u.crud.source);

    switch( reqMsg->msg_type ) 
    {
    
		case WRP_MSG_TYPE__CREATE:
		ParodusInfo( "CREATE request\n" );

		if(reqMsg->u.crud.dest !=NULL)
		{
			destVal = strdup(reqMsg->u.crud.dest);
			strtok(destVal , "/");
			destination = strtok(NULL , "");
			ParodusInfo("destination %s\n",destination);
			free(destVal);

			if(destination != NULL)
			{
				if ( strcmp(destination,"parodus/subscribe")== 0) 
				{
					//TODO handle error case
					HandleSubscriberEvent(reqMsg,resp_msg);
				}
				else
				{
					ret = createObject( reqMsg, &resp_msg );

					if(ret == 0)
					{
						cJSON *payloadObj = cJSON_Parse( (resp_msg)->u.crud.payload );
						str = cJSON_PrintUnformatted(payloadObj);
						ParodusInfo("Payload Response: %s\n", str);

						resp_msg ->u.crud.payload = (void *)str;
						if(str !=NULL)
						{
							resp_msg ->u.crud.payload_size = strlen(str);
						}
					}
					else
					{
						ParodusError("Failed to create object in config JSON\n");

						//WRP payload is NULL for failure cases
						resp_msg ->u.crud.payload = NULL;
						resp_msg ->u.crud.payload_size = 0;
					}
				}
			}
		}
	
		*responseMsg = resp_msg;
	    break;
	    
	case WRP_MSG_TYPE__RETREIVE:
	    ParodusInfo( "RETREIVE request\n" );
        ret = retrieveObject( reqMsg, &resp_msg );
        if(ret == 0)
        {
            cJSON *payloadObj = cJSON_Parse( (resp_msg)->u.crud.payload );
            str = cJSON_PrintUnformatted(payloadObj);
            ParodusInfo("Payload Response: %s\n", str);

            resp_msg ->u.crud.payload = (void *)str;
            if((resp_msg)->u.crud.payload !=NULL)
            {
                resp_msg ->u.crud.payload_size = strlen((resp_msg)->u.crud.payload);
            }
        }
        else
        {
            ParodusError("Failed to retrieve object \n");

            //WRP payload is NULL for failure cases
            resp_msg ->u.crud.payload = NULL;
            resp_msg ->u.crud.payload_size = 0;
        }
	    *responseMsg = resp_msg;
	    break;
	    
	case WRP_MSG_TYPE__UPDATE:
	    ParodusInfo( "UPDATE request\n" );
            ret = updateObject( );
	    
	    //WRP payload is NULL for update requests
	    resp_msg ->u.crud.payload = NULL;
	    resp_msg ->u.crud.payload_size = 0;
	    *responseMsg = resp_msg;
	    break;
	    
	case WRP_MSG_TYPE__DELETE:
	    ParodusInfo( "DELETE request\n" );
	    
	    if(reqMsg->u.crud.dest !=NULL && strstr(reqMsg->u.crud.dest, "subscribe") != NULL)
		{
			destVal = strdup(reqMsg->u.crud.dest);
		    service_name = get_service_name_from_destination(destVal);
            if(service_name != NULL)
            {
                ParodusPrint("service_name %s\n",service_name);
                if(true == delete_client_subscriptions(service_name))
                {
                    ParodusInfo("%s service deleted successfully\n",service_name);
                    resp_msg ->u.crud.status = 200;
                }
                else
                {
                    ParodusInfo("Failed to delete %s service\n",service_name);
                    resp_msg ->u.crud.status = 400;
                }
            }
            free(destVal);
            destVal = NULL;
        }
        else
        {
	        ret = deleteObject(reqMsg, &resp_msg );
	    }
	    //WRP payload is NULL for delete requests
        resp_msg ->u.crud.payload = NULL;
        resp_msg ->u.crud.payload_size = 0;
	    *responseMsg = resp_msg;
	      
	    break;
	    
	default:
	    ParodusInfo( "Unknown msgType for CRUD request\n" );
	    break;
     }
        
    return  0;
}
/*
*	To parse subscribe payload and add it to list
*	Send the same payload as response back to the Client
*/
int HandleSubscriberEvent(wrp_msg_t *reqMsg,wrp_msg_t *eventResp_msg)
{
	_sub_list_t *newSubscriber;

	newSubscriber = (_sub_list_t *)malloc(sizeof(_sub_list_t));
	if(newSubscriber)
	{
		memset(newSubscriber,0,sizeof(_sub_list_t));
		cJSON *cRequest = cJSON_Parse(reqMsg->u.crud.payload);
		char *serviceName = cJSON_GetArrayItem( cRequest, 0 )->string;
		ParodusInfo("serviceName is %s\n",serviceName);
		if(serviceName !=NULL)
		{
			newSubscriber->service_name = strdup(serviceName);
		}
		char *regex = cJSON_GetObjectItem( cRequest, serviceName )->valuestring;
		ParodusInfo("regex is %s\n",regex);
		if(regex !=NULL)
		{
			newSubscriber->regex = strdup(regex);
		}
		eventResp_msg->u.crud.status =  1;
		eventResp_msg->u.crud.payload =  reqMsg->u.crud.payload;
		eventResp_msg->u.crud.payload_size = reqMsg->u.crud.payload_size;

		if( true == add_Client_Subscription(newSubscriber->service_name, newSubscriber->regex))
		{
		    ParodusInfo("New subscriber is added to list\n");
		}
		free(newSubscriber->service_name);
		free(newSubscriber->regex);
		free(newSubscriber);
	}
	//TODO return proper value
	return 1;
}

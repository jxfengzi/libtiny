/**
 * Copyright (C) 2013-2015
 *
 * @author jxfengzi@gmail.com
 * @date   2013-11-19
 *
 * @file   SocketChannel.c
 *
 * @remark
 *      set tabstop=4
 *      set shiftwidth=4
 *      set expandtab
 */

#include <tiny_malloc.h>
#include <tiny_log.h>
#include <tiny_socket.h>
#include <tiny_snprintf.h>

#include "SocketChannel.h"

#define TAG     "SocketChannel"

TINY_LOR
static void _OnHandlerRemoved(void * data, void *ctx)
{
    ChannelHandler *handler = (ChannelHandler *)data;
    handler->onRemove(handler);
}

TINY_LOR
static uint16_t _socket_get_port(int fd)
{
    uint16_t port = 0;
    struct sockaddr_in sin;
    socklen_t len = (socklen_t) sizeof(sin);

    if (getsockname(fd, (struct sockaddr *)&sin, &len) == 0)
    {
        port = ntohs(sin.sin_port);
    }

    return port;
}

TINY_LOR
TinyRet SocketChannel_Dispose(Channel *thiz)
{
    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);

    LOG_D(TAG, "SocketChannel_Dispose: %s", thiz->id);
    TinyList_Dispose(&thiz->handlers);

    return TINY_RET_OK;
}

TINY_LOR
void SocketChannel_Delete(Channel *thiz)
{
    SocketChannel_Dispose(thiz);
    tiny_free(thiz);
}

TINY_LOR
void SocketChannel_OnRegister(Channel *thiz, Selector *selector, ChannelTimer *timer)
{
    RETURN_IF_FAIL(thiz);
    RETURN_IF_FAIL(selector);

    if (Channel_IsActive(thiz))
    {
        Selector_Register(selector, thiz->fd, SELECTOR_OP_READ);

        if (thiz->_getTimeout != NULL)
        {
            if (RET_SUCCEEDED(thiz->_getTimeout(thiz, timer, NULL)))
            {
                timer->fd = thiz->fd;
            }
        }
    }
}

TINY_LOR
void SocketChannel_OnActive(Channel *thiz)
{
    RETURN_IF_FAIL(thiz);

    LOG_I(TAG, "SocketChannel_OnActive: %s, handlers: %d", thiz->id, thiz->handlers.size);

    for (uint32_t i = 0; i < thiz->handlers.size; ++i)
    {
        ChannelHandler *handler = (ChannelHandler *) TinyList_GetAt(&thiz->handlers, i);

        LOG_I(TAG, "ChannelHandler: %s", handler->name);

        if (handler->channelActive != NULL)
        {
            handler->channelActive(handler, thiz);
        }
    }
}

TINY_LOR
void SocketChannel_OnInactive(Channel *thiz)
{
    RETURN_IF_FAIL(thiz);

    LOG_D(TAG, "SocketChannel_OnInactive: %s", thiz->id);

    for (uint32_t i = 0; i < thiz->handlers.size; ++i)
    {
        ChannelHandler *handler = (ChannelHandler *) TinyList_GetAt(&thiz->handlers, i);
        if (handler->channelInactive != NULL)
        {
            handler->channelInactive(handler, thiz);
        }
    }
}

TINY_LOR
void SocketChannel_OnEventTriggered(Channel *thiz, ChannelTimer *timer)
{
    RETURN_IF_FAIL(thiz);

    LOG_D(TAG, "SocketChannel_OnEventTriggered");

    if (thiz->fd == timer->fd)
    {
        for (uint32_t i = 0; i < thiz->handlers.size; ++i)
        {
            ChannelHandler *handler = (ChannelHandler *)TinyList_GetAt(&thiz->handlers, i);
            if (handler->channelEvent != NULL)
            {
                handler->channelEvent(handler, thiz, timer);
            }
        }
    }
}

TINY_LOR
TinyRet SocketChannel_GetTimeout(Channel *thiz, ChannelTimer *timer, void *ctx)
{
    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);
    RETURN_VAL_IF_FAIL(timer, TINY_RET_E_ARG_NULL);

    for (uint32_t i = 0; i < thiz->handlers.size; ++i)
    {
        ChannelHandler *handler = (ChannelHandler *) TinyList_GetAt(&thiz->handlers, i);
        if (handler->getTimeout != NULL)
        {
            return handler->getTimeout(thiz, timer, handler);
        }
    }

    return TINY_RET_E_NOT_IMPLEMENTED;
}

TINY_LOR
TinyRet SocketChannel_OnReadWrite(Channel *thiz, Selector *selector)
{
    TinyRet ret = TINY_RET_OK;

    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);
    RETURN_VAL_IF_FAIL(selector, TINY_RET_E_ARG_NULL);

    LOG_I(TAG, "SocketChannel_OnReadWrite");

    if (Selector_IsReadable(selector, thiz->fd))
    {
        char buf[CHANNEL_RECV_BUF_SIZE];
        int count = 0;

        memset(buf, 0, CHANNEL_RECV_BUF_SIZE);
        count = (int) tiny_recv(thiz->fd, buf, CHANNEL_RECV_BUF_SIZE, 0);
        if (count > 0)
        {
            SocketChannel_StartRead(thiz, DATA_RAW, buf, (uint32_t) ret);
        }
        else
        {
            if (tiny_socket_has_error(thiz->fd))
            {
                ret = TINY_RET_E_SOCKET_READ;
            }
        }
    }

    return ret;
}

TINY_LOR
TinyRet SocketChannel_Construct(Channel *thiz)
{
    TinyRet ret = TINY_RET_OK;

    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);

    LOG_D(TAG, "Channel_Construct");

    do
    {
        memset(thiz, 0, sizeof(Channel));

        ret = TinyList_Construct(&thiz->handlers);
        if (RET_FAILED(ret))
        {
            break;
        }

        TinyList_SetDeleteListener(&thiz->handlers, _OnHandlerRemoved, NULL);

        thiz->fd = -1;
        thiz->_onRegister = SocketChannel_OnRegister;
        thiz->_onReadWrite = SocketChannel_OnReadWrite;
        thiz->_onRemove = SocketChannel_Delete;
        thiz->_onActive = SocketChannel_OnActive;
        thiz->_onInactive = SocketChannel_OnInactive;
        thiz->_onEventTriggered = SocketChannel_OnEventTriggered;
        thiz->_getTimeout = SocketChannel_GetTimeout;
        thiz->_close = SocketChannel_Close;
    } while (0);

    return ret;
}

TINY_LOR
Channel * SocketChannel_New()
{
    Channel *thiz = NULL;

    do
    {
        thiz = (Channel *)tiny_malloc(sizeof(Channel));
        if (thiz == NULL)
        {
            break;
        }

        if (RET_FAILED(SocketChannel_Construct(thiz)))
        {
            SocketChannel_Delete(thiz);
            thiz = NULL;
            break;
        }
    } while (0);

    return thiz;
}

TINY_LOR
void SocketChannel_SetRemoteInfo(Channel *thiz, const char *ip, uint16_t port)
{
    RETURN_IF_FAIL(thiz);
    RETURN_IF_FAIL(ip);

    tiny_snprintf(thiz->id, CHANNEL_ID_LEN, "%d::%s::%d", thiz->fd, ip, port);

    strncpy(thiz->remote.socket.ip, ip, TINY_IP_LEN);
    thiz->remote.socket.port = port;
}

TINY_LOR
void SocketChannel_Initialize(Channel *thiz, ChannelInitializer initializer, void *ctx)
{
    RETURN_IF_FAIL(thiz);
    RETURN_IF_FAIL(initializer);

    initializer(thiz, ctx);
    thiz->_onActive(thiz);
}

TINY_LOR
TinyRet SocketChannel_Open(Channel *thiz, ChannelType type)
{
    TinyRet ret = TINY_RET_OK;

	RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);

    do
    {
        int socket_type = 0;
        int socket_protocol = 0;

        if (type == TYPE_UDP)
        {
            LOG_D(TAG, "SocketChannel_Open: UDP");
            socket_type = SOCK_DGRAM;
//            socket_protocol = IPPROTO_UDP;
            socket_protocol = 0;
        }
        else if (type == TYPE_TCP_SERVER || type == TYPE_TCP_CONNECTION)
        {
            LOG_D(TAG, "SocketChannel_Open: TCP");
            socket_type = SOCK_STREAM;
//            socket_protocol = IPPROTO_TCP;
            socket_protocol = 0;
        }
        else
        {
            LOG_E(TAG, "ChannelType invalid");
            ret = TINY_RET_E_ARG_INVALID;
            break;
        }

        thiz->fd = tiny_socket(AF_INET, socket_type, socket_protocol);
        if (thiz->fd < 0)
        {
            LOG_E(TAG, "socket failed");
            ret = TINY_RET_E_SOCKET_FD;
            break;
        }
    } while (0);

    return ret;
}

TINY_LOR
void SocketChannel_Close(Channel *thiz)
{
    LOG_D(TAG, "SocketChannel_Close: %s", thiz->id);

    Channel_Close(thiz);
}

TINY_LOR
TinyRet SocketChannel_Bind(Channel *thiz, uint16_t port, bool reuse)
{
    TinyRet ret = TINY_RET_OK;

    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);

    LOG_I(TAG, "SocketChannel_Bind: %d reuse: %s", port, reuse ? "yes" : "no");

    do
    {
        int result = 0;
        struct sockaddr_in  self_addr;
        memset(&self_addr, 0, sizeof(self_addr));
        self_addr.sin_family = AF_INET;
        self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        self_addr.sin_port = htons(port);

        if (reuse)
        {
            result = tiny_socket_reuse_port(thiz->fd);
            if (result != 0)
            {
                LOG_E(TAG, "tiny_socket_reuse_port failed: %d", result);
                ret = TINY_RET_E_SOCKET_SETSOCKOPT;
                break;
            }

            result = tiny_socket_reuse_address(thiz->fd);
            if (result != 0)
            {
                LOG_E(TAG, "tiny_socket_reuse_address failed: %d", result);
                ret = TINY_RET_E_SOCKET_SETSOCKOPT;
                break;
            }
        }

        result = tiny_bind(thiz->fd, (struct sockaddr *)&self_addr, sizeof(self_addr));
        if (result < 0)
        {
            LOG_E(TAG, "tiny_bind failed: %s", strerror(errno));
            ret = TINY_RET_E_SOCKET_BIND;
            break;
        }

        thiz->local.socket.address = self_addr.sin_addr.s_addr;
        thiz->local.socket.port = (port > 0) ? port : tiny_socket_get_port(thiz->fd);;
        tiny_snprintf(thiz->id, CHANNEL_ID_LEN, "%d::127.0.0.1::%d", thiz->fd, thiz->local.socket.port);

        LOG_I(TAG, "SocketChannel_Bind OK. channel id: %s", thiz->id);
    } while (0);

    return ret;
}

TINY_LOR
TinyRet SocketChannel_SetBlock(Channel *thiz, bool block)
{
    return (tiny_socket_set_block(thiz->fd, block) == 0) ? TINY_RET_OK : TINY_RET_E_INTERNAL;
}

TINY_LOR
TinyRet SocketChannel_Listen(Channel *thiz, int maxConnections)
{
    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);
    RETURN_VAL_IF_FAIL((maxConnections > 0), TINY_RET_E_ARG_NULL);

    if (tiny_listen(thiz->fd, maxConnections) < 0)
    {
        return TINY_RET_E_INTERNAL;
    }

    if (thiz->local.socket.port == 0)
    {
        thiz->local.socket.port = _socket_get_port(thiz->fd);
    }

    return TINY_RET_OK;
}

TINY_LOR
TinyRet SocketChannel_JoinGroup(Channel *thiz, const char *ip, const char *group)
{
    return (tiny_socket_join_group(thiz->fd, ip, group) == 0 ? TINY_RET_OK : TINY_RET_E_SOCKET_SETSOCKOPT);
}

TINY_LOR
TinyRet SocketChannel_LeaveGroup(Channel *thiz)
{
    return (tiny_socket_leave_group(thiz->fd) == 0 ? TINY_RET_OK : TINY_RET_E_SOCKET_SETSOCKOPT);
}

TINY_LOR
TinyRet SocketChannel_AddBefore(Channel *thiz, const char *name, ChannelHandler *handler)
{
    int position = -1;

	RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);
	RETURN_VAL_IF_FAIL(name, TINY_RET_E_ARG_NULL);
	RETURN_VAL_IF_FAIL(handler, TINY_RET_E_ARG_NULL);

    for (uint32_t i = 0; i < thiz->handlers.size; ++i)
    {
        ChannelHandler *h = (ChannelHandler *) TinyList_GetAt(&thiz->handlers, i);
        if (STR_EQUAL(h->name, name))
        {
            position = i;
            break;
        }
    }

    if (position < 0)
    {
        return TINY_RET_E_NOT_FOUND;
    }

    return TinyList_InsertBefore(&thiz->handlers, position, handler);
}

TINY_LOR
void SocketChannel_AddLast(Channel *thiz, ChannelHandler *handler)
{
	RETURN_IF_FAIL(thiz);
	RETURN_IF_FAIL(handler);

    TinyList_AddTail(&thiz->handlers, handler);
}

TINY_LOR
void SocketChannel_StartRead(Channel *thiz, ChannelDataType type, const void *data, uint32_t len)
{
    RETURN_IF_FAIL(thiz);
    RETURN_IF_FAIL(data);
    RETURN_IF_FAIL(len);

    thiz->currentReader = 0;
    SocketChannel_NextRead(thiz, type, data, len);
}

TINY_LOR
void SocketChannel_NextRead(Channel *thiz, ChannelDataType type, const void *data, uint32_t len)
{
    RETURN_IF_FAIL(thiz);
    RETURN_IF_FAIL(data);
    RETURN_IF_FAIL(len);

    LOG_I(TAG, "SocketChannel_NextRead");

    while (true)
    {
        ChannelHandler *handler = TinyList_GetAt(&thiz->handlers, thiz->currentReader);
        if (handler == NULL)
        {
            break;
        }

        LOG_D(TAG, "ChannelHandler: %s", handler->name);

        if (handler->inType != type)
        {
            LOG_E(TAG, "ChannelDataType not matched: %d != %d", type, handler->inType);
            break;
        }

        thiz->currentReader++;

        if (handler->channelRead == NULL)
        {
            LOG_D(TAG, "%s.channelRead not implemented", handler->name);
            continue;
        }

        if (handler->channelRead(handler, thiz, type, data, len))
        {
            break;
        }
    }
}

TINY_LOR
void SocketChannel_StartWrite(Channel *thiz, ChannelDataType type, const void *data, uint32_t len)
{
    RETURN_IF_FAIL(thiz);
    RETURN_IF_FAIL(data);
    RETURN_IF_FAIL(len);

    thiz->currentWriter = thiz->handlers.size - 1;
    SocketChannel_NextWrite(thiz, type, data, len);
}

TINY_LOR
void SocketChannel_NextWrite(Channel *thiz, ChannelDataType type, const void *data, uint32_t len)
{
    RETURN_IF_FAIL(thiz);
    RETURN_IF_FAIL(data);
    RETURN_IF_FAIL(len);

    while (true)
    {
        ChannelHandler *handler = TinyList_GetAt(&thiz->handlers, thiz->currentWriter);
        if (handler == NULL)
        {
            for (int i = 0; i < 10; ++i)
            {
                int sent = (int) tiny_send(thiz->fd, data, len, 0);
                if (sent == len)
                {
                    break;
                }

                // Found a bug on esp8266 !!!
                LOG_E(TAG, "tiny_send: %d, sent:%d", len, sent);

                if (sent == -1)
                {
                    if (tiny_socket_has_error(thiz->fd))
                    {
                        Channel_Close(thiz);
                        break;
                    }
                }
            }

            break;
        }

        //    if (handler->inType != type)
        //    {
        //        LOG_E(TAG, "ChannelDataType not matched: %d != %d", type, handler->inType);
        //        return ret;
        //    }

        thiz->currentWriter--;

        if (handler->channelWrite == NULL)
        {
            LOG_D(TAG, "%s.channelWrite NOT IMPLEMENTED", handler->name);
            continue;
        }

        LOG_D(TAG, "%s.channelWrite", handler->name);

        if (handler->channelWrite(handler, thiz, handler->inType, data, len))
        {
            break;
        }
    }
}
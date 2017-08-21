/**
 * Copyright (C) 2013-2015
 *
 * @author jxfengzi@gmail.com
 * @date   2013-7-9
 *
 * @file   tiny_socket_windows.c
 *
 * @remark
 *      set tabstop=4
 *      set shiftwidth=4
 *      set expandtab
 */

#include "tiny_socket_windows.h"

TINY_LOR
void tiny_socket_init()
{
    static bool initialized = false;
    WORD wVersionRequested;
    WSADATA wsaData;

    if (initialized)
    {
        return;
    }

    wVersionRequested = MAKEWORD(2, 0);
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
        LOG_E(TAG, "WSAStartup FAILED!");
    }
}

TINY_LOR
void tiny_socket_finalize()
{

}

TINY_LOR
int tiny_socket_set_block(int fd, bool block)
{
    unsigned long ul = block ? 0 : 1;
    return ioctlsocket(fd, FIONBIO, &ul);
}

TINY_LOR
int tiny_bind(int fd, const struct sockaddr *addr, socklen_t len)
{
    return (bind(fd, addr, len) == SOCKET_ERROR) ? -1 : 0;
}

TINY_LOR
int tiny_listen(int fd, int backlog)
{
    return (listen(fd, backlog) == SOCKET_ERROR) ? -1 : 0;
}

TINY_LOR
int tiny_recv(int fd, void *mem, size_t len, int flags)
{
//    if (n == SOCKET_ERROR)
//    {
//        DWORD e = GetLastError();
//            LOGI("GetLastError: %d", e);
//
//            if (e == WSAEWOULDBLOCK)
//            {
//                break;
//                air_usleep(100);
//                continue;
//            }
//            else
//            {
//                break;
//            }
//    }

    return (int) recv(fd, (char *)mem, len, flags);
}

TINY_LOR
int tiny_send(int fd, const void *data, size_t size, int flags)
{
    return send(fd, data, size, 0);
}

TINY_LOR
int tiny_socket_join_group(int fd, const char *ip, const char *group)
{
    //    struct sockaddr_in addr;
    int ret = NO_ERROR;

    do {
        struct ip_mreq ipMreqV4;
        char loop = 1;

        ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if (ret == SOCKET_ERROR)
        {
            DWORD e = GetLastError();
            LOG_D(TAG, "setsockopt: %d", e);
            break;
        }

        // Setup the v4 option values and ip_mreq structure
        memset(&ipMreqV4, 0, sizeof(struct ip_mreq));
        ipMreqV4.imr_multiaddr.s_addr = inet_addr(group);
        //ipMreqV4.imr_interface.s_addr = htonl(ip);
        ipMreqV4.imr_interface.s_addr = inet_addr(ip);

        ret = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ipMreqV4, sizeof(ipMreqV4));
        if (ret == SOCKET_ERROR)
        {
            // !!! bug 10065 = A socket operation was attempted to an unreachable host.
            DWORD e = GetLastError();
            LOG_E(TAG, "setsockopt: %d", e);
            break;
        }
    } while (0);

    return ret;
}

TINY_LOR
TinyRet tiny_async_connect(int fd, const char *ip, uint16_t port)
{
    TinyRet ret = TINY_RET_OK;

    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);

    do
    {
        int err = 0;
        struct sockaddr_in addr;

        memset(&addr, 0, sizeof (addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip);

        err = connect(thiz->fd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
        if (err == SOCKET_ERROR)
        {
            ret = (WSAGetLastError() == WSAEWOULDBLOCK) ? TINY_RET_PENDING : TINY_RET_E_SOCKET_CONNECTING;
        }
    } while(0);

    return ret;
}
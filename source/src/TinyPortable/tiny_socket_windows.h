/**
 * Copyright (C) 2013-2015
 *
 * @author jxfengzi@gmail.com
 * @date   2013-7-9
 *
 * @file   tiny_socket_windows.h
 *
 * @remark
 *      set tabstop=4
 *      set shiftwidth=4
 *      set expandtab
 */

#ifndef __TINY_SOCKET_WINDOWS_H__
#define __TINY_SOCKET_WINDOWS_H__

#include <tiny_typedef.h>
#include <tiny_inline.h>
#include <tiny_lor.h>
#include <tiny_ret.h>

#include <WinSock2.h>

TINY_BEGIN_DECLS


#define tiny_select                 select
#define tiny_socket_close           closesocket
#define tiny_socket                 socket
#define tiny_setsockopt             setsockopt
#define tiny_accept                 accept
#define tiny_recvfrom               recvfrom
#define tiny_sendto                 sendto
#define tiny_connect                connect

TINY_LOR
void tiny_socket_initialize();

TINY_LOR
void tiny_socket_finalize();

TINY_LOR
int tiny_socket_set_block(int fd, bool block);

TINY_LOR
int tiny_bind(int fd, const struct sockaddr *addr, socklen_t len);

TINY_LOR
int tiny_listen(int fd, int backlog);

TINY_LOR
int tiny_recv(int fd, void *mem, size_t len, int flags);

TINY_LOR
int tiny_send(int fd, const void *data, size_t size, int flags);

TINY_LOR
int tiny_socket_join_group(int fd, const char *ip, const char *group);

TINY_LOR
TinyRet tiny_async_connect(int fd, const char *ip, uint16_t port);


TINY_END_DECLS

#endif /* __TINY_SOCKET_WINDOWS_H__ */
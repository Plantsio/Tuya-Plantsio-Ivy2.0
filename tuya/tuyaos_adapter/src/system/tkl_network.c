/**
 * @file tkl_network.c
 * @brief 网络操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"

#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_network.h"

#define CANONNAME_MAX 128

typedef struct NETWORK_ERRNO_TRANS {
    int sys_err;
    int priv_err;
}NETWORK_ERRNO_TRANS_S;

#define UNW_TO_SYS_FD_SET(fds)  ((fd_set*)fds)

// 编译期结构体大小检查，如果此处编译错误，请加大 UNW_FD_MAX_COUNT 的值
char static_assert_impl_type[(sizeof(TUYA_FD_SET_T)>=sizeof(fd_set))?1:-1]; // 编译期校验

const NETWORK_ERRNO_TRANS_S unw_errno_trans[]= {
    {EINTR,UNW_EINTR},
    {EBADF,UNW_EBADF},
    {EAGAIN,UNW_EAGAIN},
    {EFAULT,UNW_EFAULT},
    {EBUSY,UNW_EBUSY},
    {EINVAL,UNW_EINVAL},
    {ENFILE,UNW_ENFILE},
    {EMFILE,UNW_EMFILE},
    {ENOSPC,UNW_ENOSPC},
    {EPIPE,UNW_EPIPE},
    {EWOULDBLOCK,UNW_EWOULDBLOCK},
    {ENOTSOCK,UNW_ENOTSOCK},
    {ENOPROTOOPT,UNW_ENOPROTOOPT},
    {EADDRINUSE,UNW_EADDRINUSE},
    {EADDRNOTAVAIL,UNW_EADDRNOTAVAIL},
    {ENETDOWN,UNW_ENETDOWN},
    {ENETUNREACH,UNW_ENETUNREACH},
    {ENETRESET,UNW_ENETRESET},
    {ECONNRESET,UNW_ECONNRESET},
    {ENOBUFS,UNW_ENOBUFS},
    {EISCONN,UNW_EISCONN},
    {ENOTCONN,UNW_ENOTCONN},
    {ETIMEDOUT,UNW_ETIMEDOUT},
    {ECONNREFUSED,UNW_ECONNREFUSED},
    {EHOSTDOWN,UNW_EHOSTDOWN},
    {EHOSTUNREACH,UNW_EHOSTUNREACH},
    {ENOMEM ,UNW_ENOMEM},
    {EMSGSIZE,UNW_EMSGSIZE}
};

/**
 * @brief 用于获取错误序号
 * 
 * @retval         errno
 */
TUYA_ERRNO tkl_net_get_errno(VOID_T)
{
    int i = 0;

    int sys_err = errno;

    for(i = 0; i < (int)sizeof(unw_errno_trans)/sizeof(unw_errno_trans[0]); i++) {
        if(unw_errno_trans[i].sys_err == sys_err) {
            return unw_errno_trans[i].priv_err;
        }
    }
    
    return -100 - sys_err;
}

/**
 * @brief : 用于ip字符串数据转换网络序ip地址(4B)
 * @param[in]      ip    ip字符串    "192.168.1.1"
 * @return  ip地址(4B)
 */
UINT32_T tkl_net_addr(CONST UCHAR_T *ip)
{
    if(ip == NULL) {
        return 0xFFFFFFFF;
    }

    return inet_addr((char *)ip);
}

/**
 * @brief : Ascii网络字符串地址转换为主机序(4B)地址 
 * @param[in]            ip_str
 * @return   主机序ip地址(4B)
 */
TUYA_IP_ADDR_T tkl_net_str2addr(CONST CHAR_T *ip)
{
    if(ip == NULL) {
        return 0xFFFFFFFF;
    }
    
    TUYA_IP_ADDR_T addr1 = inet_addr((char *)ip);
    TUYA_IP_ADDR_T addr2 = ntohl(addr1);
    // UNW_IP_ADDR_T addr3 = inet_network(ip_str);

    return addr2;
}

/**
 * @brief : set fds
 * @param[in]      fd
 * @param[inout]      fds
 * @return  0: success  <0: fail
 */
INT_T tkl_net_fd_set(CONST INT_T fd, TUYA_FD_SET_T* fds)
{
    if((fd < 0) || (fds == NULL)) {
        return -3000 + fd;
    }
	
    FD_SET(fd, UNW_TO_SYS_FD_SET(fds));
    return UNW_SUCCESS;
}

/**
 * @brief : clear fds
 * @param[in]      fd
 * @param[inout]      fds
 * @return  0: success  <0: fail
 */
INT_T tkl_net_fd_clear(CONST INT_T fd, TUYA_FD_SET_T* fds)
{
    if((fd < 0) || (fds == NULL)) {
        return -3000 + fd;
    }
	
	FD_CLR(fd, UNW_TO_SYS_FD_SET(fds));
	
    return UNW_SUCCESS;
}

/**
 * @brief : 判断fds是否被置位
 * @param[in]      fd
 * @param[in]      fds
 * @return  0-没有可读fd other-有可读fd
 */
INT_T tkl_net_fd_isset(CONST INT_T fd, TUYA_FD_SET_T* fds)
{
	if((fd < 0) || (fds == NULL)) {
        return -3000 + fd;
    }

    return FD_ISSET(fd, UNW_TO_SYS_FD_SET(fds));
}

/**
 * @brief : init fds
 * @param[inout]      fds
 * @return  0: success  <0: fail
 */
INT_T tkl_net_fd_zero(TUYA_FD_SET_T* fds)
{
    if(fds == NULL) {
        return 0xFFFFFFFF;
    }
	
	FD_ZERO(UNW_TO_SYS_FD_SET(fds));

    return UNW_SUCCESS;
}

/**
 * @brief : select
 * @param[in]         maxfd
 * @param[inout]      readfds
 * @param[inout]      writefds
 * @param[inout]      errorfds
 * @param[inout]      ms_timeout
 * @return  0: success  <0: fail
 */
INT_T tkl_net_select(CONST INT_T maxfd, TUYA_FD_SET_T *readfds, TUYA_FD_SET_T *writefds,\
                     TUYA_FD_SET_T *errorfds, CONST UINT_T ms_timeout)
{
    if(maxfd <= 0) {
        return -3000 + maxfd;
    }

    struct timeval *tmp = NULL;
    struct timeval timeout = {ms_timeout/1000, (ms_timeout%1000)*1000};
    if(0 != ms_timeout) {
        tmp = &timeout;
    }else {
        tmp = NULL;
    }

    return select(maxfd, UNW_TO_SYS_FD_SET(readfds), UNW_TO_SYS_FD_SET(writefds), UNW_TO_SYS_FD_SET(errorfds), tmp);
}

/**
 * @brief : close fd
 * @param[in]      fd
 * @return  0: success  <0: fail
 */
INT_T tkl_net_close(CONST INT_T fd)
{
	if(fd < 0) {
        return -3000 + fd;
    }

    return close(fd);
}

/**
 * @brief : shutdow fd
 * @param[in]      fd
 * @param[in]      how
 * @return  OPRT_OK: success  <0: fail
 */
INT_T tkl_net_shutdown(CONST INT_T fd, CONST INT_T how)
{
	if(fd < 0) {
        return -3000 + fd;
    }

    return shutdown(fd,how);
}

/**
 * @brief : creat fd
 * @param[in]      type
 * @return  >=0: socketfd  <0: fail
 */
INT_T tkl_net_socket_create(CONST TUYA_PROTOCOL_TYPE_E type)
{
    int fd = -1;

    if(PROTOCOL_TCP == type) {
        fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    } else if (PROTOCOL_RAW == type) {
        fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    } else {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    return fd;
}

/**
 * @brief : connect
 * @param[in]      fd
 * @param[in]      addr
 * @param[in]      port
 * @return  0: success  Other: fail
 */
INT_T tkl_net_connect(CONST INT_T fd, CONST TUYA_IP_ADDR_T addr, CONST UINT16_T port)
{
    struct sockaddr_in sock_addr;
    uint16_t tmp_port = port;
    TUYA_IP_ADDR_T tmp_addr = addr;
	
	if(fd < 0) {
        return -3000 + fd;
    }

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return connect(fd, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in));
}

/**
 * @brief : raw connect
 * @param[in]      fd
 * @param[in]      p_socket
 * @param[in]      len
 * @return  0: success  Other: fail
 */
INT_T tkl_net_connect_raw(CONST INT_T fd, void *p_socket_addr, CONST INT_T len)
{
	if(fd < 0) {
        return -3000 + fd;
    }
	
    return connect(fd, (struct sockaddr *)p_socket_addr, len);
}

/**
 * @brief : bind
 * @param[in]      fd
 * @param[in]      addr
 * @param[in]      port
 * @return  0: success  Other: fail
 */
INT_T tkl_net_bind(CONST INT_T fd, CONST TUYA_IP_ADDR_T addr, CONST UINT16_T port)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    unsigned short tmp_port = port;
    TUYA_IP_ADDR_T tmp_addr = addr;

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return bind(fd,(struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in));
}

/**
 * @brief : bind ip
 * @param[in]            fd
 * @param[inout]         addr
 * @param[inout]         port
 * @return  0: success  <0: fail
 */
INT_T tkl_net_socket_bind(CONST INT_T fd, CONST CHAR_T *ip)
{
    if(NULL == ip) {
        return -3000;
    }

    struct sockaddr_in addr_client   = {0};
    addr_client.sin_family   = AF_INET;
    addr_client.sin_addr.s_addr      = inet_addr(ip);
    addr_client.sin_port     = 0;    /// 0 表示由系统自动分配端口号

    if (0 != bind(fd,(struct sockaddr*)&addr_client,sizeof(addr_client))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/**
 * @brief : listen
 * @param[in]      fd
 * @param[in]      backlog
 * @return  0: success  < 0: fail
 */
INT_T tkl_net_listen(CONST INT_T fd, CONST INT_T backlog)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    return listen(fd,backlog);
}

/**
 * @brief : accept
 * @param[in]            fd
 * @param[inout]         addr
 * @param[inout]         port
 * @return  >=0: 新接收到的socketfd  others: fail
 */
INT_T tkl_net_accept(CONST INT_T fd, TUYA_IP_ADDR_T *addr, UINT16_T *port)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    struct sockaddr_in sock_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int cfd = accept(fd, (struct sockaddr *)&sock_addr,&len);
    if(cfd < 0) {
        return UNW_FAIL;
    }

    if(addr) {
        *addr = ntohl((sock_addr.sin_addr.s_addr));
    }

    if(port) {
        *port = ntohs((sock_addr.sin_port));
    }

    return cfd;
}

/**
 * @brief : send
 * @param[in]      fd
 * @param[in]      buf
 * @param[in]      nbytes
 * @return  nbytes has sended
 */
TUYA_ERRNO tkl_net_send(CONST INT_T fd, CONST void *buf, CONST UINT_T nbytes)
{
    if((fd < 0) || (buf == NULL) || (nbytes == 0)) {
        return -3000 + fd;
    }

    return send(fd,buf,nbytes,0);
}

/**
 * @brief : send to
 * @param[in]      fd
 * @param[in]      buf
 * @param[in]      nbytes
 * @param[in]      addr
 * @param[in]      port
 * @return  nbytes has sended
 */
INT_T tkl_net_send_to(CONST INT_T fd, CONST void *buf, CONST UINT_T nbytes,\
                      CONST TUYA_IP_ADDR_T addr,CONST UINT16_T port)
{
    unsigned short tmp_port = port;
    TUYA_IP_ADDR_T tmp_addr = addr;

    struct sockaddr_in sock_addr;
	
	if((fd < 0) || (buf == NULL) || (nbytes == 0)) {
        return -3000 + fd;
    }
	
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return sendto(fd,buf,nbytes,0,(struct sockaddr *)&sock_addr,sizeof(sock_addr));
}

/**
 * @brief : recv
 * @param[in]         fd
 * @param[inout]      buf
 * @param[in]         nbytes
 * @return  nbytes has received
 */
INT_T tkl_net_recv(CONST INT_T fd, void *buf, CONST UINT_T nbytes)
{
    fd_set readfds;
    fd_set exceptfds;

	if((fd < 0) || (buf == NULL) || (nbytes == 0)) {
        return -3000 + fd;
    }

    FD_ZERO(&readfds);
    FD_ZERO(&exceptfds);
    FD_SET(fd, &readfds);
    FD_SET(fd, &exceptfds);

    if(select(fd+1, &readfds, NULL, &exceptfds, NULL) < 0) {
        return -2;
    }

    if(FD_ISSET(fd, &exceptfds)) {
        return -4;
    }

    return recv(fd,buf,nbytes,0);
}

/**
 * @brief : Receive enough data to specify
 * @param[in]            fd
 * @param[inout]         buf
 * @param[in]            buf_size
 * @param[in]            nd_size
 * @return  nbytes has received
 */
INT_T tkl_net_recv_nd_size(CONST INT_T fd, void *buf, UINT_T buf_size, UINT_T nd_size)
{
    unsigned int rd_size = 0;
    int ret = 0;
	
	if((fd < 0) || (NULL == buf) || (buf_size == 0) || \
       (nd_size == 0) || (buf_size < nd_size)) {
        return -3000 + fd;
    }

    while(rd_size < nd_size) {
        ret = recv(fd,((uint8_t *)buf+rd_size),nd_size-rd_size,0);
        if(ret <= 0) {
            TUYA_ERRNO err = tkl_net_get_errno();
            if(UNW_EWOULDBLOCK == err || \
               UNW_EINTR == err || \
               UNW_EAGAIN == err) {
                tkl_system_sleep(10);
                continue;
            }

            break;
        }

        rd_size += ret;
    }

    if(rd_size < nd_size) {
        return -2;
    }

    return rd_size;
}

/**
 * @brief : recvfrom
 * @param[in]         fd
 * @param[inout]      buf
 * @param[in]         nbytes
 * @param[inout]         addr
 * @param[inout]         port
 * @return  nbytes has received
 */
INT_T tkl_net_recvfrom(CONST INT_T fd, void *buf, UINT_T nbytes,\
                       TUYA_IP_ADDR_T *addr, UINT16_T *port)
{
    int ret = 0;
	struct sockaddr_in sock_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
	
	if((fd < 0) || (buf == NULL) || (nbytes == 0)) {
        return -3000 + fd;
    }
	
    ret = recvfrom(fd,buf,nbytes,0,(struct sockaddr *)&sock_addr,&addr_len);
    if(ret <= 0) {
        return ret;
    }

    if(addr) {
        *addr = ntohl(sock_addr.sin_addr.s_addr);
    }

    if(port) {
        *port = ntohs(sock_addr.sin_port);
    }

    return ret;
}

/**
 * @brief : get fd block info
 * @param[in]      fd
 * @return  <0-失败   >0-非阻塞    0-阻塞
 */
INT_T tkl_net_get_nonblock(CONST INT_T fd)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    if((fcntl(fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK) {
        return 0;
    }

    /* hz-非阻塞直接由“(fcntl(fd, F_GETFL, 0) & O_NONBLOCK)”去判断，不用errno，原因是必须调用接口后errno才会生效*/
    if(errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
    }
	
	return 0;

}

/**
 * @brief : set block block or not
 * @param[in]      fd
 * @param[in]      block
 * @return  0: success  <0: fail
 */
INT_T tkl_net_set_block(CONST INT_T fd, CONST bool_t block)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if(block) {
        flags &= (~O_NONBLOCK);
    }else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(fd,F_SETFL,flags) < 0) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/**
 * @brief : set timeout
 * @param[in]         fd
 * @param[in]         ms_timeout
 * @param[in]         type
 * @return  0: success  <0: fail
 */
INT_T tkl_net_set_timeout(CONST INT_T fd, INT_T ms_timeout, TUYA_TRANS_TYPE_E type)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    struct timeval timeout = {ms_timeout/1000, (ms_timeout%1000)*1000};
    int optname = ((type == TRANS_RECV) ? SO_RCVTIMEO:SO_SNDTIMEO);

    if(0 != setsockopt(fd, SOL_SOCKET, optname, (char *)&timeout, sizeof(timeout))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/**
 * @brief : set buf size
 * @param[in]         fd
 * @param[in]         buf_size
 * @param[in]         type
 * @return  0: success  <0: fail
 */
INT_T tkl_net_set_bufsize(CONST INT_T fd, INT_T buf_size, TUYA_TRANS_TYPE_E type)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int size = buf_size;
    int optname = ((type == TRANS_RECV) ? SO_RCVBUF:SO_SNDBUF);

    if(0 != setsockopt(fd, SOL_SOCKET, optname, (char *)&size, sizeof(size))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/**
 * @brief : set reuse
 * @param[in]         fd
 * @return  0: success  <0: fail
 */
INT_T tkl_net_set_reuse(CONST INT_T fd)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int flag = 1;
    if(0 != setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/**
 * @brief : disable nagle
 * @param[in]         fd
 * @return  0: success  <0: fail
 */
INT_T tkl_net_disable_nagle(CONST INT_T fd)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int flag = 1;
    if(0 != setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/**
 * @brief : set broadcast
 * @param[in]         fd
 * @return  0: success  <0: fail
 */
INT_T tkl_net_set_boardcast(CONST INT_T fd)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int flag = 1;
    if(0 != setsockopt(fd,SOL_SOCKET,SO_BROADCAST,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}


/**
 * @brief : tcp保活设置 
 * @param[in]            fd-the socket fd
 * @param[in]            alive-open(1) or close(0) 
 * @param[in]            idle-how long to send a alive packet(in seconds) 
 * @param[in]            intr-time between send alive packets (in seconds)
 * @param[in]            cnt-keep alive packets fail times to close the connection
 * @return  0: success  <0: fail
 */
INT_T tkl_net_set_keepalive(CONST INT_T fd, BOOL_T alive,UINT_T idle, \
                            UINT_T intr, UINT_T cnt)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int ret = 0;
    int keepalive = alive;
    int keepidle = idle;
    int keepinterval = intr;
    int keepcount = cnt;

    ret |= setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount));
    if(0 != ret) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/**
* @brief Enable broadcast option of socket fd
*
* @param[in] fd: file descriptor
*
* @note This API is used to enable broadcast option of socket fd.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_set_broadcast(IN CONST INT_T fd)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    INT_T flag = 1;
    if(0 != setsockopt(fd,SOL_SOCKET,SO_BROADCAST,(CONST CHAR_T*)&flag,sizeof(int))) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : dns parse
 * @param[in]            domain
 * @param[inout]         addr
 * @return  0: success  <0: fail
 */
INT_T tkl_net_gethostbyname(CONST CHAR_T *domain, TUYA_IP_ADDR_T *addr)
{
    struct hostent* h = NULL;
	
	 if((domain == NULL) || (addr == NULL)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
	
    if ((h = gethostbyname(domain)) == NULL) {
        return UNW_FAIL;
    }

    *addr = ntohl(((struct in_addr *)(h->h_addr_list[0]))->s_addr);
    return UNW_SUCCESS;
}

/**
* @brief Get ip address by socket fd
*
* @param[in] fd: file descriptor
* @param[out] addr: ip address
*
* @note This API is used for getting ip address by socket fd.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_get_socket_ip(INT_T fd, TUYA_IP_ADDR_T *addr)
{
    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    socklen_t len = sizeof(sock_addr);
    INT_T ret = getsockname(fd, (struct sockaddr*)&sock_addr, &len);
    if (ret != 0) {
        return OPRT_COM_ERROR;
    }

    *addr = ntohl(sock_addr.sin_addr.s_addr);
    return OPRT_OK;
}

OPERATE_RET tkl_net_set_cloexec(CONST INT_T fd)
{
    return OPRT_OK;
}

/**
* @brief Change ip address to string
*
* @param[in] ipaddr: ip address
*
* @note This API is used to change ip address(in host byte order) to string(in IPv4 numbers-and-dots(xx.xx.xx.xx) notion).
*
* @return ip string
*/
CHAR_T* tkl_net_addr2str(TUYA_IP_ADDR_T ipaddr)
{
    unsigned int addr = lwip_htonl(ipaddr);
    return ip_ntoa((ip_addr_t *) &addr);
}

/**
* @brief Set socket options
*
* @param[in] fd: file descriptor
* @param[in] level: setting level
* @param[in] optname: the name of the option
* @param[in] optval: the value of option
* @param[in] optlen: the length of the option value
*
* @note This API is used for setting socket options.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_setsockopt(CONST INT_T fd, CONST TUYA_OPT_LEVEL level, CONST TUYA_OPT_NAME optname, CONST VOID_T *optval, CONST INT_T optlen)
{
    return setsockopt(fd, level, optname, optval , optlen);
}

/**
* @brief Get socket options
*
* @param[in] fd: file descriptor
* @param[in] level: getting level
* @param[in] optname: the name of the option
* @param[out] optval: the value of option
* @param[out] optlen: the length of the option value
*
* @note This API is used for getting socket options.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_getsockopt(CONST INT_T fd, CONST TUYA_OPT_LEVEL level, CONST TUYA_OPT_NAME optname, VOID_T *optval, INT_T *optlen)
{
    return getsockopt(fd, level, optname, optval , (socklen_t *)optlen);
}

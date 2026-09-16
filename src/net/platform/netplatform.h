#ifndef NETPLATFORM_H
#define NETPLATFORM_H


#ifdef _WIN32
typedef SOCKET netsock_t;
#define NETSOCK_INVALID SOCKET_INVALID
#else
typedef int netsock_t;
#define NETSOCK_INVALID ((netsock_t)-1)
#endif

#ifdef NET_NULL
#undef NET_NULL
#endif
#define NET_NULL ( (void*)0 )
typedef enum {
  NET_FAILURE = 0,
  NET_SUCCESS = 1,
} netresult_t;

void netplatform_init(void);
void netplatform_shutdown(void);

#endif

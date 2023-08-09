#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "net.h"
#include "config.h"
#include "uio.h"

static struct addrinfo net_server;
static bool net_server_set = false;
static bool net_connected = false;
static int net_socket = -1;

static bool net_parse_address(const char* srv, size_t *out_domain_len,
        char **out_port_start)
{
    char *port_iter;
    char *port_start = strchr(srv, ':');
    if (!port_start) {
        *out_port_start = NULL;
        *out_domain_len = strlen(srv);
        return true;
    }

    /* Only one ':' is allowed */
    if (strchr(port_start + 1, ':'))
        return false;

    *out_domain_len = port_start - srv;

    /*port = strtol(port_start + 1, &port_end, 10);
    if (port_end == port_start || *port_end != '\0' ||
            ((port == LONG_MIN || port == LONG_MAX) && errno))
        return false;*/

    /*if (port <= 0 || port > 65535)
        return false;*/

    port_iter = port_start + 1;
    if (*port_iter == '\0')
        return false;
    while (*port_iter) {
        if (!isdigit(*port_iter))
            return false;
        port_iter++;
    }
    *out_port_start = port_start + 1;

    return true;
}

static const void *net_get_sockaddr_addr(const struct sockaddr *sa)
{
    switch (sa->sa_family) {
    case AF_INET:
        return &((struct sockaddr_in*)sa)->sin_addr;
    case AF_INET6:
        return &((struct sockaddr_in6*)sa)->sin6_addr;
    default:
        uio_error("Sockaddr is not ipv4 or ipv6");
        exit(1);
    }
}

static bool net_lookup_server(const char *domain, const char *port,
        struct addrinfo *out_addr)
{
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        //.ai_family = AF_INET6,
        .ai_socktype = SOCK_DGRAM,
        .ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG,
        //.ai_flags = AI_NUMERICSERV,
    }, *res = NULL, *curr_res;
    
    int ret = getaddrinfo(domain, port, &hints, &res);
    if (ret != 0) {
        uio_error("Cannot get addrinfo from address: '%s:%s' (%s)",
                domain, port, gai_strerror(ret));
        return false;
    }
    
    curr_res = res;
    while (curr_res) {
        char ip_buffer[INET6_ADDRSTRLEN] = {0};
        if (inet_ntop(curr_res->ai_family,
                net_get_sockaddr_addr(curr_res->ai_addr),
                ip_buffer, sizeof(ip_buffer)))
            uio_debug("Lookup addrinfo entry: %s", ip_buffer);
        else
            uio_debug("Cannot convert binary ip to string: %s",
                    strerror(errno));

        /* For now, always choose the first one */
        break;
        
        curr_res = curr_res->ai_next;
    }
    
    if (!curr_res) {
        uio_error("Cannot select a usable address.");
        freeaddrinfo(res);
        return false;
    }

    *out_addr = *curr_res;
    out_addr->ai_addr = malloc(sizeof(struct sockaddr));
    *out_addr->ai_addr = *curr_res->ai_addr;
    out_addr->ai_next = NULL;
    out_addr->ai_canonname = NULL;
    freeaddrinfo(res);
    return true;
}

int net_socket_setup()
{
    struct sockaddr l_addr = {0};
    socklen_t l_addr_len;
    int sock;
    uint16_t *port;
    enum error err;

    if ((err = config_get("port", (void**)&port)) != NOERR) {
        uio_error("Cannot get UDP binding port from config (%s)",
                error_to_string(err));
        return -1;
    }

    sock = socket(net_server.ai_family, net_server.ai_socktype,
            net_server.ai_protocol);
    if (sock == -1) {
        uio_error("Cannot create new socket: %s", strerror(errno));
        return -1;
    }

    l_addr.sa_family = net_server.ai_family;
    if (net_server.ai_family == AF_INET) {
        struct sockaddr_in *tmp = (struct sockaddr_in*)&l_addr;
        l_addr_len = sizeof(struct sockaddr_in);

        tmp->sin_port = htons(*port);
        tmp->sin_addr.s_addr = INADDR_ANY;

    } else {
        struct sockaddr_in6 *tmp = (struct sockaddr_in6*)&l_addr;
        l_addr_len = sizeof(struct sockaddr_in6);

        tmp->sin6_port = htons(*port);
        tmp->sin6_addr = in6addr_any;

    }
    if (bind(sock, &l_addr, l_addr_len) != 0) {
        uio_error("Cannot bind UDP socket to local port: %s", strerror(errno));
        close(sock);
        return -1;
    }

    return sock;
}

static enum error net_connect(int sock, struct addrinfo *ai)
{
    if (net_connected)
        return ERR_NET_CONNECTED;

    if (connect(sock, ai->ai_addr, ai->ai_addrlen) != 0) {
        uio_error("Cannot connect to the server: %s\n", strerror(errno));
        return ERR_NET_CONNECT_FAIL;
    }
    net_connected = true;

    return NOERR;
}

enum error net_init()
{
    enum error err;
    const char **srv = NULL;
    char *port_start = NULL;
    size_t domain_len;
    int sock;

    err = config_get("api-server", (void**)&srv);
    if (err != NOERR) {
        uio_error("Cannot get the api servers address (%s).", error_to_string(err));
        return ERR_NET_APIADDR;
    }

    if (!net_parse_address(*srv, &domain_len, &port_start)) {
        uio_error("Cannot parse the api server address: '%s'.", *srv);
        return ERR_NET_APIADDR;
    }
    /* Port will be set to NULL, if its not in the address */
    if (port_start == NULL)
        port_start = "9000";

    char api_domain[domain_len + 1];
    memcpy(api_domain, *srv, domain_len);
    api_domain[domain_len] = '\0';

    if (!net_lookup_server(api_domain, port_start, &net_server)) {
        //uio_error("Cannot look up the api server address");
        return ERR_NET_APIADDR;
    }
    net_server_set = true;

    sock = net_socket_setup();
    if (sock == -1) {
        return ERR_NET_SOCKET;
    }

    err = net_connect(sock, &net_server);
    if (err != NOERR) {
        net_free();
        return err;
    }
    net_socket = sock;

    return NOERR;
}

void net_free()
{
    if (net_server_set) {
        free(net_server.ai_addr);
        memset(&net_server, 0, sizeof(net_server));
        net_server_set = false;
    }
    if (net_socket != -1) {
        if (net_connected)
            shutdown(net_socket, SHUT_RDWR);
        close(net_socket);
        net_socket = -1;
    }
    net_connected = false;
}

ssize_t net_send(const void *msg, size_t msg_len)
{
    ssize_t w_len = send(net_socket, msg, msg_len, 0);
    if (w_len == -1) {
        int en = errno;

        uio_error("{net} Send failed: %s", strerror(en));
        if (en == EINTR)
            return -2;
        return -1;
    }
    return w_len;
}

ssize_t net_read(void* out_data, size_t read_size)
{
    ssize_t read = recv(net_socket, out_data, read_size, 0);
    if (read == -1) {
        int en = errno;

        uio_error("{net} Read failed: %s", strerror(errno));
        return -en;
    }
    if (read == read_size)
        uio_warning("{net} Data may have been discarded!");
    return read;
}

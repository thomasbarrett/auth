#include <smtp.h>
#include <util.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct smtp_client {
    int fd;
    SSL *ssl;
} smtp_client_t;

static int create_tcp_connection(char *hostname, int port, int *res) {
    struct hostent *host = gethostbyname(hostname);
    if (host == NULL) return -1;
    
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(in_addr_t*)(*host->h_addr_list);

    int err = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (err != 0) {
        close(fd);
        return -1;
    }
    
    *res = fd;
    return 0;
}

smtp_client_t* smtp_client_create(char *host, int port, char *user, char *pass) {
    smtp_client_t *res = calloc(sizeof(smtp_client_t), 1);
    
    int err = create_tcp_connection(host, port, &res->fd);
    if (err != 0) {
        free(res);
        return NULL;
    }
    
    char buf[1024];
    size_t nread = read(res->fd, buf, sizeof(buf));
    buf[nread] = 0;

    const char *cmd = "EHLO caltech.edu\r\n";
    write(res->fd, cmd, strlen(cmd));
    printf("%s", buf);

    nread = read(res->fd, buf, sizeof(buf));
    buf[nread] = 0;

    cmd = "STARTTLS\r\n";
    write(res->fd, cmd, strlen(cmd));
    printf("%s", buf);

    nread = read(res->fd, buf, sizeof(buf));
    buf[nread] = 0;
 
    SSL_library_init();
    OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	const SSL_METHOD *method = TLS_client_method();
	SSL_CTX *ctx = SSL_CTX_new(method);
	if (ctx == NULL) {
		free(res);
        return NULL;
	}
    res->ssl = SSL_new(ctx); 
    SSL_set_fd(res->ssl, res->fd);
    if (SSL_connect(res->ssl) != 1) {
        ERR_print_errors_fp(stderr);
        close(res->fd);
        SSL_CTX_free(ctx);
        return NULL;
    }
  
    cmd = "EHLO caltech.edu\r\n";
    SSL_write(res->ssl, cmd, strlen(cmd));
    nread = SSL_read(res->ssl, buf, sizeof(buf));
    buf[nread] = 0;
    printf("%s", buf);

    cmd = "AUTH LOGIN\r\n";
    SSL_write(res->ssl, cmd, strlen(cmd));
    nread = SSL_read(res->ssl, buf, sizeof(buf));
    buf[nread] = 0;
    printf("%s", buf);

    char *user_encoded = base64_encode(user, strlen(user), true);
    SSL_write(res->ssl, user_encoded, strlen(user_encoded));
    SSL_write(res->ssl, "\r\n", 2);
    free(user_encoded);
    nread = SSL_read(res->ssl, buf, sizeof(buf));
    buf[nread] = 0;
    printf("%s", buf);

   char *pass_encoded = base64_encode(pass, strlen(pass), true);
    SSL_write(res->ssl, pass_encoded, strlen(pass_encoded));
    SSL_write(res->ssl, "\r\n", 2);
    free(pass_encoded);
    nread = SSL_read(res->ssl, buf, sizeof(buf));
    buf[nread] = 0;
    printf("%s", buf);

    return res; 
}

int smtp_client_send(smtp_client_t *client, char *sender, char *recipient, char *msg) {
    if (strlen(sender) > 64 || strlen(recipient) > 64) return -1;
    char cmd1[256], buf[256];
    sprintf(cmd1, "MAIL FROM: <%s>\r\n", sender);
    SSL_write(client->ssl, cmd1, strlen(cmd1));
    size_t nread = SSL_read(client->ssl, buf, sizeof(buf));
    buf[nread] = 0;
    printf("%s", buf);

    sprintf(cmd1, "RCPT TO: <%s>\r\n", recipient);
    SSL_write(client->ssl, cmd1, strlen(cmd1));
    nread = SSL_read(client->ssl, buf, sizeof(buf));
    buf[nread] = 0;
    printf("%s", buf);

    sprintf(cmd1, "DATA\r\n");
    SSL_write(client->ssl, cmd1, strlen(cmd1));
    nread = SSL_read(client->ssl, buf, sizeof(buf));
    buf[nread] = 0;
    printf("%s", buf);

    SSL_write(client->ssl, msg, strlen(msg));
    SSL_write(client->ssl, "\r\n.\r\n", 5);
    nread = SSL_read(client->ssl, buf, sizeof(buf));
    buf[nread] = 0;
    printf("%s", buf);

    return 0;
}

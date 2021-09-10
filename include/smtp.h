#ifndef SMTP_H
#define SMTP_H

/**
 * smtp_client_t is a simple ESMTP client that is able to connect and send emails
 * from the caltech mail server. 
 */
typedef struct smtp_client smtp_client_t;

/**
 * Create a smtp client that connects to a mail server with the given hostname
 * and port. Authenticate using ESMTP AUTH LOGIN extension using the given username
 * and password.
 */
smtp_client_t* smtp_client_create(char *host, int port, char *user, char *pass);

/**
 * smtp_client_send sends an email with the given message from the given sender
 * to the given recipient.
 */
int smtp_client_send(smtp_client_t *client, char *sender, char *recipient, char *msg);

#endif /* SMTP_H */

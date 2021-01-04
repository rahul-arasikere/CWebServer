/**
 * webserver.c -- A webserver written in C
 * 
 * Test with curl (if you don't have it, install it):
 * 
 *    curl -D - http://localhost:3490/
 *    curl -D - http://localhost:3490/d20
 *    curl -D - http://localhost:3490/date
 * 
 * You can also test the above URLs in your browser! They should work!
 * 
 * Posting Data:
 * 
 *    curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/save
 * 
 * (Posting data is harder to test from a browser.)
 */

#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/file.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include "net.h"
#include "file.h"
#include "mime.h"
#include "cache.h"

#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

cache_t *cache;
/**
 * Send an HTTP response
 *
 * header:       "HTTP/1.1 404 NOT FOUND" or "HTTP/1.1 200 OK", etc.
 * content_type: "text/plain", etc.
 * body:         the data to send.
 * 
 * Return the value from the send() function.
 */
int send_response(int fd, char *header, char *content_type, void *body, int content_length)
{
    const int max_response_size = 262144;
    char response[max_response_size];
    snprintf(response, max_response_size - 1, "%s\nContent-type: %s\n\n", header, content_type); //set header to response
    int rv = send(fd, response, strlen(response), 0);                                            //send header
    if (rv < 0)
    {
        perror("send");
        return -1;
    }
    int fv = send(fd, body, content_length, 0);
    if (fv < 0)
    {
        perror("send");
        return -1;
    }
    return rv + fv;
}

void resp_500(int fd)
{
    char filepath[4096];
    struct file_data *filedata;
    char *mime_type;

    // Fetch the 404.html file
    snprintf(filepath, sizeof filepath, "%s/500.html", SERVER_FILES);
    filedata = file_load(filepath);

    if (filedata == NULL)
    {
        send_response(
            fd,
            "HTTP/1.1 500 INTERNAL SERVER ERROR",
            "text/html",
            "Back to the drawing board\n",
            27);
    }
    else
    {
        mime_type = mime_type_get(filepath);
        send_response(fd, "HTTP/1.1 500 INTERNAL SERVER ERROR", mime_type, filedata->data, filedata->size);
    }

    file_free(filedata);
}

void acquire_data(int fd, char *request_path, char *arguments)
{
    char *arg = strtok(arguments, "&");
    char *key = strtok(arg, "=");
    char *value = strtok(NULL, "=");
    char filepath[4096];
    snprintf(filepath, sizeof filepath, "%s%s.csv", SERVER_ROOT, request_path);
    FILE *fp = fopen(filepath, "a");
    time_t seconds;
    time(&seconds);
    fprintf(fp, "%ld %s\n", seconds, value);
    send_response(fd, "HTTP/1.1 200 OK", "text/plain", "OK", 2);
    fclose(fp);
}

void execute_cgi_script(int fd, char *request_path, char *arguments)
{
    char filepath[4096];
    // Fetch the file from the server root.
    snprintf(filepath, sizeof filepath, "%s%s", SERVER_ROOT, request_path);
    int _server[2];
    if (pipe(_server) < 0)
    {
        resp_500(fd);
        return;
    }
    char *argv[] = {"/bin/sh", filepath, arguments, NULL};
    pid_t child = fork();
    if (child < 0)
    {
        resp_500(fd);
        return;
    }
    else if (child == 0)
    {
        // pipe output to pipe
        dup2(_server[0], STDIN_FILENO);
        dup2(_server[1], STDOUT_FILENO);
        dup2(_server[1], STDERR_FILENO);
        execvp(argv[0], argv);
        close(_server[0]);
        close(_server[1]);
    }
    else
    {
        close(_server[1]);
        if (waitpid(child, NULL, 0) < 0)
        {
            // child error
            resp_500(fd);
        }
        else
        {
            // read from pipe and send it to client
            char buf[262144];
            int len = read(_server[0], buf, 262144);
            send(fd, "HTTP/1.1 200 OK\n", 16, 0);
            send(fd, buf, len, 0);
        }
    }
}

void resp_501(int fd)
{
    char filepath[4096];
    struct file_data *filedata;
    char *mime_type;

    // Fetch the 404.html file
    snprintf(filepath, sizeof filepath, "%s/501.html", SERVER_FILES);
    filedata = file_load(filepath);

    if (filedata == NULL)
    {
        send_response(
            fd,
            "HTTP/1.1 501 NOT IMPLEMENTED",
            "text/html",
            "Listen. In order to maintain air-speed velocity, a swallow needs to beat its wings forty-three times every second, right?\n",
            123);
    }
    else
    {
        mime_type = mime_type_get(filepath);
        send_response(fd, "HTTP/1.1 501 NOT IMPLEMENTED", mime_type, filedata->data, filedata->size);
    }

    file_free(filedata);
}

/**
 * Send a 404 response
 */
void resp_404(int fd)
{
    char filepath[4096];
    struct file_data *filedata;
    char *mime_type;

    // Fetch the 404.html file
    snprintf(filepath, sizeof filepath, "%s/404.html", SERVER_FILES);
    filedata = file_load(filepath);

    if (filedata == NULL)
    {
        send_response(fd, "HTTP/1.1 404 NOT FOUND", "text/html", "These are not the droids you are looking for...", 48);
    }
    else
    {
        mime_type = mime_type_get(filepath);
        send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);
    }
    file_free(filedata);
}

/**
 * Read and return a file from disk or cache
 */
void get_file(int fd, cache_t *cache, char *request_path)
{
    char filepath[4096];
    struct file_data *filedata;
    char *mime_type;
    // Fetch the file from the server root.
    snprintf(filepath, sizeof filepath, "%s%s", SERVER_ROOT, request_path);
    cache_entry_t *ce = cache_get(cache, filepath);
    if (ce != NULL)
    {
        send_response(fd, "HTTP/1.1 200 OK", ce->content_type, ce->content, ce->content_length);
    }
    else
    {

        filedata = file_load(filepath);
        if (filedata == NULL)
        {
            // file not found
            // check if a directory
            DIR *dp = opendir(filepath);
            if (dp)
            {
                char *dirlist = (char *)malloc(262144);
                struct dirent *dir;
                int len = snprintf(dirlist, 262144, "<html>\n\t<body>\n\t\t\n\t\t<h1>Directory Listing</h1>\n\t\t\t<br />\n\t\t<ul>\n");
                while ((dir = readdir(dp)) != NULL)
                {
                    if (dir->d_type == DT_DIR)
                    {
                        // create directory listing
                        len += snprintf(dirlist + len, 262144, "\t\t\t<li><a href=\"./%s/\">%s</a></li>\n", dir->d_name, dir->d_name);
                    }
                    else
                    {
                        // create file listing
                        len += snprintf(dirlist + len, 262144, "\t\t\t<li><a href=\"./%s\">%s</a></li>\n", dir->d_name, dir->d_name);
                    }
                }
                len += snprintf(dirlist + len, 262144, "\t\t</ul>\n\t</body>\n</html>\n");
                send_response(fd, "HTTP/1.1 200 OK", "text/html", dirlist, len);
                cache_put(cache, filepath, "text/html", dirlist, len);
                closedir(dp);
                free(dirlist);
            }
            else
            {
                // not a directory return 404.
                resp_404(fd);
            }
        }
        else
        {
            mime_type = mime_type_get(filepath);
            send_response(fd, "HTTP/1.1 200 OK", mime_type, filedata->data, filedata->size);
            cache_put(cache, filepath, mime_type, filedata->data, filedata->size);
        }
        free(filedata);
    }
}

/**
 * Search for the end of the HTTP header
 * 
 * "Newlines" in HTTP can be \r\n (carriage return followed by newline) or \n
 * (newline) or \r (carriage return).
 */
char *find_start_of_body(char *header)
{
    char *line = strtok(header, "\n");
    while (line != NULL)
    {
        if (strcmp(line, "\n") == 0)
        {
            break;
        }
        line = strtok(NULL, "\n");
    }
    return line;
}

/**
 * Handle HTTP request and send response
 */
void handle_http_request(int fd)
{
    const int request_buffer_size = 65536; // 64K
    char request[request_buffer_size];

    // read request
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);

    if (bytes_recvd < 0)
    {
        perror("recv");
        return;
    }
    // read the first word, corresponds to request method GET, POST, DELETE, PUT, etc ...
    char *method = strtok(request, " ");
    char *request_path = strtok(NULL, " ");
    char *filepath = strtok(request_path, "?");
    char *arguments = strtok(NULL, "?");
    char *ext = strrchr(filepath, '.');
    if (strcmp(method, "GET") == 0)
    {
        // handle the get request
        if (ext != NULL)
        {
            if (strcmp(ext, ".cgi") == 0)
            {
                // cgi script, run and output to socket.
                execute_cgi_script(fd, filepath, arguments);
            }
            else
            {
                // return file
                get_file(fd, cache, filepath);
            }
        }
        else
        {
            if (strcmp(filepath, "/sensorUpdate") == 0)
            {
                acquire_data(fd, filepath, arguments);
            }
            else
            {
                // directory listing requested
                get_file(fd, cache, filepath);
            }
        }
    }
    else
    {
        // return 501 status not implemented...
        resp_501(fd);
    }
    close(fd);
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf(stderr, "webserver: expected port, usage: ./server PORT [5000 - 65536]\n");
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    if (port < 5000 || port > 65536)
    {
        fprintf(stderr, "(main): Port %d out of bounds!\n", port);
        exit(EXIT_FAILURE);
    }

    int newfd;                          // listen on sock_fd, new connection on newfd
    struct sockaddr_storage their_addr; // connector's address information
    char s[INET6_ADDRSTRLEN];

    // create a cache that holds last 10 pages requested.
    cache = cache_create(10, 0);

    // Get a listening socket
    int listenfd = get_listener_socket(port);

    if (listenfd < 0)
    {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        exit(EXIT_FAILURE);
    }

    printf("webserver: waiting for connections on port %s...\n", argv[1]);

    // This is the main loop that accepts incoming connections and
    // responds to the request. The main parent process
    // then goes back to waiting for new connections.

    listen(listenfd, 10);

    while (1)
    {
        socklen_t sin_size = sizeof their_addr;

        // Parent process will block on the accept() call until someone
        // makes a new connection:
        newfd = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
        if (newfd == -1)
        {
            perror("accept");
            continue;
        }

        // Print out a message that we got the connection
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);
        // newfd is a new socket descriptor for the new connection.
        // listenfd is still listening for new connections.
        if (fork() == 0)
        {                    //forks a child process, to handle the connection.
            close(listenfd); //close listenfd in child process.
            handle_http_request(newfd);
            close(newfd);
            exit(EXIT_SUCCESS);
        }
        else
        {
            close(newfd);
        }
    }

    // Unreachable code

    return EXIT_SUCCESS;
}

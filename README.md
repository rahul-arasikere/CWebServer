# CWebServer

A simple multi-process socket based web-server written in C.

## Usage

Run: `./webserv PORT [5000-65536]`

## About

I have implemented a very basic version of caching, there is no expiry time so files being regenerated is no deterministic.
You can replace the generic HTTP error messages by providing `'ERROR'.html` in the `./serverfiles` directory.

The hash-table implementation and linked-list implementation was taken from Lambda C git repo.

## Demo

I have included a version of my portfolio website for you to try. There is also the `my-histogram.cgi` file that will generate a dynamic image and return it.

To test NOT IMPLEMENTED ERROR (501), you can run `curl -X http://ip-address:port/`. It should return a monty python joke.

## Physical Computing

A very basic arduino control loop is included in the `arduino` directory. Modify the settings variables before compiling.

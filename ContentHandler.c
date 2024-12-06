#include <malloc.h>
#include <string.h>
#include "headers/ContentHandler.h"

WebSocketContent* create_content()
{
    WebSocketContent *content = malloc(sizeof(WebSocketContent));
    if (content == NULL)
    {
        printf("create_content failed at malloc content!\n");
        return NULL;
    }

    content->content_address = malloc(sizeof(char[MAX_CONTENT_COUNT]));
    if (content->content_address == NULL)
    {
        printf("create_content failed at malloc content_address!\n");
        return NULL;
    }

    content->content = malloc(sizeof(char[MAX_CONTENT_COUNT]));
    if(content->content == NULL)
    {
        printf("create_content failed at malloc content->content!\n");
        return NULL;
    }

    content->size = malloc(sizeof(int));
    if(content->size == NULL)
    {
        printf("create_content failed at malloc size!\n");
        return NULL;
    }
    return content;
}

char* get_content(WebSocketContent* socket, const char *address)
{
    for(int i = 0; i < *(socket->size); i++)
    {
        if (socket->content_address[i] == *address)
        {
            return socket->content[i];
        }
    }
    return 0;
}

int set_content(WebSocketContent* socket, const char *address, const char *content)
{
    for(int i = 0; i < *(socket->size); i++)
    {
        if (strcmp(socket->content_address[i], address) == 0)
        {
            strcpy(socket->content[i], content);
            return 204;
        }
    }

    if (*(socket->size) >= MAX_CONTENT_COUNT)
        return 500;

    socket->content_address[*(socket->size)] = (char*) malloc(strlen(address) + 1);
    if (socket->content_address[*(socket->size)] == NULL)
    {
        printf("Failed to allocate memory for content_address");
        return 500;
    }
    strcpy(socket->content_address[*(socket->size)], address);

    socket->content[*(socket->size)] = (char*) malloc(strlen(content) + 1);
    if (socket->content[*(socket->size)] == NULL)
    {
        printf("Failed to allocate memory for content");
        return 500;
    }
    strcpy(socket->content[*(socket->size)], content);
    *(socket->size) += 1;
    return 201;
}

int delete_content(WebSocketContent* socket, const char *address)
{
    int index = -1;
    for (int i = 0; i < *(socket->size); i++)
    {
        if (socket->content_address[i] == *address)
        {
            index = i;
            break;
        }
    }
    if (index == -1)
        return 404;

    // Shift all 1 to left
    for (int i = index; i < *(socket->size) - 1; i++)
    {
        socket->content_address[i] = socket->content_address[i + 1];
        socket->content[i] = socket->content[i + 1];
    }
    *(socket->size) -= 1;

    return 204;
}
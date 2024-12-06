#define MAX_CONTENT_COUNT 100
#define BUFFER_SIZE 1024

typedef struct websocketcontent
{
    char* *content_address;
    char* *content;
    int *size;

} WebSocketContent;

/// Creates a new websocketcontent context
/// \return the new websocketcontent
WebSocketContent* create_content();
/// Gets the content of an address
/// \param socket Websocket->content
/// \param address address
/// \return 0 if no content found
char* get_content(WebSocketContent* socket, const char *address);
/// Sets the content on an address or creates new
/// \param socket Websocket->content
/// \param address address
/// \param content content
/// \return 204 if overwritten, 201 if created, 500 if something failed
int set_content(WebSocketContent* socket, const char *address, const char *content);
/// Deletes the content at an address
/// \param socket Websocket->content
/// \param address address
/// \return 404 if not found, otherwise 204
int delete_content(WebSocketContent* socket, const char *address);


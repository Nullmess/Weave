/* Weave native network API. A connected, user-authorized companion carries
 * requests over the official Storage RPC; no Wi-Fi module is required.
 * Callbacks run on the Weave network worker, never on the calling FAP thread.
 * They may be called repeatedly for a single reply; inspect `more`.
 * `data` is borrowed and is only valid for the duration of the callback.
 */
#ifndef WEAVE_H
#define WEAVE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RECORD_WEAVE "weave"
#define WEAVE_API_VERSION 2
#define WEAVE_URL_MAX 192
#define WEAVE_RESPONSE_MAX 512  /* Per callback / inline request chunk, not total download */
#define WEAVE_STREAM_MAX (16UL * 1024UL * 1024UL)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Weave Weave;
typedef enum {
    WeaveTransportNone, WeaveTransportBle, WeaveTransportUsb
} WeaveTransport;
typedef enum {
    WeaveStatusOk, WeaveStatusOffline, WeaveStatusBusy, WeaveStatusInvalid,
    WeaveStatusTooLarge, WeaveStatusTimeout, WeaveStatusRemoteError,
    WeaveStatusDenied
} WeaveStatus;
typedef enum {
    WeaveSocketTcp, WeaveSocketUdp, WeaveSocketWebSocket
} WeaveSocketType;

typedef struct {
    uint32_t id;
    WeaveStatus status;
    uint16_t http_status; /* HTTP status, or the socket session ID on OPEN */
    const uint8_t* data;
    size_t size;
    uint32_t offset; /* byte offset in the response stream */
    uint32_t total;  /* number of bytes in the complete response */
    bool more;       /* true if more callbacks for this request will follow */
} WeaveResponse;
typedef void (*WeaveCallback)(const WeaveResponse* response, void* context);

WeaveTransport weave_get_transport(Weave* weave);
WeaveStatus weave_http_get(Weave* weave, const char* url, WeaveCallback cb, void* ctx);
WeaveStatus weave_http_request(Weave* weave, const char* method, const char* url,
    const uint8_t* body, size_t length, WeaveCallback cb, void* ctx);
/* Streams the upload directly from an SD file; never loads it all into RAM. */
WeaveStatus weave_http_upload_file(Weave* weave, const char* method,
    const char* url, const char* sd_path, WeaveCallback cb, void* ctx);
/* OPEN callbacks return a session ID in http_status. Only outbound sockets. */
WeaveStatus weave_socket_open(Weave* weave, WeaveSocketType type,
    const char* host_or_wss_url, uint16_t port, WeaveCallback cb, void* ctx);
WeaveStatus weave_socket_send(Weave* weave, WeaveSocketType type,
    uint16_t session, const uint8_t* body, size_t length, WeaveCallback cb, void* ctx);
WeaveStatus weave_socket_receive(Weave* weave, WeaveSocketType type,
    uint16_t session, WeaveCallback cb, void* ctx);
WeaveStatus weave_socket_close(Weave* weave, WeaveSocketType type,
    uint16_t session, WeaveCallback cb, void* ctx);

#ifdef __cplusplus
}
#endif
#endif

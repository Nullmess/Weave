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
#define WEAVE_API_VERSION 3
#define WEAVE_URL_MAX 384
#define WEAVE_RESPONSE_MAX 512  /* Per callback / inline request chunk, not total download */
#define WEAVE_STREAM_MAX (16UL * 1024UL * 1024UL)
#define WEAVE_QUEUE_MAX 4
#define WEAVE_HTTP_HEADERS_MAX 8

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
typedef struct {
    const char* name; /* ASCII HTTP field name; forbidden hop-by-hop fields rejected */
    const char* value; /* printable ASCII, without CR or LF */
} WeaveHttpHeader;
/* Transport availability notifications, scoped to the caller app ID.
 * Subscribe/unsubscribe from the owning application thread. */
typedef void (*WeaveTransportCallback)(WeaveTransport transport, void* context);
/* Push data from a watched TCP/UDP/WebSocket session. Large events are
 * delivered in 512-byte chunks. `data` is borrowed during the callback. */
typedef struct {
    WeaveSocketType type;
    uint16_t session;
    const uint8_t* data;
    size_t size;
    uint32_t offset;
    uint32_t total;
    bool more;
} WeaveSocketEvent;
typedef void (*WeaveSocketEventCallback)(const WeaveSocketEvent* event, void* context);

WeaveTransport weave_get_transport(Weave* weave);
/* Bounded async FIFO: up to WEAVE_QUEUE_MAX queued/in-flight requests.
 * One request is transferred at a time over the WN2 mailbox.
 * Callbacks run on the Weave worker; never release their context early.
 */
bool weave_transport_subscribe(Weave* weave, WeaveTransportCallback cb, void* context);
void weave_transport_unsubscribe(Weave* weave, WeaveTransportCallback cb, void* context);
WeaveStatus weave_http_get(Weave* weave, const char* url, WeaveCallback cb, void* ctx);
WeaveStatus weave_http_request(Weave* weave, const char* method, const char* url,
    const uint8_t* body, size_t length, WeaveCallback cb, void* ctx);
/* HTTPX v3. Headers and any inline body together fit in a 512-byte frame.
 * HTTPS is required for the extended API. Request bytes are staged on SD;
 * do not send unprotected credentials or private keys through the mailbox.
 */
WeaveStatus weave_http_request_ex(Weave* weave, const char* method, const char* url,
    const WeaveHttpHeader* headers, size_t header_count,
    const uint8_t* body, size_t length, WeaveCallback cb, void* ctx);
WeaveStatus weave_http_upload_file_ex(Weave* weave, const char* method, const char* url,
    const WeaveHttpHeader* headers, size_t header_count,
    const char* sd_path, WeaveCallback cb, void* ctx);
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
/* WATCH enables/disables unsolicited socket data delivery via the companion.
 * Subscribe before WATCH; close or unwatch before releasing a FAP context. */
WeaveStatus weave_socket_watch(Weave* weave, WeaveSocketType type,
    uint16_t session, bool enabled, WeaveCallback cb, void* ctx);
bool weave_socket_subscribe_events(Weave* weave, WeaveSocketEventCallback cb, void* ctx);
void weave_socket_unsubscribe_events(Weave* weave, WeaveSocketEventCallback cb, void* ctx);
/* Drain and detach a FAP's non-NULL callback context before it unloads.
 * Drops queued/in-flight requests with that context, unsubscribes its listeners,
 * and waits for callbacks already executing to return. No cancellation callback
 * is emitted. Call from the FAP thread, NEVER from a Weave callback.
 * Returns false on timeout; in that case, the caller MUST retain its context
 * and loaded code until a subsequent call returns true.
 * All callbacks belonging to one FAP should use the SAME unique context.
 * The loader additionally detaches all app-owned callbacks before freeing mapped
 * code; this cannot protect contexts the FAP has already freed. Stop other
 * submitting threads before explicit release and never block a callback on unload.
 */
bool weave_context_release(Weave* weave, void* context, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
#endif

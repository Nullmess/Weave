#pragma once

/* Internal firmware-only entry point.  Keep the public SDK allocator ABI stable
 * so existing third-party FAPs are not broken by the Archive sorting option. */
#include "file_browser_worker.h"

#ifdef __cplusplus
extern "C" {
#endif

BrowserWorker* file_browser_worker_alloc_sorted(
    FuriString* path,
    const char* base_path,
    const char* ext_filter,
    bool skip_assets,
    bool hide_dot_files);

#ifdef __cplusplus
}
#endif

#include "uec_api.h"

#include <string.h>

struct uec_context {
    unsigned int marker;
};

static struct uec_context g_context = {0u};

static uec_result UEC_CALL StubGetCapabilities(uec_context* context,
                                                uec_capabilities* outCapabilities)
{
    if (context != &g_context || outCapabilities == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *outCapabilities = UEC_CAPABILITY_BOOTSTRAP | UEC_CAPABILITY_ACTORS |
        UEC_CAPABILITY_REFLECTION | UEC_CAPABILITY_CLASS_METADATA |
        UEC_CAPABILITY_CONFIGURATION | UEC_CAPABILITY_STREAMING;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubGetLastError(uec_context* context,
                                            char* buffer,
                                            size_t bufferSize,
                                            size_t* requiredSize)
{
    static const char message[] = "No error";
    if (context != &g_context || requiredSize == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *requiredSize = sizeof(message);
    if (buffer == NULL || bufferSize < sizeof(message)) return UEC_RESULT_BUFFER_TOO_SMALL;
    memcpy(buffer, message, sizeof(message));
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubLog(uec_context* context, uec_string_view message)
{
    if (context != &g_context || (message.data == NULL && message.size != 0u)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubReleaseContext(uec_context* context)
{
    return context == &g_context ? UEC_RESULT_OK : UEC_RESULT_INVALID_HANDLE;
}

static const uec_api g_api = {
    .struct_size = sizeof(uec_api),
    .abi_major = UEC_ABI_MAJOR,
    .abi_minor = UEC_ABI_MINOR,
    .get_capabilities = &StubGetCapabilities,
    .get_last_error = &StubGetLastError,
    .log = &StubLog,
    .release_context = &StubReleaseContext
};

UEC_API uec_result UEC_CALL uec_get_api(uint32_t requestedMajor,
                                        uint32_t requestedMinor,
                                        const uec_api** outApi,
                                        uec_context** outContext)
{
    if (outApi == NULL || outContext == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *outApi = NULL;
    *outContext = NULL;
    if (requestedMajor != UEC_ABI_MAJOR || requestedMinor > UEC_ABI_MINOR) {
        return UEC_RESULT_UNSUPPORTED;
    }
    *outApi = &g_api;
    *outContext = &g_context;
    return UEC_RESULT_OK;
}

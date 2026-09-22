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
    if (requiredSize != NULL) *requiredSize = 0;
    if (requiredSize == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (context != &g_context) return UEC_RESULT_INVALID_HANDLE;
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

static uec_result UEC_CALL StubGetRuntimeStats(uec_context* context,
                                               uec_runtime_stats* outStats)
{
    if (outStats == NULL || outStats->struct_size < sizeof(*outStats)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    outStats->active_subscriptions = 0u;
    outStats->pending_requests = 0u;
    outStats->active_callbacks = 0u;
    return context == &g_context ? UEC_RESULT_OK : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetWorldCountByKind(uec_context* context,
                                                   uec_world_kind kind,
                                                   uint32_t* outCount)
{
    (void)kind;
    if (outCount != NULL) *outCount = 0u;
    if (outCount == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetWorldAtByKind(uec_context* context,
                                                uec_world_kind kind,
                                                uint32_t index,
                                                uec_world** outWorld)
{
    (void)kind;
    (void)index;
    if (outWorld != NULL) *outWorld = NULL;
    if (outWorld == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubInvokeActorFunctionValue(
    uec_actor* actor,
    uec_string_view functionName,
    const uec_property_value* argumentValues,
    uint32_t argumentCount,
    uec_property_value* outReturnValue)
{
    (void)actor;
    (void)functionName;
    (void)argumentValues;
    (void)argumentCount;
    if (outReturnValue != NULL && outReturnValue->struct_size >= sizeof(*outReturnValue)) {
        outReturnValue->kind = UEC_PROPERTY_UNKNOWN;
        outReturnValue->bool_value = UEC_FALSE;
        outReturnValue->integer_value = 0;
        outReturnValue->real_value = 0.0;
    }
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubInvokeActorFunctionValues(
    uec_actor* actor,
    uec_string_view functionName,
    const uec_property_value* argumentValues,
    uint32_t argumentCount,
    uec_property_value* outValues,
    uint32_t outCapacity,
    uint32_t* outCount)
{
    (void)actor;
    (void)functionName;
    (void)argumentValues;
    (void)argumentCount;
    (void)outValues;
    (void)outCapacity;
    if (outCount != NULL) *outCount = 0u;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubRunOnGameThread(uec_context* context,
                                               uec_game_thread_callback callback,
                                               void* userData,
                                               uint64_t* outRequestId)
{
    (void)userData;
    if (outRequestId != NULL) *outRequestId = 0;
    if (callback == NULL || outRequestId == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (context != &g_context) return UEC_RESULT_INVALID_HANDLE;
    *outRequestId = 1;
    return UEC_RESULT_OK;
}

static const uec_api g_api = {
    .struct_size = sizeof(uec_api),
    .abi_major = UEC_ABI_MAJOR,
    .abi_minor = UEC_ABI_MINOR,
    .get_capabilities = &StubGetCapabilities,
    .get_last_error = &StubGetLastError,
    .log = &StubLog,
    .release_context = &StubReleaseContext,
    .get_runtime_stats = &StubGetRuntimeStats,
    .get_world_count_by_kind = &StubGetWorldCountByKind,
    .get_world_at_by_kind = &StubGetWorldAtByKind,
    .invoke_actor_function_value = &StubInvokeActorFunctionValue,
    .invoke_actor_function_values = &StubInvokeActorFunctionValues,
    .run_on_game_thread = &StubRunOnGameThread
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

#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"

#include "uec_api.h"

namespace
{
    uec_result VerifyConfigAccess(const uec_api* api,
                                  uec_context* context,
                                  const FString& sectionName)
    {
        if (GConfig == nullptr || api == nullptr || context == nullptr) {
            return UEC_RESULT_NOT_INITIALIZED;
        }

        FTCHARToUTF8 sectionUtf8(*sectionName);
        static constexpr char stringKeyData[] = "BridgeString";
        static constexpr char integerKeyData[] = "BridgeInteger";
        static constexpr char booleanKeyData[] = "SeededBoolean";
        static constexpr char expectedString[] = "uec-config-smoke-roundtrip";
        const uec_string_view section{sectionUtf8.Get(),
                                      static_cast<size_t>(sectionUtf8.Length())};
        const uec_string_view stringKey{stringKeyData, sizeof(stringKeyData) - 1u};
        const uec_string_view integerKey{integerKeyData, sizeof(integerKeyData) - 1u};
        const uec_string_view booleanKey{booleanKeyData, sizeof(booleanKeyData) - 1u};
        const uec_string_view stringValue{expectedString, sizeof(expectedString) - 1u};
        GConfig->SetBool(*sectionName, TEXT("SeededBoolean"), true, GGameIni);

        uec_result result = api->set_config_string(context, section, stringKey, stringValue);
        if (result != UEC_RESULT_OK) return result;
        result = api->set_config_integer(context, section, integerKey,
                                         INT64_C(123456789));
        if (result != UEC_RESULT_OK) return result;

        char observedString[64] = {0};
        size_t requiredSize = 0u;
        result = api->get_config_string(context, section, stringKey, observedString,
                                        sizeof(observedString), &requiredSize);
        if (result != UEC_RESULT_OK || requiredSize != sizeof(expectedString) ||
            FMemory::Memcmp(observedString, expectedString, sizeof(expectedString)) != 0) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }

        int64_t observedInteger = 0;
        result = api->get_config_integer(context, section, integerKey, &observedInteger);
        if (result != UEC_RESULT_OK || observedInteger != INT64_C(123456789)) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }

        uec_bool observedBoolean = UEC_FALSE;
        result = api->get_config_bool(context, section, booleanKey, &observedBoolean);
        if (result != UEC_RESULT_OK || observedBoolean != UEC_TRUE) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }
        return UEC_RESULT_OK;
    }

    uec_result VerifyVersionedPayload(const uec_api* api,
                                      uec_context* context,
                                      const FString& slotName)
    {
        static constexpr uint8_t expectedPayload[] = {
            0x55u, 0x45u, 0x43u, 0x00u, 0xA5u, 0x5Au, 0x01u};
        constexpr uint32_t expectedSchemaVersion = 37u;
        FTCHARToUTF8 slotUtf8(*slotName);
        const uec_string_view slot{slotUtf8.Get(),
                                   static_cast<size_t>(slotUtf8.Length())};
        uec_bool saved = UEC_FALSE;
        uec_result result = api->save_versioned_application_data(
            context, slot, 0, expectedSchemaVersion, expectedPayload,
            sizeof(expectedPayload), &saved);
        if (result != UEC_RESULT_OK || saved != UEC_TRUE) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }

        uint32_t schemaVersion = 0u;
        size_t requiredSize = 0u;
        result = api->load_versioned_application_data(
            context, slot, 0, &schemaVersion, nullptr, 0u, &requiredSize);
        if (result != UEC_RESULT_BUFFER_TOO_SMALL ||
            schemaVersion != expectedSchemaVersion || requiredSize != sizeof(expectedPayload)) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }

        uint8_t shortBuffer[3] = {0xC3u, 0x3Cu, 0x96u};
        schemaVersion = 0u;
        requiredSize = 0u;
        result = api->load_versioned_application_data(
            context, slot, 0, &schemaVersion, shortBuffer, sizeof(shortBuffer), &requiredSize);
        if (result != UEC_RESULT_BUFFER_TOO_SMALL ||
            schemaVersion != expectedSchemaVersion || requiredSize != sizeof(expectedPayload) ||
            shortBuffer[0] != 0xC3u || shortBuffer[1] != 0x3Cu || shortBuffer[2] != 0x96u) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }

        uint8_t observedPayload[sizeof(expectedPayload)] = {0u};
        schemaVersion = 0u;
        requiredSize = 0u;
        result = api->load_versioned_application_data(
            context, slot, 0, &schemaVersion, observedPayload,
            sizeof(observedPayload), &requiredSize);
        if (result != UEC_RESULT_OK || schemaVersion != expectedSchemaVersion ||
            requiredSize != sizeof(expectedPayload) ||
            FMemory::Memcmp(observedPayload, expectedPayload, sizeof(expectedPayload)) != 0) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }
        return UEC_RESULT_OK;
    }
}

extern "C" uec_result UEC_CALL uec_host_persistence_smoke(void)
{
    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == nullptr || context == nullptr || api->release_context == nullptr ||
        api->set_config_string == nullptr || api->get_config_string == nullptr ||
        api->set_config_integer == nullptr || api->get_config_integer == nullptr ||
        api->get_config_bool == nullptr || api->save_versioned_application_data == nullptr ||
        api->load_versioned_application_data == nullptr || api->delete_game_slot == nullptr) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    const FString testId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString sectionName = FString::Printf(TEXT("UEC_APISmoke_%s"), *testId);
    const FString slotName = FString::Printf(TEXT("UEC_APISmoke_%s"), *testId);
    if (result == UEC_RESULT_OK) {
        result = VerifyConfigAccess(api, context, sectionName);
    }
    if (result == UEC_RESULT_OK) {
        result = VerifyVersionedPayload(api, context, slotName);
    }

    uec_result cleanupResult = UEC_RESULT_OK;
    if (GConfig != nullptr) {
        GConfig->EmptySection(*sectionName, GGameIni);
        GConfig->Flush(false, GGameIni);
    }
    if (api != nullptr && context != nullptr && api->delete_game_slot != nullptr) {
        FTCHARToUTF8 slotUtf8(*slotName);
        const uec_string_view slot{slotUtf8.Get(),
                                   static_cast<size_t>(slotUtf8.Length())};
        uec_bool deleted = UEC_FALSE;
        cleanupResult = api->delete_game_slot(context, slot, 0, &deleted);
        if (cleanupResult == UEC_RESULT_OK && result == UEC_RESULT_OK &&
            deleted != UEC_TRUE) {
            cleanupResult = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (api != nullptr && context != nullptr && api->release_context != nullptr) {
        const uec_result releaseResult = api->release_context(context);
        if (cleanupResult == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            cleanupResult = releaseResult;
        }
    }
    if (result == UEC_RESULT_OK && cleanupResult != UEC_RESULT_OK) return cleanupResult;
    return result;
}

#include "c_widget_ui.h"

#include <stddef.h>

uec_result UEC_CALL uec_widget_set_text_child(const uec_api* api,
                                              uec_object* user_widget,
                                              uec_string_view child_name,
                                              uec_string_view text)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_widget_child) +
                               sizeof(api->get_widget_child)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->set_text_block_text == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;

    result = api->set_text_block_text(child, text);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

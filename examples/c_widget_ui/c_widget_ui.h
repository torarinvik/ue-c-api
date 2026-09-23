#ifndef UEC_C_WIDGET_UI_H
#define UEC_C_WIDGET_UI_H

#include "uec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Find a named UTextBlock child, set its contents, and release the weak handle. */
uec_result UEC_CALL uec_widget_set_text_child(const uec_api* api,
                                              uec_object* user_widget,
                                              uec_string_view child_name,
                                              uec_string_view text);

#ifdef __cplusplus
}
#endif

#endif

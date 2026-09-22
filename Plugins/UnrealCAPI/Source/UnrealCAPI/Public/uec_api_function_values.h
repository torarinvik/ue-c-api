#ifndef UEC_API_FUNCTION_VALUES_H
#define UEC_API_FUNCTION_VALUES_H

/* Included by uec_api.h after the common ABI types. Consumers include that header. */
typedef enum uec_function_struct_kind {
    UEC_FUNCTION_STRUCT_NONE = 0,
    UEC_FUNCTION_STRUCT_VECTOR3 = 1,
    UEC_FUNCTION_STRUCT_QUATERNION = 2,
    UEC_FUNCTION_STRUCT_TRANSFORM = 3
} uec_function_struct_kind;

typedef struct uec_function_struct_value {
    uec_function_struct_kind kind;
    union {
        uec_vector3 vector3;
        uec_quaternion quaternion;
        uec_transform transform;
    } value;
} uec_function_struct_value;

/*
 * The struct_value member extends the ABI 1.133 prefix. Set struct_size to
 * sizeof the current record before using typed values. Older record sizes keep
 * the original text-backed struct behavior.
 */
typedef struct uec_function_argument {
    uint32_t struct_size;
    uec_property_kind kind;
    uec_bool bool_value;
    uint8_t reserved[3];
    int64_t integer_value;
    double real_value;
    uec_object* object_value;
    uec_class* class_value;
    uec_world* world_value;
    uec_string_view text_value;
    uec_function_struct_value struct_value;
} uec_function_argument;

typedef struct uec_function_output {
    uint32_t struct_size;
    uec_property_kind kind;
    uec_bool bool_value;
    uint8_t reserved[3];
    int64_t integer_value;
    double real_value;
    uec_object* object_value;
    uec_class* class_value;
    char* text_buffer;
    size_t text_buffer_size;
    size_t text_required_size;
    /* Set kind to a supported struct request before invocation; NONE exports text. */
    uec_function_struct_value struct_value;
} uec_function_output;

typedef void (UEC_CALL *uec_event_bridge_callback)(uint64_t subscription_id,
                                                   int64_t event_id,
                                                   int64_t integer_value,
                                                   double real_value,
                                                   uec_string_view text_value,
                                                   void* user_data);
typedef void (UEC_CALL *uec_latent_function_callback)(uint64_t request_id,
                                                      uec_result result,
                                                      void* user_data);

#endif

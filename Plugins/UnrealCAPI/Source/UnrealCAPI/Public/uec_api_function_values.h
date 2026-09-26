#ifndef UEC_API_FUNCTION_VALUES_H
#define UEC_API_FUNCTION_VALUES_H

/* Included by uec_api.h after the common ABI types. Consumers include that header. */
typedef enum uec_function_struct_kind {
    UEC_FUNCTION_STRUCT_NONE = 0,
    UEC_FUNCTION_STRUCT_VECTOR3 = 1,
    UEC_FUNCTION_STRUCT_QUATERNION = 2,
    UEC_FUNCTION_STRUCT_TRANSFORM = 3,
    UEC_FUNCTION_STRUCT_ROTATOR = 4,
    UEC_FUNCTION_STRUCT_LINEAR_COLOR = 5,
    UEC_FUNCTION_STRUCT_VECTOR2 = 6,
    UEC_FUNCTION_STRUCT_VECTOR4 = 7
} uec_function_struct_kind;

typedef uec_function_struct_kind uec_property_struct_kind;
/* ABI 1.156 property tags remain stable; ABI 1.160 adds these call value tags. */
#define UEC_PROPERTY_STRUCT_NONE UEC_FUNCTION_STRUCT_NONE
#define UEC_PROPERTY_STRUCT_VECTOR3 UEC_FUNCTION_STRUCT_VECTOR3
#define UEC_PROPERTY_STRUCT_QUATERNION UEC_FUNCTION_STRUCT_QUATERNION
#define UEC_PROPERTY_STRUCT_TRANSFORM UEC_FUNCTION_STRUCT_TRANSFORM
#define UEC_PROPERTY_STRUCT_ROTATOR UEC_FUNCTION_STRUCT_ROTATOR
#define UEC_PROPERTY_STRUCT_LINEAR_COLOR UEC_FUNCTION_STRUCT_LINEAR_COLOR
#define UEC_PROPERTY_STRUCT_VECTOR2 UEC_FUNCTION_STRUCT_VECTOR2
#define UEC_PROPERTY_STRUCT_VECTOR4 UEC_FUNCTION_STRUCT_VECTOR4

/* Unreal FVector2D components in X, Y order. */
typedef struct uec_vector2 {
    double x;
    double y;
} uec_vector2;

/* Unreal FVector4 components in X, Y, Z, W order. */
typedef struct uec_vector4 {
    double x;
    double y;
    double z;
    double w;
} uec_vector4;

/* Unreal FRotator fields are pitch, yaw, and roll in degrees. */
typedef struct uec_rotator {
    double pitch;
    double yaw;
    double roll;
} uec_rotator;

/* Components are linear-light RGBA values; they are not clamped to [0, 1]. */
typedef struct uec_linear_color {
    double r;
    double g;
    double b;
    double a;
} uec_linear_color;

typedef struct uec_function_struct_value {
    uec_function_struct_kind kind;
    union {
        uec_vector3 vector3;
        uec_quaternion quaternion;
        uec_transform transform;
        uec_rotator rotator;
        uec_linear_color linear_color;
        uec_vector2 vector2;
        uec_vector4 vector4;
    } value;
} uec_function_struct_value;

/* Size-tagged whole-property values for supported Unreal math structs. */
typedef struct uec_property_struct_value {
    uint32_t struct_size;
    uec_property_struct_kind kind;
    union {
        uec_vector3 vector3;
        uec_quaternion quaternion;
        uec_transform transform;
        uec_rotator rotator;
        uec_linear_color linear_color;
        uec_vector2 vector2;
        uec_vector4 vector4;
    } value;
} uec_property_struct_value;

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

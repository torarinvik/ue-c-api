#ifndef UEC_API_H
#define UEC_API_H

/* Public header: this file must compile as C11 and C++. */
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(UEC_BUILDING_LIBRARY)
#    define UEC_API __declspec(dllexport)
#  else
#    define UEC_API __declspec(dllimport)
#  endif
#  define UEC_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define UEC_API __attribute__((visibility("default")))
#  else
#    define UEC_API
#  endif
#  define UEC_CALL
#endif

#define UEC_ABI_MAJOR 1u
#define UEC_ABI_MINOR 8u

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t uec_bool;
enum { UEC_FALSE = 0u, UEC_TRUE = 1u };

typedef enum uec_result {
    UEC_RESULT_OK = 0,
    UEC_RESULT_INVALID_ARGUMENT = 1,
    UEC_RESULT_INVALID_HANDLE = 2,
    UEC_RESULT_BUFFER_TOO_SMALL = 3,
    UEC_RESULT_NOT_INITIALIZED = 4,
    UEC_RESULT_WRONG_THREAD = 5,
    UEC_RESULT_UNSUPPORTED = 6,
    UEC_RESULT_SHUTTING_DOWN = 7,
    UEC_RESULT_INTERNAL_ERROR = 8
} uec_result;

typedef enum uec_world_kind {
    UEC_WORLD_KIND_UNKNOWN = 0,
    UEC_WORLD_KIND_GAME = 1,
    UEC_WORLD_KIND_PIE = 2,
    UEC_WORLD_KIND_EDITOR = 3,
    UEC_WORLD_KIND_GAME_PREVIEW = 4,
    UEC_WORLD_KIND_INACTIVE = 5
} uec_world_kind;

typedef uint64_t uec_capabilities;
enum {
    UEC_CAPABILITY_BOOTSTRAP = UINT64_C(1) << 0,
    UEC_CAPABILITY_LOGGING = UINT64_C(1) << 1,
    UEC_CAPABILITY_WORLD = UINT64_C(1) << 2,
    UEC_CAPABILITY_ACTORS = UINT64_C(1) << 3,
    UEC_CAPABILITY_REFLECTION = UINT64_C(1) << 4,
    UEC_CAPABILITY_COMPONENTS = UINT64_C(1) << 5,
    UEC_CAPABILITY_TIMERS = UINT64_C(1) << 6,
    UEC_CAPABILITY_CLASS_METADATA = UINT64_C(1) << 7
};

typedef struct uec_context uec_context;
typedef struct uec_world uec_world;
typedef struct uec_actor uec_actor;
typedef struct uec_scene_component uec_scene_component;
typedef struct uec_class uec_class;
typedef struct uec_api_version {
    uint32_t major;
    uint32_t minor;
    uint32_t struct_size;
} uec_api_version;

typedef struct uec_string_view {
    const char* data;
    size_t size;
} uec_string_view;

typedef struct uec_vector3 {
    double x;
    double y;
    double z;
} uec_vector3;

typedef struct uec_quaternion {
    double x;
    double y;
    double z;
    double w;
} uec_quaternion;

typedef struct uec_transform {
    uec_vector3 translation;
    uec_quaternion rotation;
    uec_vector3 scale;
} uec_transform;

typedef enum uec_property_kind {
    UEC_PROPERTY_UNKNOWN = 0,
    UEC_PROPERTY_BOOL = 1,
    UEC_PROPERTY_INTEGER = 2,
    UEC_PROPERTY_FLOAT = 3,
    UEC_PROPERTY_DOUBLE = 4,
    UEC_PROPERTY_ENUM = 5,
    UEC_PROPERTY_STRING = 6,
    UEC_PROPERTY_NAME = 7,
    UEC_PROPERTY_TEXT = 8,
    UEC_PROPERTY_OBJECT = 9,
    UEC_PROPERTY_CLASS = 10,
    UEC_PROPERTY_STRUCT = 11,
    UEC_PROPERTY_ARRAY = 12,
    UEC_PROPERTY_MAP = 13,
    UEC_PROPERTY_SET = 14
} uec_property_kind;

typedef void (UEC_CALL *uec_timer_callback)(uint64_t timer_id, void* user_data);

typedef struct uec_api {
    uint32_t struct_size;
    uint32_t abi_major;
    uint32_t abi_minor;

    uec_result (UEC_CALL *get_capabilities)(uec_context* context,
                                             uec_capabilities* out_capabilities);
    uec_result (UEC_CALL *get_last_error)(uec_context* context,
                                           char* buffer,
                                           size_t buffer_size,
                                           size_t* required_size);
    uec_result (UEC_CALL *log)(uec_context* context, uec_string_view message);
    uec_result (UEC_CALL *release_context)(uec_context* context);
    uec_result (UEC_CALL *get_world_count)(uec_context* context, uint32_t* out_count);
    uec_result (UEC_CALL *get_world_at)(uec_context* context,
                                        uint32_t index,
                                        uec_world** out_world);
    uec_result (UEC_CALL *get_world_kind)(uec_world* world,
                                          uec_world_kind* out_kind);
    uec_result (UEC_CALL *get_default_world)(uec_context* context, uec_world** out_world);
    uec_result (UEC_CALL *release_world)(uec_world* world);
    uec_result (UEC_CALL *spawn_actor)(uec_world* world,
                                       uec_string_view class_path,
                                       const uec_transform* transform,
                                       uec_actor** out_actor);
    uec_result (UEC_CALL *release_actor)(uec_actor* actor);
    uec_result (UEC_CALL *destroy_actor)(uec_actor* actor);
    uec_result (UEC_CALL *get_actor_transform)(uec_actor* actor, uec_transform* out_transform);
    uec_result (UEC_CALL *set_actor_transform)(uec_actor* actor,
                                               const uec_transform* transform,
                                               uec_bool sweep);
    uec_result (UEC_CALL *get_actor_name)(uec_actor* actor,
                                          char* buffer,
                                          size_t buffer_size,
                                          size_t* required_size);
    uec_result (UEC_CALL *actor_has_tag)(uec_actor* actor,
                                         uec_string_view tag,
                                         uec_bool* out_has_tag);
    uec_result (UEC_CALL *get_actor_root_component)(uec_actor* actor,
                                                    uec_scene_component** out_component);
    uec_result (UEC_CALL *get_actor_component_count)(uec_actor* actor,
                                                     uint32_t* out_count);
    uec_result (UEC_CALL *get_actor_component_at)(uec_actor* actor,
                                                  uint32_t index,
                                                  uec_scene_component** out_component);
    uec_result (UEC_CALL *release_scene_component)(uec_scene_component* component);
    uec_result (UEC_CALL *get_component_transform)(uec_scene_component* component,
                                                   uec_transform* out_transform);
    uec_result (UEC_CALL *set_component_transform)(uec_scene_component* component,
                                                   const uec_transform* transform,
                                                   uec_bool sweep);
    uec_result (UEC_CALL *set_component_visible)(uec_scene_component* component,
                                                 uec_bool visible,
                                                 uec_bool propagate_to_children);
    uec_result (UEC_CALL *set_component_active)(uec_scene_component* component,
                                                uec_bool active,
                                                uec_bool reset);
    uec_result (UEC_CALL *set_timer)(uec_world* world,
                                     double interval_seconds,
                                     uec_bool looping,
                                     uec_timer_callback callback,
                                     void* user_data,
                                     uint64_t* out_timer_id);
    uec_result (UEC_CALL *clear_timer)(uec_world* world, uint64_t timer_id);
    uec_result (UEC_CALL *find_class)(uec_context* context,
                                      uec_string_view class_path,
                                      uec_class** out_class);
    uec_result (UEC_CALL *release_class)(uec_class* klass);
    uec_result (UEC_CALL *get_class_name)(uec_class* klass,
                                          char* buffer,
                                          size_t buffer_size,
                                          size_t* required_size);
    uec_result (UEC_CALL *class_is_a)(uec_class* klass,
                                      uec_string_view parent_class_path,
                                      uec_bool* out_is_a);
    uec_result (UEC_CALL *get_class_property_count)(uec_class* klass,
                                                    uint32_t* out_count);
    uec_result (UEC_CALL *get_class_property_at)(uec_class* klass,
                                                 uint32_t index,
                                                 char* name_buffer,
                                                 size_t name_buffer_size,
                                                 size_t* name_required_size,
                                                 uec_property_kind* out_kind);
} uec_api;

/* Bootstrap entry point. The returned function table remains valid until the
 * plugin is unloaded. The context is opaque and must be released with the
 * table's release_context function. */
UEC_API uec_result UEC_CALL uec_get_api(uint32_t requested_major,
                                        uint32_t requested_minor,
                                        const uec_api** out_api,
                                        uec_context** out_context);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UEC_API_H */

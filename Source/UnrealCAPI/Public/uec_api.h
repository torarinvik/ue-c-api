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
#define UEC_ABI_MINOR 78u

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
    UEC_RESULT_INTERNAL_ERROR = 8,
    UEC_RESULT_QUEUE_FULL = 9
} uec_result;

typedef enum uec_world_kind {
    UEC_WORLD_KIND_UNKNOWN = 0,
    UEC_WORLD_KIND_GAME = 1,
    UEC_WORLD_KIND_PIE = 2,
    UEC_WORLD_KIND_EDITOR = 3,
    UEC_WORLD_KIND_GAME_PREVIEW = 4,
    UEC_WORLD_KIND_INACTIVE = 5
} uec_world_kind;

typedef enum uec_net_mode {
    UEC_NET_MODE_UNKNOWN = 0,
    UEC_NET_MODE_STANDALONE = 1,
    UEC_NET_MODE_DEDICATED_SERVER = 2,
    UEC_NET_MODE_LISTEN_SERVER = 3,
    UEC_NET_MODE_CLIENT = 4
} uec_net_mode;

typedef uint64_t uec_capabilities;
enum {
    UEC_CAPABILITY_BOOTSTRAP = UINT64_C(1) << 0,
    UEC_CAPABILITY_LOGGING = UINT64_C(1) << 1,
    UEC_CAPABILITY_WORLD = UINT64_C(1) << 2,
    UEC_CAPABILITY_ACTORS = UINT64_C(1) << 3,
    UEC_CAPABILITY_REFLECTION = UINT64_C(1) << 4,
    UEC_CAPABILITY_COMPONENTS = UINT64_C(1) << 5,
    UEC_CAPABILITY_TIMERS = UINT64_C(1) << 6,
    UEC_CAPABILITY_CLASS_METADATA = UINT64_C(1) << 7,
    UEC_CAPABILITY_COLLISION = UINT64_C(1) << 8,
    UEC_CAPABILITY_ASSETS = UINT64_C(1) << 9,
    UEC_CAPABILITY_ASYNC_ASSETS = UINT64_C(1) << 10,
    UEC_CAPABILITY_LEVEL_TRAVEL = UINT64_C(1) << 11,
    UEC_CAPABILITY_PLAYER_FLOW = UINT64_C(1) << 12,
    UEC_CAPABILITY_INPUT = UINT64_C(1) << 13,
    UEC_CAPABILITY_PHYSICS = UINT64_C(1) << 14,
    UEC_CAPABILITY_COLLISION_QUERIES = UINT64_C(1) << 15,
    UEC_CAPABILITY_AUDIO = UINT64_C(1) << 16,
    UEC_CAPABILITY_UI = UINT64_C(1) << 17,
    UEC_CAPABILITY_CAMERA = UINT64_C(1) << 18,
    UEC_CAPABILITY_SAVE_DATA = UINT64_C(1) << 19,
    UEC_CAPABILITY_THREADING = UINT64_C(1) << 20,
    UEC_CAPABILITY_MOVEMENT = UINT64_C(1) << 21,
    UEC_CAPABILITY_PRESENTATION = UINT64_C(1) << 22,
    UEC_CAPABILITY_RETAINED_OBJECTS = UINT64_C(1) << 23,
    UEC_CAPABILITY_CONFIGURATION = UINT64_C(1) << 24,
    UEC_CAPABILITY_STREAMING = UINT64_C(1) << 25
};

typedef struct uec_context uec_context;
typedef struct uec_world uec_world;
typedef struct uec_actor uec_actor;
typedef struct uec_scene_component uec_scene_component;
typedef struct uec_class uec_class;
typedef struct uec_object uec_object;
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

typedef struct uec_property_value {
    uint32_t struct_size;
    uec_property_kind kind;
    uec_bool bool_value;
    uint8_t reserved[3];
    int64_t integer_value;
    double real_value;
} uec_property_value;

typedef enum uec_trace_channel {
    UEC_TRACE_VISIBILITY = 0,
    UEC_TRACE_CAMERA = 1,
    UEC_TRACE_WORLD_STATIC = 2,
    UEC_TRACE_WORLD_DYNAMIC = 3,
    UEC_TRACE_PAWN = 4,
    UEC_TRACE_PHYSICS_BODY = 5
} uec_trace_channel;

typedef enum uec_collision_shape_kind {
    UEC_COLLISION_SHAPE_SPHERE = 0,
    UEC_COLLISION_SHAPE_BOX = 1,
    UEC_COLLISION_SHAPE_CAPSULE = 2
} uec_collision_shape_kind;

typedef enum uec_collision_enabled {
    UEC_COLLISION_DISABLED = 0,
    UEC_COLLISION_QUERY_ONLY = 1,
    UEC_COLLISION_PHYSICS_ONLY = 2,
    UEC_COLLISION_QUERY_AND_PHYSICS = 3
} uec_collision_enabled;

typedef enum uec_widget_visibility {
    UEC_WIDGET_VISIBLE = 0,
    UEC_WIDGET_COLLAPSED = 1,
    UEC_WIDGET_HIDDEN = 2
} uec_widget_visibility;

typedef enum uec_input_action_value_kind {
    UEC_INPUT_ACTION_VALUE_BOOLEAN = 0,
    UEC_INPUT_ACTION_VALUE_AXIS_1D = 1,
    UEC_INPUT_ACTION_VALUE_AXIS_2D = 2,
    UEC_INPUT_ACTION_VALUE_AXIS_3D = 3
} uec_input_action_value_kind;

typedef enum uec_input_trigger_event {
    UEC_INPUT_TRIGGER_STARTED = 1,
    UEC_INPUT_TRIGGER_ONGOING = 2,
    UEC_INPUT_TRIGGER_TRIGGERED = 3,
    UEC_INPUT_TRIGGER_CANCELED = 4,
    UEC_INPUT_TRIGGER_COMPLETED = 5
} uec_input_trigger_event;

typedef struct uec_input_action_value {
    uint32_t struct_size;
    uec_input_action_value_kind kind;
    uec_bool bool_value;
    uint8_t reserved[3];
    uec_vector3 axis;
} uec_input_action_value;

typedef void (UEC_CALL *uec_input_action_callback)(uint64_t binding_id,
                                                   uec_input_action_value value,
                                                   void* user_data);

typedef struct uec_collision_shape {
    uint32_t struct_size;
    uec_collision_shape_kind kind;
    uint32_t reserved;
    double radius;
    uec_vector3 half_extents;
    double half_height;
} uec_collision_shape;

typedef struct uec_hit_result {
    uec_bool blocking_hit;
    uint8_t reserved[7];
    uec_vector3 location;
    uec_vector3 normal;
    double distance;
    uec_actor* actor;
} uec_hit_result;

typedef void (UEC_CALL *uec_timer_callback)(uint64_t timer_id, void* user_data);
typedef void (UEC_CALL *uec_tick_callback)(uint64_t subscription_id,
                                           double delta_seconds,
                                           void* user_data);
typedef void (UEC_CALL *uec_audio_finished_callback)(uint64_t subscription_id,
                                                     void* user_data);
typedef void (UEC_CALL *uec_animation_finished_callback)(uint64_t subscription_id,
                                                         void* user_data);
typedef void (UEC_CALL *uec_component_hit_callback)(uint64_t subscription_id,
                                                    uec_actor* other_actor,
                                                    uec_vector3 normal_impulse,
                                                    void* user_data);
typedef void (UEC_CALL *uec_widget_event_callback)(uint64_t subscription_id,
                                                   void* user_data);
typedef void (UEC_CALL *uec_object_load_callback)(uint64_t request_id,
                                                  uec_result result,
                                                  uec_object* object,
                                                  void* user_data);
typedef void (UEC_CALL *uec_save_game_callback)(uint64_t request_id,
                                                uec_result result,
                                                uec_object* save_game,
                                                uec_bool success,
                                                void* user_data);
typedef void (UEC_CALL *uec_game_thread_callback)(void* user_data);

typedef struct uec_api {
    uint32_t struct_size;
    uint32_t abi_major;
    uint32_t abi_minor;

    /* Bootstrap, diagnostics, and context ownership. */
    uec_result (UEC_CALL *get_capabilities)(uec_context* context,
                                             uec_capabilities* out_capabilities);
    uec_result (UEC_CALL *get_last_error)(uec_context* context,
                                           char* buffer,
                                           size_t buffer_size,
                                           size_t* required_size);
    uec_result (UEC_CALL *log)(uec_context* context, uec_string_view message);
    uec_result (UEC_CALL *release_context)(uec_context* context);

    /* World selection and player flow. */
    uec_result (UEC_CALL *get_world_count)(uec_context* context, uint32_t* out_count);
    uec_result (UEC_CALL *get_world_at)(uec_context* context,
                                        uint32_t index,
                                        uec_world** out_world);
    uec_result (UEC_CALL *get_world_kind)(uec_world* world,
                                          uec_world_kind* out_kind);
    uec_result (UEC_CALL *get_world_name)(uec_world* world,
                                          char* buffer,
                                          size_t buffer_size,
                                          size_t* required_size);
    uec_result (UEC_CALL *travel_world)(uec_world* world,
                                        uec_string_view level_path);
    uec_result (UEC_CALL *get_first_player_controller)(uec_world* world,
                                                      uec_actor** out_controller);
    uec_result (UEC_CALL *get_controller_pawn)(uec_actor* controller,
                                               uec_actor** out_pawn);
    uec_result (UEC_CALL *possess_pawn)(uec_actor* controller,
                                        uec_actor* pawn);
    uec_result (UEC_CALL *set_controller_view_target)(uec_actor* controller,
                                                      uec_actor* view_target);

    /* Input polling, physics, and the default-world convenience path. */
    uec_result (UEC_CALL *get_input_key_down)(uec_actor* controller,
                                              uec_string_view key_name,
                                              uec_bool* out_down);
    uec_result (UEC_CALL *get_input_key_value)(uec_actor* controller,
                                               uec_string_view key_name,
                                               double* out_value);
    uec_result (UEC_CALL *get_actor_velocity)(uec_actor* actor,
                                              uec_vector3* out_velocity);
    uec_result (UEC_CALL *set_actor_physics_velocity)(uec_actor* actor,
                                                      uec_vector3 velocity,
                                                      uec_bool add_to_current);
    uec_result (UEC_CALL *apply_actor_impulse)(uec_actor* actor,
                                               uec_vector3 impulse,
                                               uec_bool velocity_change);
    uec_result (UEC_CALL *apply_actor_force)(uec_actor* actor,
                                             uec_vector3 force);
    uec_result (UEC_CALL *get_default_world)(uec_context* context, uec_world** out_world);
    uec_result (UEC_CALL *release_world)(uec_world* world);

    /* Actor and scene-component lifetime, transforms, tags, and timers. */
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

    /* Class and reflected-property metadata and access. */
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
    uec_result (UEC_CALL *get_actor_property_value)(uec_actor* actor,
                                                    uec_string_view property_name,
                                                    uec_property_value* out_value);
    uec_result (UEC_CALL *get_actor_property_string)(uec_actor* actor,
                                                     uec_string_view property_name,
                                                     char* buffer,
                                                     size_t buffer_size,
                                                     size_t* required_size,
                                                     uec_property_kind* out_kind);
    uec_result (UEC_CALL *set_actor_property_value)(uec_actor* actor,
                                                    uec_string_view property_name,
                                                    const uec_property_value* value);
    uec_result (UEC_CALL *set_actor_property_string)(uec_actor* actor,
                                                     uec_string_view property_name,
                                                     uec_string_view value);

    /* Collision, object loading, and initial presentation adapters. */
    uec_result (UEC_CALL *line_trace)(uec_world* world,
                                      uec_vector3 start,
                                      uec_vector3 end,
                                      uec_trace_channel channel,
                                      uec_bool trace_complex,
                                      uec_hit_result* out_hit);
    uec_result (UEC_CALL *invoke_actor_function)(uec_actor* actor,
                                                 uec_string_view function_name);
    uec_result (UEC_CALL *load_object)(uec_context* context,
                                       uec_string_view object_path,
                                       uec_object** out_object);
    uec_result (UEC_CALL *release_object)(uec_object* object);
    uec_result (UEC_CALL *get_object_name)(uec_object* object,
                                           char* buffer,
                                           size_t buffer_size,
                                           size_t* required_size);
    uec_result (UEC_CALL *object_is_a)(uec_object* object,
                                       uec_string_view class_path,
                                       uec_bool* out_is_a);
    uec_result (UEC_CALL *request_object_load)(uec_context* context,
                                               uec_string_view object_path,
                                               uec_object_load_callback callback,
                                               void* user_data,
                                               uint64_t* out_request_id);
    uec_result (UEC_CALL *cancel_object_load)(uec_context* context,
                                              uint64_t request_id);
    uec_result (UEC_CALL *sweep_trace)(uec_world* world,
                                       uec_vector3 start,
                                       uec_vector3 end,
                                       const uec_collision_shape* shape,
                                       uec_trace_channel channel,
                                       uec_bool trace_complex,
                                       uec_hit_result* out_hit);
    uec_result (UEC_CALL *overlap_shape)(uec_world* world,
                                         uec_vector3 center,
                                         const uec_collision_shape* shape,
                                         uec_trace_channel channel,
                                         uint32_t max_hits,
                                         uec_actor** out_actors,
                                         uint32_t* out_count);
    uec_result (UEC_CALL *play_sound_at_location)(uec_world* world,
                                                  uec_object* sound,
                                                  uec_vector3 location,
                                                  double volume_multiplier,
                                                  double pitch_multiplier);
    uec_result (UEC_CALL *create_widget)(uec_world* world,
                                         uec_string_view widget_class_path,
                                         uec_object** out_widget);
    uec_result (UEC_CALL *add_widget_to_viewport)(uec_object* widget,
                                                  int32_t z_order);
    uec_result (UEC_CALL *remove_widget_from_parent)(uec_object* widget);
    uec_result (UEC_CALL *get_camera_field_of_view)(uec_scene_component* component,
                                                    double* out_degrees);
    uec_result (UEC_CALL *set_camera_field_of_view)(uec_scene_component* component,
                                                    double degrees);

    /* Generic object properties and save-game persistence. */
    uec_result (UEC_CALL *get_object_property_value)(uec_object* object,
                                                     uec_string_view property_name,
                                                     uec_property_value* out_value);
    uec_result (UEC_CALL *get_object_property_string)(uec_object* object,
                                                      uec_string_view property_name,
                                                      char* buffer,
                                                      size_t buffer_size,
                                                      size_t* required_size,
                                                      uec_property_kind* out_kind);
    uec_result (UEC_CALL *set_object_property_value)(uec_object* object,
                                                     uec_string_view property_name,
                                                     const uec_property_value* value);
    uec_result (UEC_CALL *set_object_property_string)(uec_object* object,
                                                      uec_string_view property_name,
                                                      uec_string_view value);
    uec_result (UEC_CALL *create_save_game)(uec_context* context,
                                            uec_string_view class_path,
                                            uec_object** out_save_game);
    uec_result (UEC_CALL *save_game_to_slot)(uec_object* save_game,
                                             uec_string_view slot_name,
                                             int32_t user_index,
                                             uec_bool* out_saved);
    uec_result (UEC_CALL *load_game_from_slot)(uec_context* context,
                                               uec_string_view class_path,
                                               uec_string_view slot_name,
                                               int32_t user_index,
                                               uec_object** out_save_game);
    uec_result (UEC_CALL *delete_game_slot)(uec_context* context,
                                            uec_string_view slot_name,
                                            int32_t user_index,
                                            uec_bool* out_deleted);

    /* Queued game-thread work, movement, meshes, animation, and materials. */
    uec_result (UEC_CALL *run_on_game_thread)(uec_context* context,
                                              uec_game_thread_callback callback,
                                              void* user_data,
                                              uint64_t* out_request_id);
    uec_result (UEC_CALL *cancel_game_thread_request)(uec_context* context,
                                                      uint64_t request_id);
    uec_result (UEC_CALL *add_pawn_movement_input)(uec_actor* pawn,
                                                   uec_vector3 world_direction,
                                                   double scale,
                                                   uec_bool force);
    uec_result (UEC_CALL *jump_character)(uec_actor* character);
    uec_result (UEC_CALL *stop_character_jumping)(uec_actor* character);
    uec_result (UEC_CALL *set_static_mesh)(uec_scene_component* component,
                                           uec_object* mesh);
    uec_result (UEC_CALL *set_skeletal_mesh)(uec_scene_component* component,
                                             uec_object* mesh,
                                             uec_bool reinitialize_pose);
    uec_result (UEC_CALL *play_skeletal_animation)(uec_scene_component* component,
                                                   uec_object* animation,
                                                   uec_bool looping);
    uec_result (UEC_CALL *stop_skeletal_animation)(uec_scene_component* component);
    uec_result (UEC_CALL *set_component_material_scalar)(uec_scene_component* component,
                                                         uec_string_view parameter_name,
                                                         double value);
    uec_result (UEC_CALL *set_component_material_vector)(uec_scene_component* component,
                                                         uec_string_view parameter_name,
                                                         uec_vector3 value);

    /* Retained objects, component/actor identity, and input mappings. */
    uec_result (UEC_CALL *retain_object)(uec_object* object,
                                         uec_object** out_retained_object);
    uec_result (UEC_CALL *get_component_class_name)(uec_scene_component* component,
                                                    char* buffer,
                                                    size_t buffer_size,
                                                    size_t* required_size);
    uec_result (UEC_CALL *component_is_a)(uec_scene_component* component,
                                          uec_string_view class_path,
                                          uec_bool* out_is_a);
    uec_result (UEC_CALL *attach_scene_component)(uec_scene_component* child,
                                                 uec_scene_component* parent,
                                                 uec_bool keep_world_transform,
                                                 uec_string_view socket_name);
    uec_result (UEC_CALL *detach_scene_component)(uec_scene_component* component,
                                                  uec_bool keep_world_transform);
    uec_result (UEC_CALL *get_actor_class_name)(uec_actor* actor,
                                                char* buffer,
                                                size_t buffer_size,
                                                size_t* required_size);
    uec_result (UEC_CALL *actor_is_a)(uec_actor* actor,
                                      uec_string_view class_path,
                                      uec_bool* out_is_a);
    uec_result (UEC_CALL *add_input_mapping_context)(uec_actor* controller,
                                                     uec_object* mapping_context,
                                                     int32_t priority);
    uec_result (UEC_CALL *remove_input_mapping_context)(uec_actor* controller,
                                                        uec_object* mapping_context);

    /* Reflected functions, collision settings, and attached audio. */
    uec_result (UEC_CALL *get_class_function_count)(uec_class* klass,
                                                    uint32_t* out_count);
    uec_result (UEC_CALL *get_class_function_at)(uec_class* klass,
                                                 uint32_t index,
                                                 char* name_buffer,
                                                 size_t name_buffer_size,
                                                 size_t* name_required_size,
                                                 uint32_t* out_parameter_count,
                                                 uec_bool* out_has_return_value,
                                                 uec_bool* out_is_latent);
    uec_result (UEC_CALL *set_component_collision_enabled)(uec_scene_component* component,
                                                           uec_collision_enabled enabled);
    uec_result (UEC_CALL *set_component_collision_response)(uec_scene_component* component,
                                                            uec_trace_channel channel,
                                                            uec_bool block);
    uec_result (UEC_CALL *is_object_path_loaded)(uec_context* context,
                                                 uec_string_view object_path,
                                                 uec_bool* out_loaded);
    uec_result (UEC_CALL *spawn_sound_attached)(uec_scene_component* attach_to,
                                                uec_object* sound,
                                                uec_string_view socket_name,
                                                double volume_multiplier,
                                                double pitch_multiplier,
                                                uec_object** out_audio_component);
    uec_result (UEC_CALL *stop_audio_component)(uec_object* audio_component);
    uec_result (UEC_CALL *get_input_action_value)(uec_actor* controller,
                                                  uec_object* action,
                                                  uec_input_action_value* out_value);
    uec_result (UEC_CALL *line_trace_filtered)(uec_world* world,
                                               uec_vector3 start,
                                               uec_vector3 end,
                                               uec_trace_channel channel,
                                               uec_bool trace_complex,
                                               const uec_actor* const* ignored_actors,
                                               uint32_t ignored_actor_count,
                                               uec_hit_result* out_hit);
    uec_result (UEC_CALL *inject_input_action_value)(uec_actor* controller,
                                                     uec_object* action,
                                                     const uec_input_action_value* value);
    uec_result (UEC_CALL *get_actor_property_object)(uec_actor* actor,
                                                     uec_string_view property_name,
                                                     uec_object** out_object);
    uec_result (UEC_CALL *set_actor_property_object)(uec_actor* actor,
                                                     uec_string_view property_name,
                                                     uec_object* object);
    uec_result (UEC_CALL *get_object_property_object)(uec_object* object,
                                                      uec_string_view property_name,
                                                      uec_object** out_value);
    uec_result (UEC_CALL *set_object_property_object)(uec_object* object,
                                                      uec_string_view property_name,
                                                      uec_object* value);

    /* Asynchronous save operations and indexed actor queries. */
    uec_result (UEC_CALL *async_save_game_to_slot)(uec_object* save_game,
                                                   uec_string_view slot_name,
                                                   int32_t user_index,
                                                   uec_save_game_callback callback,
                                                   void* user_data,
                                                   uint64_t* out_request_id);
    uec_result (UEC_CALL *async_load_game_from_slot)(uec_context* context,
                                                     uec_string_view slot_name,
                                                     int32_t user_index,
                                                     uec_save_game_callback callback,
                                                     void* user_data,
                                                     uint64_t* out_request_id);
    uec_result (UEC_CALL *cancel_save_game_request)(uec_context* context,
                                                    uint64_t request_id);
    uec_result (UEC_CALL *get_actor_count_by_class)(uec_world* world,
                                                    uec_string_view class_path,
                                                    uint32_t* out_count);
    uec_result (UEC_CALL *get_actor_at_by_class)(uec_world* world,
                                                 uec_string_view class_path,
                                                 uint32_t index,
                                                 uec_actor** out_actor);
    uec_result (UEC_CALL *destroy_audio_component)(uec_object* audio_component);

    /* Tokenized input bindings and explicit world-context access. */
    uec_result (UEC_CALL *bind_input_action)(uec_actor* actor,
                                             uec_object* action,
                                             uec_input_trigger_event trigger_event,
                                             uec_input_action_callback callback,
                                             void* user_data,
                                             uint64_t* out_binding_id);
    uec_result (UEC_CALL *unbind_input_action)(uec_context* context,
                                               uint64_t binding_id);
    uec_result (UEC_CALL *get_player_controller)(uec_world* world,
                                                 uint32_t player_index,
                                                 uec_actor** out_controller);
    uec_result (UEC_CALL *get_world_game_instance)(uec_world* world,
                                                   uec_object** out_game_instance);

    /* Append-only ABI extensions: reflected calls, subscriptions, identity,
     * UMG, component velocity, and network context. */
    uec_result (UEC_CALL *invoke_actor_function_text)(
        uec_actor* actor,
        uec_string_view function_name,
        const uec_string_view* argument_values,
        uint32_t argument_count,
        char* return_buffer,
        size_t return_buffer_size,
        size_t* return_required_size,
        uec_property_kind* out_return_kind);
    uec_result (UEC_CALL *subscribe_world_tick)(uec_world* world,
                                                uec_tick_callback callback,
                                                void* user_data,
                                                uint64_t* out_subscription_id);
    uec_result (UEC_CALL *unsubscribe_world_tick)(uec_context* context,
                                                  uint64_t subscription_id);
    uec_result (UEC_CALL *bind_audio_finished)(uec_object* audio_component,
                                               uec_audio_finished_callback callback,
                                               void* user_data,
                                               uint64_t* out_subscription_id);
    uec_result (UEC_CALL *unbind_audio_finished)(uec_context* context,
                                                 uint64_t subscription_id);
    uec_result (UEC_CALL *get_object_path)(uec_object* object,
                                           char* buffer,
                                           size_t buffer_size,
                                           size_t* required_size);
    uec_result (UEC_CALL *get_object_class_name)(uec_object* object,
                                                 char* buffer,
                                                 size_t buffer_size,
                                                 size_t* required_size);
    uec_result (UEC_CALL *set_widget_visibility)(uec_object* widget,
                                                 uec_widget_visibility visibility);
    uec_result (UEC_CALL *set_text_block_text)(uec_object* widget,
                                               uec_string_view text);
    uec_result (UEC_CALL *bind_button_clicked)(uec_object* button,
                                               uec_widget_event_callback callback,
                                               void* user_data,
                                               uint64_t* out_subscription_id);
    uec_result (UEC_CALL *unbind_button_clicked)(uec_context* context,
                                                 uint64_t subscription_id);
    uec_result (UEC_CALL *get_component_velocity)(uec_scene_component* component,
                                                  uec_vector3* out_velocity);
    uec_result (UEC_CALL *get_world_pie_instance)(uec_world* world,
                                                  int32_t* out_instance);
    uec_result (UEC_CALL *get_world_net_mode)(uec_world* world,
                                              uec_net_mode* out_mode);
    uec_result (UEC_CALL *get_actor_tag_count)(uec_actor* actor,
                                               uint32_t* out_count);
    uec_result (UEC_CALL *get_actor_tag_at)(uec_actor* actor,
                                            uint32_t index,
                                            char* buffer,
                                            size_t buffer_size,
                                            size_t* required_size);
    uec_result (UEC_CALL *get_actor_bounds)(uec_actor* actor,
                                            uec_vector3* out_origin,
                                            uec_vector3* out_extent);
    uec_result (UEC_CALL *find_player_start)(uec_world* world,
                                             uint32_t player_index,
                                             uec_actor** out_start);
    uec_result (UEC_CALL *bind_animation_finished)(uec_scene_component* component,
                                                   uec_animation_finished_callback callback,
                                                   void* user_data,
                                                   uint64_t* out_subscription_id);
    uec_result (UEC_CALL *unbind_animation_finished)(uec_context* context,
                                                     uint64_t subscription_id);
    uec_result (UEC_CALL *get_world_has_authority)(uec_world* world,
                                                   uec_bool* out_has_authority);
    uec_result (UEC_CALL *get_world_game_mode)(uec_world* world,
                                               uec_object** out_game_mode);
    uec_result (UEC_CALL *get_world_game_state)(uec_world* world,
                                                uec_object** out_game_state);
    uec_result (UEC_CALL *get_actor_component_count_by_class)(
        uec_actor* actor,
        uec_string_view class_path,
        uint32_t* out_count);
    uec_result (UEC_CALL *get_actor_component_at_by_class)(
        uec_actor* actor,
        uec_string_view class_path,
        uint32_t index,
        uec_scene_component** out_component);
    uec_result (UEC_CALL *get_config_string)(uec_context* context,
                                             uec_string_view section,
                                             uec_string_view key,
                                             char* buffer,
                                             size_t buffer_size,
                                             size_t* required_size);
    uec_result (UEC_CALL *set_config_string)(uec_context* context,
                                             uec_string_view section,
                                             uec_string_view key,
                                             uec_string_view value);
    uec_result (UEC_CALL *get_streaming_level_count)(uec_world* world,
                                                     uint32_t* out_count);
    uec_result (UEC_CALL *get_streaming_level_at)(uec_world* world,
                                                  uint32_t index,
                                                  char* package_buffer,
                                                  size_t package_buffer_size,
                                                  size_t* package_required_size,
                                                  uec_bool* out_loaded,
                                                  uec_bool* out_visible);
    uec_result (UEC_CALL *set_streaming_level_state)(uec_world* world,
                                                     uec_string_view package_path,
                                                     uec_bool should_be_loaded,
                                                     uec_bool should_be_visible);
    uec_result (UEC_CALL *is_class_path_loaded)(uec_context* context,
                                                uec_string_view class_path,
                                                uec_bool* out_loaded);
    uec_result (UEC_CALL *bind_component_hit)(uec_scene_component* component,
                                              uec_component_hit_callback callback,
                                              void* user_data,
                                              uint64_t* out_subscription_id);
    uec_result (UEC_CALL *unbind_component_hit)(uec_context* context,
                                                uint64_t subscription_id);
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

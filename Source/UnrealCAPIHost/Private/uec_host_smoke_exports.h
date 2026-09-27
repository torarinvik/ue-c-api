#pragma once

#include "uec_api.h"

class UWorld;

extern "C" {
    uec_result UEC_CALL uec_host_smoke_bootstrap(void);
    uec_result UEC_CALL uec_host_reflection_metadata_smoke(void);
    uec_result UEC_CALL uec_host_persistence_smoke(void);
    uec_result UEC_CALL uec_host_collision_smoke(void);
    uec_result UEC_CALL uec_host_collision_queries_smoke(void);
    uec_result UEC_CALL uec_host_reflection_containers_smoke(void);
    uec_result UEC_CALL uec_host_reflection_scalars_smoke(void);
    uec_result UEC_CALL uec_host_reflection_guid_smoke(void);
    uec_result UEC_CALL uec_host_reflection_temporal_smoke(void);
    uec_result UEC_CALL uec_host_player_flow_smoke(void);
    uec_result UEC_CALL uec_host_animation_smoke_start(void);
    uec_bool UEC_CALL uec_host_animation_smoke_is_running(void);
    void UEC_CALL uec_host_animation_smoke_cancel(void);
    uec_result UEC_CALL uec_host_blueprint_invocation_smoke(void);
    uec_result UEC_CALL uec_host_gc_smoke(void);
    uec_result UEC_CALL uec_host_physics_smoke_start(void);
    uec_bool UEC_CALL uec_host_physics_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_physics_smoke_cancel(void);
    uec_result UEC_CALL uec_host_authority_smoke(void);
    uec_result UEC_CALL uec_host_event_bridge_smoke(void);
    uec_result UEC_CALL uec_host_latent_smoke_start(void);
    uec_bool UEC_CALL uec_host_latent_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_latent_smoke_cancel(void);
    uec_result UEC_CALL uec_host_input_smoke_start(void);
    uec_bool UEC_CALL uec_host_input_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_input_smoke_cancel(void);
    uec_result UEC_CALL uec_host_queue_smoke_start(void);
    uec_bool UEC_CALL uec_host_queue_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_queue_smoke_cancel(void);
    uec_result UEC_CALL uec_host_async_save_smoke_start(void);
    uec_bool UEC_CALL uec_host_async_save_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_async_save_smoke_cancel(void);
    uec_result UEC_CALL uec_host_object_load_smoke_start(void);
    uec_bool UEC_CALL uec_host_object_load_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_object_load_smoke_cancel(void);
    uec_result UEC_CALL uec_host_gameplay_example_smoke_start(void);
    uec_bool UEC_CALL uec_host_gameplay_example_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_gameplay_example_smoke_cancel(void);
    uec_result UEC_CALL uec_host_travel_smoke_start(void);
    uec_bool UEC_CALL uec_host_travel_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_travel_smoke_cancel(void);
    uec_result UEC_CALL uec_host_pie_restart_smoke_capture(UWorld* world);
    uec_result UEC_CALL uec_host_pie_restart_smoke_verify(void);
    void UEC_CALL uec_host_pie_restart_smoke_cancel(void);
    uec_result UEC_CALL uec_host_shutdown_pending_smoke_arm(void);
    uec_result UEC_CALL uec_host_shutdown_pending_smoke_prepare(void);
    uec_result UEC_CALL uec_host_shutdown_pending_smoke_verify(void);
    uec_result UEC_CALL uec_host_multi_pie_smoke(void);
    uec_result UEC_CALL uec_host_listen_server_authority_smoke(void);
    uec_result UEC_CALL uec_host_listen_server_impulse_smoke_start(void);
    uec_bool UEC_CALL uec_host_listen_server_impulse_smoke_poll(uec_result* out_result);
    void UEC_CALL uec_host_listen_server_impulse_smoke_cancel(void);
    void UEC_CALL uec_host_dedicated_server_context_smoke_start(void);
}

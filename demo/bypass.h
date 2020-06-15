extern "C" {

extern uint8_t bypass_enable;

void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom);
extern void __cdecl ORIGINAL_cam_render_scene(t2position* pos, double zoom);
extern const uint32_t BYPASS_cam_render_scene;
extern const uint32_t TRAMPOLINE_cam_render_scene;

}

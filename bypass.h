extern "C" {

extern uint8_t bypass_enable;

void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom);
extern void __cdecl ORIGINAL_cam_render_scene(t2position* pos, double zoom);
extern const uint32_t BYPASS_cam_render_scene;
extern const uint32_t TRAMPOLINE_cam_render_scene;

void __stdcall HOOK_cD8Renderer_Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
extern void __stdcall ORIGINAL_cD8Renderer_Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
extern const uint32_t BYPASS_cD8Renderer_Clear;
extern const uint32_t TRAMPOLINE_cD8Renderer_Clear;

void __cdecl HOOK_dark_render_overlays(void);
extern void __cdecl ORIGINAL_dark_render_overlays(void);
extern const uint32_t BYPASS_dark_render_overlays;
extern const uint32_t TRAMPOLINE_dark_render_overlays;

uint32_t ADDR_ObjPosSetLocation;
extern void __cdecl CALL_ObjPosSetLocation(t2id obj, t2location* loc);

}

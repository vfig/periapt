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

void __cdecl HOOK_rendobj_render_object(t2id obj, UCHAR* clut, ULONG fragment);
extern void __cdecl ORIGINAL_rendobj_render_object(t2id obj, UCHAR* clut, ULONG fragment);
extern const uint32_t BYPASS_rendobj_render_object;
extern const uint32_t TRAMPOLINE_rendobj_render_object;

uint32_t ADDR_ObjPosSetLocation;
extern void __cdecl CALL_ObjPosSetLocation(t2id obj, t2location* loc);

void __cdecl HOOK_explore_portals(t2portalcell *cell);
extern void __cdecl ORIGINAL_explore_portals(t2portalcell *cell);
extern const uint32_t BYPASS_explore_portals;
extern const uint32_t TRAMPOLINE_explore_portals;

void __cdecl HOOK_initialize_first_region_clip(int w, int h, t2clipdata *clip);
extern const uint32_t BYPASS_initialize_first_region_clip;
extern const uint32_t TRAMPOLINE_initialize_first_region_clip;
extern uint32_t RESUME_initialize_first_region_clip;

void __cdecl HOOK_mm_hardware_render(t2mmsmodel *m);
extern void __cdecl ORIGINAL_mm_hardware_render(t2mmsmodel *m);
extern const uint32_t BYPASS_mm_hardware_render;
extern const uint32_t TRAMPOLINE_mm_hardware_render;

int __cdecl HOOK_mDrawTriangleLists(IDirect3DDevice9 *device, D3DPRIMITIVETYPE PrimitiveType,
    UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
extern const uint32_t BYPASS_mDrawTriangleLists;
extern const uint32_t TRAMPOLINE_mDrawTriangleLists;
extern uint32_t RESUME_mDrawTriangleLists;

uint32_t ADDR_ComputeCellForLocation;
extern int __cdecl CALL_ComputeCellForLocation(t2location* loc);

int __cdecl HOOK_rendobj_init_frame(void);
extern const uint32_t BYPASS_rendobj_init_frame;
extern const uint32_t TRAMPOLINE_rendobj_init_frame;
extern uint32_t RESUME_rendobj_init_frame;

}

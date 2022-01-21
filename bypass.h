extern "C" {

extern unsigned char bypass_enable;

void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom);
extern void __cdecl ORIGINAL_cam_render_scene(t2position* pos, double zoom);
extern const unsigned int BYPASS_cam_render_scene;
extern const unsigned int TRAMPOLINE_cam_render_scene;

void __stdcall HOOK_cD8Renderer_Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
extern void __stdcall ORIGINAL_cD8Renderer_Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
extern const unsigned int BYPASS_cD8Renderer_Clear;
extern const unsigned int TRAMPOLINE_cD8Renderer_Clear;

void __cdecl HOOK_dark_render_overlays(void);
extern void __cdecl ORIGINAL_dark_render_overlays(void);
extern const unsigned int BYPASS_dark_render_overlays;
extern const unsigned int TRAMPOLINE_dark_render_overlays;

void __cdecl HOOK_rendobj_render_object(t2id obj, UCHAR* clut, ULONG fragment);
extern void __cdecl ORIGINAL_rendobj_render_object(t2id obj, UCHAR* clut, ULONG fragment);
extern const unsigned int BYPASS_rendobj_render_object;
extern const unsigned int TRAMPOLINE_rendobj_render_object;

unsigned int ADDR_ObjPosSetLocation;
extern void __cdecl CALL_ObjPosSetLocation(t2id obj, t2location* loc);

void __cdecl HOOK_explore_portals(t2portalcell *cell);
extern void __cdecl ORIGINAL_explore_portals(t2portalcell *cell);
extern const unsigned int BYPASS_explore_portals;
extern const unsigned int TRAMPOLINE_explore_portals;

void __cdecl HOOK_initialize_first_region_clip(int w, int h, t2clipdata *clip);
extern const unsigned int BYPASS_initialize_first_region_clip;
extern const unsigned int TRAMPOLINE_initialize_first_region_clip;
extern unsigned int RESUME_initialize_first_region_clip;

void __cdecl HOOK_mm_hardware_render(t2mmsmodel *m);
extern void __cdecl ORIGINAL_mm_hardware_render(t2mmsmodel *m);
extern const unsigned int BYPASS_mm_hardware_render;
extern const unsigned int TRAMPOLINE_mm_hardware_render;

int __cdecl HOOK_mDrawTriangleLists(IDirect3DDevice9 *device, D3DPRIMITIVETYPE PrimitiveType,
    UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
extern const unsigned int BYPASS_mDrawTriangleLists;
extern const unsigned int TRAMPOLINE_mDrawTriangleLists;
extern unsigned int RESUME_mDrawTriangleLists;

unsigned int ADDR_ComputeCellForLocation;
extern int __cdecl CALL_ComputeCellForLocation(t2location* loc);

}

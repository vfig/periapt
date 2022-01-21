;
; In target:
; 
; 	TARGET:     REMAINDER:
; 	[ preamble ][ more code ... ]
; 
; 	PATCH (replaces preamble):
; 	[ jmp BYPASS ]
; 
; In bypass module:
; 
; 	TRAMPOLINE:
; 	[ preamble ][ jmp REMAINDER ]
; 
; 	ORIGINAL:
; 	[ swizzle args ][ call TRAMPOLINE ][ cleanup ]
; 
; 	BYPASS:
; 	[ if disabled jmp TRAMPOLINE ][ swizzle args ][ call HOOK ][ cleanup ]
; 
; In C code:
; 
; 	extern void __cdecl ORIGINAL(args);
; 	void __cdecl HOOK(args) { ... ORIGINAL(args) ... }
; 
; To install:
; 
; 	1. Copy preamble from TARGET to TRAMPOLINE
; 	2. Patch TRAMPOLINE with address of REMAINDER
; 	3. Apply PATCH to TARGET
; 
; To uninstall:
; 
; 	1. Copy preamble from TRAMPOLINE to TARGET
;

	.model	FLAT, C

	.data

	public bypass_enable
	public RESUME_initialize_first_region_clip
	public RESUME_mDrawTriangleLists

bypass_enable LABEL BYTE
	DB 00h

RESUME_initialize_first_region_clip LABEL DWORD
	DB 4 DUP(0CCh)				; resume address

RESUME_mDrawTriangleLists LABEL DWORD
	DB 4 DUP(0CCh)				; resume address


; ------------------------------------------------------------------------

	.code

; ------------------------------------------------------------------------

; void cam_render_scene(pos, zoom)		; Custom convention, caller cleanup:
;	t2position* pos;			; EAX
;	double zoom;				; [ESP+04]
;						; Void return.

	extern HOOK_cam_render_scene: PTR
	public BYPASS_cam_render_scene
	public ORIGINAL_cam_render_scene
	public TRAMPOLINE_cam_render_scene

TRAMPOLINE_cam_render_scene:
	nop					; preamble
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					; jmp REMAINDER
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.

ORIGINAL_cam_render_scene:
	mov	eax, dword ptr [esp+4]		; swizzle args to custom convention
	fld	qword ptr [esp+8]		;	.
	sub	sp, 8				;	.
	fst	qword ptr [esp+0]		;	.
	call	TRAMPOLINE_cam_render_scene	; call TRAMPOLINE
	add	sp, 8				; cleanup
	ret					;	.

BYPASS_cam_render_scene:
	test	byte ptr [bypass_enable], 0FFh	; if disabled, jmp TRAMPOLINE
	jz	TRAMPOLINE_cam_render_scene	;	.
	fld	qword ptr [esp+4]		; swizzle args to __cdecl convention
	sub	sp, 8				;	.
	fst	qword ptr [esp+0]		;	.
	push	eax				;	.
	call	HOOK_cam_render_scene		; call HOOK
	add	sp, 12				; cleanup
	ret					;	.

; ------------------------------------------------------------------------

; void cD8Renderer::Clear			; __stdcall
;	DWORD Count,				; [ESP+04]
;	CONST D3DRECT* pRects,			; [ESP+08]
;	DWORD Flags,				; [ESP+0C]
;	D3DCOLOR Color,				; [ESP+10]
;	float Z,				; [ESP+14]
;	DWORD Stencil				; [ESP+18]
;						; Void return.

	extern HOOK_cD8Renderer_Clear@24: DWORD
	public BYPASS_cD8Renderer_Clear
	public ORIGINAL_cD8Renderer_Clear@24
	public TRAMPOLINE_cD8Renderer_Clear

TRAMPOLINE_cD8Renderer_Clear:
	nop					; preamble
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					; jmp REMAINDER
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.

ORIGINAL_cD8Renderer_Clear@24:
	jmp	TRAMPOLINE_cD8Renderer_Clear	; call TRAMPOLINE

BYPASS_cD8Renderer_Clear:
	test	byte ptr [bypass_enable], 0FFh	; if disabled, jmp TRAMPOLINE
	jz	TRAMPOLINE_cD8Renderer_Clear	;	.
	jmp	HOOK_cD8Renderer_Clear@24	; call HOOK

; ------------------------------------------------------------------------

; void dark_render_overlays(void)		; __cdecl
;						; Void return.

	extern HOOK_dark_render_overlays: DWORD
	public BYPASS_dark_render_overlays
	public ORIGINAL_dark_render_overlays
	public TRAMPOLINE_dark_render_overlays

TRAMPOLINE_dark_render_overlays:
	nop					; preamble
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					; jmp REMAINDER
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.

ORIGINAL_dark_render_overlays:
	jmp	TRAMPOLINE_dark_render_overlays	; call TRAMPOLINE

BYPASS_dark_render_overlays:
	test	byte ptr [bypass_enable], 0FFh		; if disabled, jmp TRAMPOLINE
	jz	TRAMPOLINE_dark_render_overlays	;	.
	jmp	HOOK_dark_render_overlays		; call HOOK


; ------------------------------------------------------------------------

; void rendobj_render_object(obj,clut,fragment)	; __cdecl
;	t2id obj;				; Object to render.
;	uchar* clut;				; Color LUT; may be null.
;	ulong fragment;				; Object fragment; should always be 0 with HW renderer.
;						; Void return.

	extern HOOK_rendobj_render_object: DWORD
	public BYPASS_rendobj_render_object
	public ORIGINAL_rendobj_render_object
	public TRAMPOLINE_rendobj_render_object

TRAMPOLINE_rendobj_render_object:
	nop					; preamble
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					; jmp REMAINDER
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.

ORIGINAL_rendobj_render_object:
	jmp	TRAMPOLINE_rendobj_render_object	; call TRAMPOLINE

BYPASS_rendobj_render_object:
	test	byte ptr [bypass_enable], 0FFh		; if disabled, jmp TRAMPOLINE
	jz	TRAMPOLINE_rendobj_render_object	;	.
	jmp	HOOK_rendobj_render_object		; call HOOK


; ------------------------------------------------------------------------

; void ObjPosSetLocation(obj, loc)		; Custom convention, caller cleanup:
;	t2id obj;				; EDI
;	t2location* loc;			; ESI
;						; Void return.

	extern ADDR_ObjPosSetLocation: DWORD
	public CALL_ObjPosSetLocation

CALL_ObjPosSetLocation:
	push	esi				; swizzle args to custom convention
	push	edi				;	.
	mov	edi, dword ptr [esp+0ch]	;	.
	mov	esi, dword ptr [esp+10h]	; 	.
	call	dword ptr [ADDR_ObjPosSetLocation]	; call original
	pop	edi				;	.
	pop	esi				;	,
	ret					;	.

; ------------------------------------------------------------------------

; void explore_portals(cell)			; Custom convention, caller cleanup:
;	t2portalcell* cell;			; ESI
;						; Void return.

	extern HOOK_explore_portals: DWORD
	public BYPASS_explore_portals
	public ORIGINAL_explore_portals
	public TRAMPOLINE_explore_portals

TRAMPOLINE_explore_portals:
	nop					; preamble
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					; jmp REMAINDER
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.

ORIGINAL_explore_portals:
	mov	esi, dword ptr [esp+4]		; swizzle args to custom convention
	call	TRAMPOLINE_explore_portals	; call TRAMPOLINE
	ret					;	.

BYPASS_explore_portals:
	test	byte ptr [bypass_enable], 0FFh	; if disabled, jmp TRAMPOLINE
	jz	TRAMPOLINE_explore_portals	;	.
	push	esi				; swizzle args to __cdecl convention
	call	HOOK_explore_portals		; call HOOK
	add	sp, 4				; cleanup
	ret					;	.


; ------------------------------------------------------------------------

; in initialize_first_region_clip...		; in media res:
;	int w;					; ECX
;	int h;					; EBP
;	t2clipdata *clip;			; EAX

	extern HOOK_initialize_first_region_clip: DWORD
	public BYPASS_initialize_first_region_clip
	public TRAMPOLINE_initialize_first_region_clip

TRAMPOLINE_initialize_first_region_clip:
	nop					; preamble
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					; jmp REMAINDER
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.

BYPASS_initialize_first_region_clip:
	test	byte ptr [bypass_enable], 0FFh	; if disabled, jmp TRAMPOLINE
	jz	TRAMPOLINE_initialize_first_region_clip	;	.
	push	eax				; preserve registers
	push	ecx				;	.
	push	edx				;	.
	push	eax				; param 2: t2clipdata*
	push	ebp				; param 1: height
	push	ecx				; param 0: width
	call	HOOK_initialize_first_region_clip ; call HOOK
	add	sp, 12				; cleanup
	pop	edx				; restore registers
	pop	ecx				;	.
	pop	eax				;	.
	jmp	dword ptr [RESUME_initialize_first_region_clip]


; ------------------------------------------------------------------------

; void mm_hardware_render(m)			; __cdecl
;	t2mmsmodel *m;				; Mesh to render.
;						; Void return.

	extern HOOK_mm_hardware_render: DWORD
	public BYPASS_mm_hardware_render
	public ORIGINAL_mm_hardware_render
	public TRAMPOLINE_mm_hardware_render

TRAMPOLINE_mm_hardware_render:
	nop					; preamble
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					; jmp REMAINDER
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.

ORIGINAL_mm_hardware_render:
	jmp	TRAMPOLINE_mm_hardware_render	; call TRAMPOLINE

BYPASS_mm_hardware_render:
	test	byte ptr [bypass_enable], 0FFh	; if disabled, jmp TRAMPOLINE
	jz	TRAMPOLINE_mm_hardware_render	;	.
	jmp	HOOK_mm_hardware_render	; call HOOK


; ------------------------------------------------------------------------

; in mDrawTriangleLists				; in media res:
;						; params to DrawPrimitiveUp()
;						; have just been pushed onto
;						; the stack.

	extern HOOK_mDrawTriangleLists: DWORD
	public BYPASS_mDrawTriangleLists
	public TRAMPOLINE_mDrawTriangleLists

TRAMPOLINE_mDrawTriangleLists:
	nop					; preamble
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.
	nop					; jmp REMAINDER
	nop					;	.
	nop					;	.
	nop					;	.
	nop					;	.

BYPASS_mDrawTriangleLists:
	test	byte ptr [bypass_enable], 0FFh	; if disabled, jmp TRAMPOLINE
	jz	TRAMPOLINE_mDrawTriangleLists	;	.
						; params are on stack already
	call	HOOK_mDrawTriangleLists	; call HOOK
	add	sp, 20				; cleanup
	jmp	dword ptr [RESUME_mDrawTriangleLists]


; ------------------------------------------------------------------------

; int ComputeCellForLocation(loc)		; Custom convention, caller cleanup:
;	t2location* loc;			; EAX
;						; EAX return.

	extern ADDR_ComputeCellForLocation: DWORD
	public CALL_ComputeCellForLocation

CALL_ComputeCellForLocation:
	mov	eax, dword ptr [esp+04h]	; swizzle args to custom convention
	call	dword ptr [ADDR_ComputeCellForLocation]	; call original
	ret					;	.


	END
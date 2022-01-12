/*

In target:

	TARGET:     REMAINDER:
	[ preamble ][ more code ... ]

	PATCH (replaces preamble):
	[ jmp BYPASS ]

In bypass module:

	TRAMPOLINE:
	[ preamble ][ jmp REMAINDER ]

	ORIGINAL:
	[ swizzle args ][ call TRAMPOLINE ][ cleanup ]

	BYPASS:
	[ if disabled jmp TRAMPOLINE ][ swizzle args ][ call HOOK ][ cleanup ]

In C code:

	extern void __cdecl ORIGINAL(args);
	void __cdecl HOOK(args) { ... ORIGINAL(args) ... }

To install:

	1. Copy preamble from TARGET to TRAMPOLINE
	2. Patch TRAMPOLINE with address of REMAINDER
	3. Apply PATCH to TARGET

To uninstall:

	1. Copy preamble from TRAMPOLINE to TARGET

*/

	.intel_syntax noprefix

	.data

	.global _bypass_enable
	.global _RESUME_initialize_first_region_clip
	.global _RESUME_mDrawTriangleLists

_bypass_enable:
	.byte 0x00

_RESUME_initialize_first_region_clip:
	.space	4, 0x00				# resume address

_RESUME_mDrawTriangleLists:
	.space	4, 0x00				# skip address


/* ------------------------------------------------------------------------*/

	.text

/* ------------------------------------------------------------------------*/

# void cam_render_scene(pos, zoom)		# Custom convention, caller cleanup:
#	t2position* pos;			# EAX
#	double zoom;				# [ESP+04]
#						# Void return.

	.extern _HOOK_cam_render_scene
	.global _BYPASS_cam_render_scene
	.global _ORIGINAL_cam_render_scene
	.global _TRAMPOLINE_cam_render_scene

_TRAMPOLINE_cam_render_scene:
	.space	9, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_ORIGINAL_cam_render_scene:
	mov	eax, dword ptr [esp+4]		# swizzle args to custom convention
	fld	qword ptr [esp+8]		#	.
	sub	sp, 8				#	.
	fst	qword ptr [esp+0]		#	.
	call	_TRAMPOLINE_cam_render_scene	# call TRAMPOLINE
	add	sp, 8				# cleanup
	ret					#	.

_BYPASS_cam_render_scene:
	test	byte ptr [_bypass_enable], 0xff	# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_cam_render_scene	#	.
	fld	qword ptr [esp+4]		# swizzle args to __cdecl convention
	sub	sp, 8				#	.
	fst	qword ptr [esp+0]		#	.
	push	eax				#	.
	call	_HOOK_cam_render_scene		# call HOOK
	add	sp, 12				# cleanup
	ret					#	.

/* ------------------------------------------------------------------------*/

# void cD8Renderer::Clear			# __stdcall
#	DWORD Count,				# [ESP+04]
#	CONST D3DRECT* pRects,			# [ESP+08]
#	DWORD Flags,				# [ESP+0C]
#	D3DCOLOR Color,				# [ESP+10]
#	float Z,				# [ESP+14]
#	DWORD Stencil				# [ESP+18]
#						# Void return.

	.extern _HOOK_cD8Renderer_Clear@24
	.global _BYPASS_cD8Renderer_Clear
	.global _ORIGINAL_cD8Renderer_Clear@24
	.global _TRAMPOLINE_cD8Renderer_Clear

_TRAMPOLINE_cD8Renderer_Clear:
	.space	6, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_ORIGINAL_cD8Renderer_Clear@24:
	jmp	_TRAMPOLINE_cD8Renderer_Clear	# call TRAMPOLINE

_BYPASS_cD8Renderer_Clear:
	test	byte ptr [_bypass_enable], 0xff	# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_cD8Renderer_Clear	#	.
	jmp	_HOOK_cD8Renderer_Clear@24	# call HOOK

/* ------------------------------------------------------------------------*/

# void dark_render_overlays(void)		# __cdecl
#						# Void return.

	.extern _HOOK_dark_render_overlays
	.global _BYPASS_dark_render_overlays
	.global _ORIGINAL_dark_render_overlays
	.global _TRAMPOLINE_dark_render_overlays

_TRAMPOLINE_dark_render_overlays:
	.space	6, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_ORIGINAL_dark_render_overlays:
	jmp	_TRAMPOLINE_dark_render_overlays	# call TRAMPOLINE

_BYPASS_dark_render_overlays:
	test	byte ptr [_bypass_enable], 0xff		# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_dark_render_overlays	#	.
	jmp	_HOOK_dark_render_overlays		# call HOOK


/* ------------------------------------------------------------------------*/

# void rendobj_render_object(obj,clut,fragment)	# __cdecl
#	t2id obj;				# Object to render.
#	uchar* clut;				# Color LUT; may be null.
#	ulong fragment;				# Object fragment; should always be 0 with HW renderer.
#						# Void return.

	.extern _HOOK_rendobj_render_object
	.global _BYPASS_rendobj_render_object
	.global _ORIGINAL_rendobj_render_object
	.global _TRAMPOLINE_rendobj_render_object

_TRAMPOLINE_rendobj_render_object:
	.space	6, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_ORIGINAL_rendobj_render_object:
	jmp	_TRAMPOLINE_rendobj_render_object	# call TRAMPOLINE

_BYPASS_rendobj_render_object:
	test	byte ptr [_bypass_enable], 0xff		# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_rendobj_render_object	#	.
	jmp	_HOOK_rendobj_render_object		# call HOOK


/* ------------------------------------------------------------------------*/

# void ObjPosSetLocation(obj, loc)		# Custom convention, caller cleanup:
#	t2id obj;				# EDI
#	t2location* loc;			# ESI
#						# Void return.

	.extern _ADDR_ObjPosSetLocation
	.global _CALL_ObjPosSetLocation

_CALL_ObjPosSetLocation:
	push	esi				# swizzle args to custom convention
	push	edi				#	.
	mov	edi, dword ptr [esp+0x0c]	#	.
	mov	esi, dword ptr [esp+0x10]	# 	.
	call	dword ptr [_ADDR_ObjPosSetLocation]	# call original
	pop	edi				#	.
	pop	esi				#	,
	ret					#	.

/* ------------------------------------------------------------------------*/

# void explore_portals(cell)			# Custom convention, caller cleanup:
#	t2portalcell* cell;			# ESI
#						# Void return.

	.extern _HOOK_explore_portals
	.global _BYPASS_explore_portals
	.global _ORIGINAL_explore_portals
	.global _TRAMPOLINE_explore_portals

_TRAMPOLINE_explore_portals:
	.space	6, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_ORIGINAL_explore_portals:
	mov	esi, dword ptr [esp+4]		# swizzle args to custom convention
	call	_TRAMPOLINE_explore_portals	# call TRAMPOLINE
	ret					#	.

_BYPASS_explore_portals:
	test	byte ptr [_bypass_enable], 0xff	# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_explore_portals	#	.
	push	esi				# swizzle args to __cdecl convention
	call	_HOOK_explore_portals		# call HOOK
	add	sp, 4				# cleanup
	ret					#	.


/* ------------------------------------------------------------------------*/

# in initialize_first_region_clip...		# in media res:
#	int w;					# ECX
#	int h;					# EBP
#	t2clipdata *clip;			# EAX

	.extern _HOOK_initialize_first_region_clip
	.global _BYPASS_initialize_first_region_clip
	.global _TRAMPOLINE_initialize_first_region_clip

_TRAMPOLINE_initialize_first_region_clip:
	.space	5, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_BYPASS_initialize_first_region_clip:
	test	byte ptr [_bypass_enable], 0xff	# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_initialize_first_region_clip	#	.
	push	eax				# preserve registers
	push	ecx				#	.
	push	edx				#	.
	push	eax				# param 2: t2clipdata*
	push	ebp				# param 1: height
	push	ecx				# param 0: width
	call	_HOOK_initialize_first_region_clip # call HOOK
	add	sp, 12				# cleanup
	pop	edx				# restore registers
	pop	ecx				#	.
	pop	eax				#	.
	jmp	dword ptr [_RESUME_initialize_first_region_clip]


/* ------------------------------------------------------------------------*/

# void mm_setup_material(index)			# Custom convention, caller cleanup:
#	int index;				# EDI
#						# Void return.

	.extern _HOOK_mm_setup_material
	.global _BYPASS_mm_setup_material
	.global _ORIGINAL_mm_setup_material
	.global _TRAMPOLINE_mm_setup_material

_TRAMPOLINE_mm_setup_material:
	.space	7, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_ORIGINAL_mm_setup_material:
	mov	edi, dword ptr [esp+4]		# swizzle args to custom convention
	call	_TRAMPOLINE_mm_setup_material	# call TRAMPOLINE
	ret					#	.

_BYPASS_mm_setup_material:
	test	byte ptr [_bypass_enable], 0xff	# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_mm_setup_material	#	.
	push	edi				#	.
	call	_HOOK_mm_setup_material		# call HOOK
	add	sp, 4				# cleanup
	ret					#	.


/* ------------------------------------------------------------------------*/

# void mm_hardware_render(m)			# __cdecl
#	t2mmsmodel *m;				# Mesh to render.
#						# Void return.

	.extern _HOOK_mm_hardware_render
	.global _BYPASS_mm_hardware_render
	.global _ORIGINAL_mm_hardware_render
	.global _TRAMPOLINE_mm_hardware_render

_TRAMPOLINE_mm_hardware_render:
	.space	8, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_ORIGINAL_mm_hardware_render:
	jmp	_TRAMPOLINE_mm_hardware_render	# call TRAMPOLINE

_BYPASS_mm_hardware_render:
	test	byte ptr [_bypass_enable], 0xff	# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_mm_hardware_render	#	.
	jmp	_HOOK_mm_hardware_render	# call HOOK


/* ------------------------------------------------------------------------*/
/*
# void mDrawTriangleLists(unknown)		# Custom convention, caller cleanup:
#	void *unknown;				# ESI
#						# Void return.

	.extern _HOOK_mDrawTriangleLists
	.global _BYPASS_mDrawTriangleLists
	.global _ORIGINAL_mDrawTriangleLists
	.global _TRAMPOLINE_mDrawTriangleLists

_TRAMPOLINE_mDrawTriangleLists:
	.space	7, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_ORIGINAL_mDrawTriangleLists:
	push	esi
	mov	esi, dword ptr [esp+8]		# swizzle args to custom convention
	call	_TRAMPOLINE_mDrawTriangleLists	# call TRAMPOLINE
	pop	esi
	ret					#	.

_BYPASS_mDrawTriangleLists:
	test	byte ptr [_bypass_enable], 0xff	# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_mDrawTriangleLists	#	.
	push	eax				# preserve registers
	push	ecx				#	.
	push	edx				#	.
	push	esi				# param 0
	call	_HOOK_mDrawTriangleLists	# call HOOK
	add	sp, 4				# cleanup
	pop	edx				# restore registers
	pop	ecx				#	.
	pop	eax				#	.
	ret					#	.
*/

/* ------------------------------------------------------------------------*/

# in mDrawTriangleLists				# in media res:
#	int *info;				# EDI (info[2] is element count)
#	void *

	.extern _HOOK_mDrawTriangleLists
	.global _BYPASS_mDrawTriangleLists
	.global _TRAMPOLINE_mDrawTriangleLists

_TRAMPOLINE_mDrawTriangleLists:
	.space	6, 0x90				# preamble
	.space	5, 0x90				# jmp REMAINDER

_BYPASS_mDrawTriangleLists:
	test	byte ptr [_bypass_enable], 0xff	# if disabled, jmp TRAMPOLINE
	jz	_TRAMPOLINE_mDrawTriangleLists	#	.
						# params are on stack already
	call	_HOOK_mDrawTriangleLists	# call HOOK
	add	sp, 20				# cleanup
	jmp	dword ptr [_RESUME_mDrawTriangleLists]

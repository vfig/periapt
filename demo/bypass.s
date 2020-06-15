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

_bypass_enable:
	.byte 0x00

/* ------------------------------------------------------------------------*/

	.text

/* ------------------------------------------------------------------------*/

# void cam_render_scene(pos, zoom)		# Custom convention, caller cleanup:
#	t2position* pos;			# <- in EAX
#	double zoom;				# <- on stack
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

/*
Unpatched:

_cam_render_scene:
	<original code>

Patched:

_cam_render_scene:
	JMP _TRAM_cam_render_scene
_cam_render_scene+9:
	# Continuation point

_PREF_cam_render_scene:
	# 9 bytes copied from unpatched _cam_render_scene
	#
	JMP _cam_render_scene+9

*/

	.intel_syntax noprefix

	.text

	.global _ORIG_cam_render_scene
	.global _PREF_cam_render_scene
	.global _CONT_cam_render_scene
	.global _TRAM_cam_render_scene
	.extern _HOOK_cam_render_scene

	# extern void ORIG_cam_render_scene (void* pos, double zoom);
	# extern void* PREF_cam_render_scene;
	# extern void* CONT_cam_render_scene;
	# extern void* TRAM_cam_render_scene;
	# void HOOK_cam_render_scene(void* pos, double zoom);

/*
Incoming:
	ESP +	0	4	8	12
		[rtn] 	Pos*	double...
Outgoing:
	EAX	Pos*
	ESP +	0	4	8
		double...	[rtn] etc...
*/

_ORIG_cam_render_scene:			# _cdecl thunk
	mov	eax, dword ptr [esp+4]	# mov eax, pos
	fld	qword ptr [esp+8]	# push zoom
	sub	sp, 8			#	.
	fst	qword ptr [esp+0]	#	.
	call	_PREF_cam_render_scene	# call prefix
	add	sp, 8			# cleanup
	ret
_PREF_cam_render_scene:			# Prefix: to be patched with 9 bytes (first two
	.byte	0x90,0x90,0x90,0x90	# instructions) from _cam_render_scene.
	.byte	0x90,0x90,0x90,0x90	#	.
	.byte	0x90			#	.
	.byte	0x90,0x90,0x90,0x90	# Continue: to be patched with
	.byte	0x90			# 	JMP _cam_render_scene+9

/*
Incoming:
	EAX	Pos*
	ESP +	0	4	8
		[rtn]	double...
Outgoing:
	ESP +	0	4	8	12
		Pos*	double...	[rtn] etc...
*/
_TRAM_cam_render_scene:			# JMP here from patched _cam_render_scene.
	fld	qword ptr [esp+4]	# push zoom
	sub	sp, 8			#	.
	fst	qword ptr [esp+0]	#	.
	push	eax			# push pos
	call	_HOOK_cam_render_scene	# call C hook
	add	sp, 12			# cleanup
	ret

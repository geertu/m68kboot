; cpxstart.asm
;=============================================================================
; Startup file for CPXs
;
; 'Save_vars' is the area to store your default variables.
; Add more dc.w's as necessary
;
; Assemble with Mad Mac
;
; For Mark Williams - Use  'cpx_init_'
; For Alcyon        - Use  '_cpx_init'
;
;  
;


export cpxstart
import cpx_init

	text

cpxstart:
	jmp cpx_init



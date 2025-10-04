; ***********************************************************
;
; file: blink.asm
; target: PIC16f684 on PICKit 1
; author: David Henry
; licence: This program is licensed under the terms of the
;          MIT licence.  You can get a copy of it at:
;     http://www.opensource.org/licenses/mit-license.php
;
; This program makes blinking alternatively two LEDs (D0
; and D1).
;
; ***********************************************************

	; use PIC 16F684
	list		p=16f684
	#include	<p16f684.inc>

	; set configuration word.
	__CONFIG	_FCMEN_OFF & _IESO_OFF & _BOD_OFF & _CPD_OFF & _CP_OFF & _MCLRE_OFF & _PWRTE_OFF & _WDT_OFF & _INTRC_OSC_NOCLKOUT

	; -------------------------------------------------------
	; global variable declarations.
	; -------------------------------------------------------

count1	equ	0x20
count2	equ	0x21
temp	equ	0x22

	; -------------------------------------------------------
	; reset vector.  this is where the PCL is set after
	; power on and reset.
	; -------------------------------------------------------

	org	0x00
	goto	main

wait
	; -------------------------------------------------------
	; wait function.  perform a little tempo.
	; -------------------------------------------------------

	nop			; consume one cycle

	decfsz	count1,f	; decrement first counter
	goto	wait		;  until reaching zero

	movlw	0xff		; reload first counter
	movwf	count1

	decfsz	count2,f	; decrement second counter
	goto	wait		;  until reaching zero

	movlw	0xff		; reload second counter
	movwf	count2

	return

main
	; -------------------------------------------------------
	; program's main entry point.
	; -------------------------------------------------------

	; init wait counters
	movlw	0xff
	movwf	count1
	movwf	count2

	; init PortA
	bsf	STATUS,RP0	; enter bank 1
	movlw	B'11001110'
	movwf	TRISA		; configure I/O

	bcf	STATUS,RP0	; enter bank 0
	movlw	B'00010000'	; setup PortA's output mask
	movwf	temp		; save it to temp

loop
	; -------------------------------------------------------
	; program's main loop.
	; -------------------------------------------------------

	movf	temp,w		; load PortA's output mask

	movwf	PORTA
	xorlw	B'00110000'	; light on alternatively LEDs #0 and #1

	movwf	temp		; save the modified mask

	call	wait		; burn some cycles before switching LEDs
	goto	loop		; repeat forever

	end

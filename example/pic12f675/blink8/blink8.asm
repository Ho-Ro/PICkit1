; ***********************************************************
;
; file: blink8.asm
; target: PIC12f675 on PICKit 1
; author: David Henry
; licence: This program is licensed under the terms of the
;          MIT licence.  You can get a copy of it at:
;     http://www.opensource.org/licenses/mit-license.php
;
; This program makes blinking alternatively height LEDs (D0
; to D7).
;
; ***********************************************************

	; use PIC 12F675
	list		p=12f675
	#include	<p12f675.inc>

	; set configuration word.
	__CONFIG	_CPD_OFF & _CP_OFF & _BODEN_OFF & _MCLRE_OFF & _WDT_OFF & _PWRTE_ON & _INTRC_OSC_NOCLKOUT

	; hide warning message 302:
	; "Register in operand not in bank 0. Ensure bank bits are correct."
	errorlevel	-302

	; -------------------------------------------------------
	; global variable declarations.
	; -------------------------------------------------------

count1	equ	0x20
count2	equ	0x21
ledn	equ	0x22
index	equ	0x23

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

config_led
	; -------------------------------------------------------
	; LED configuration.  return into W the TRISIO
	; configuration for lighting the proper LED.  before
	; calling this function, W must contain the LED number
	; (from 0 to 7) we want to light on.
	; we use this number as an offset into a table.
	; -------------------------------------------------------

	addwf	PCL,f
	retlw	B'11001110'	; xx00x11x
	retlw	B'11001110'
	retlw	B'11101010'	; xx10x01x
	retlw	B'11101010'
	retlw	B'11011010'	; xx01x01x
	retlw	B'11011010'
	retlw	B'11111000'	; xx11x00x
	retlw	B'11111000'

power_led
	; -------------------------------------------------------
	; LED powering on.  return into W the GPIO mask for
	; powering on the proper LED.  before calling this
	; function, W must contain the LED number (from 0 to 7)
	; we want to light on.
	; we use this number as an offset into a table.
	; -------------------------------------------------------

	addwf	PCL,f		; GP   5 4 2 1
	retlw	B'00010000'	; D0 = 0 1 Z Z
	retlw	B'00100000'	; D1 = 1 0 Z Z
	retlw	B'00010000'	; D2 = Z 1 0 Z
	retlw	B'00000100'	; D3 = Z 0 1 Z
	retlw	B'00100000'	; D4 = 1 Z 0 Z
	retlw	B'00000100'	; D5 = 0 Z 1 Z
	retlw	B'00000100'	; D6 = Z Z 1 0
	retlw	B'00000010'	; D7 = Z Z 0 1

led_on
	; -------------------------------------------------------
	; light on a LED.  W contains the LED number we want
	; to enable.
	; -------------------------------------------------------

	movwf	ledn		; save LED number to file

	bsf	STATUS,RP0	; enter bank 1
	call	config_led	; get TRISIO value, put it into W
	movwf	TRISIO		; configure I/O

	movf	ledn,w		; restore LED number

	bcf	STATUS,RP0	; enter bank 0
	call	power_led	; get GPIO value, put it into W
	movwf	GPIO		; power on the desired LED
	return

main
	; -------------------------------------------------------
	; program's main entry point.
	; -------------------------------------------------------

	; init wait counters
	bcf	STATUS,RP0	; enter bank 0
	movlw	0xff
	movwf	count1
	movwf	count2

	; init LED index
	clrf	index

loop
	; -------------------------------------------------------
	; program's main loop.
	; -------------------------------------------------------

	movf	index,w		; light on the LED number 'index'
	call	led_on
	call	wait		; burn some cycles before switching LEDs

	; next LED
	incf	index,1
	btfsc	index,3
	clrf	index

	goto	loop		; repeat forever

	end

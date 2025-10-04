; ***********************************************************
;
; file: switch8.asm
; target: PIC16f684 on PICKit 1
; author: David Henry
; licence: This program is licensed under the terms of the
;          MIT licence.  You can get a copy of it at:
;     http://www.opensource.org/licenses/mit-license.php
;
; This program makes lighting a LED (D0 to D7).  User can
; switch the LEDs by pressing the button connected to RA3.
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

count1	equ	0x7c
count2	equ	0x7d
ledn	equ	0x7e
index	equ	0x7f

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
	; LED configuration.  return into W the TRISA
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
	; LED powering on.  return into W the PORTA mask for
	; powering on the proper LED.  before calling this
	; function, W must contain the LED number (from 0 to 7)
	; we want to light on.
	; we use this number as an offset into a table.
	; -------------------------------------------------------

	addwf	PCL,f		; RA   5 4 2 1
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
	call	config_led	; get TRISA value, put it into W
	movwf	TRISA		; configure I/O

	movf	ledn,w		; restore LED number

	bcf	STATUS,RP0	; enter bank 0
	call	power_led	; get PORTA value, put it into W
	movwf	PORTA		; power on the desired LED
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
	btfss	PORTA,3
	incf	index,1
	btfsc	index,3
	clrf	index

	goto	loop		; repeat forever

	end

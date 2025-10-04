; ***********************************************************
;
; file: timer8.asm
; target: PIC12f675 on PICKit 1
; author: David Henry
; licence: This program is licensed under the terms of the
;          MIT licence.  You can get a copy of it at:
;     http://www.opensource.org/licenses/mit-license.php
;
; This program makes lighting LEDs D0 to D7 one by one.
; 16-bit Timer1 is used for waiting before switching to the
; next LED.
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

ledn		equ	0x20
index		equ	0x21

status_temp	equ 	0x22
w_temp		equ	0x23

	; -------------------------------------------------------
	; reset vector.  this is where the PCL is set after
	; power on and reset.
	; -------------------------------------------------------

	org	0x00
	goto	main

	; -------------------------------------------------------
	; interrupt vector.  this is where the PCL is set after
	; an interruption occured.
	; -------------------------------------------------------

	org	0x04
	movwf	w_temp
	swapf	STATUS,W
	clrf	STATUS		; (enter bank 0)
	movwf	status_temp

	btfsc	PIR1,0		; test if timer1 overflowed (TMR1IF)
	goto	tmr1_isr

	goto	end_isr

tmr1_isr
	; -------------------------------------------------------
	; timer1 interruption service request.
	; -------------------------------------------------------

	incf	index,1		; increment LED number
	btfsc	index,3		; if index = 8,
	clrf	index		;  index := 0

	clrf	PIR1		; clear interrupt flag

	movf	index,w		; light on the LED number 'index'
	call	led_on

end_isr
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,f
	swapf	w_temp,w
	retfie

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

	bcf	STATUS,RP0	; enter bank 0
	movwf	ledn		; save LED number to file

	call	config_led	; get TRISIO value, put it into W
	bsf	STATUS,RP0	; enter bank 1
	movwf	TRISIO		; configure I/O

	bcf	STATUS,RP0	; enter bank 0
	movf	ledn,w		; restore LED number

	call	power_led	; get GPIO value, put it into W
	bcf	STATUS,RP0	; enter bank 0
	movwf	GPIO		; power on the desired LED
	return

main
	; -------------------------------------------------------
	; program's main entry point.
	; -------------------------------------------------------

	; init LED index
	movlw	0xff		; first time 'index' will be
	movwf	index		;  incremented, it will become 0x00

	; init 16-bit timer
	bcf	STATUS,RP0	; enter bank 0
	clrf	TMR1L		; reset timer1 register pair
	clrf	TMR1H

	; enable timer interrupts
	movlw	0xc0		; GIE, PIE
	movwf	INTCON

	bsf	STATUS,RP0	; enter bank 1
	movlw	0x01		; TMR1IE
	movwf	PIE1

	; enable timer1
	bcf	STATUS,RP0	; enter bank 0
	movlw	0x21		; 1:4 prescale value; enable timer1
	movwf	T1CON		;  overflow interrupt

loop
	; -------------------------------------------------------
	; program's main loop.
	; -------------------------------------------------------

	goto	loop		; repeat forever

	end

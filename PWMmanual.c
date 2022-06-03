/* 
 * File:   Maestro.c
 * Author: Pablo Caal & Jorge Cerón
 * 
 * Created on 24 de mayo de 2022, 06:00 PM
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT    // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF               // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF              // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF              // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF                 // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF                // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF              // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF               // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF              // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF                // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V           // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF                // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000      // Frecuencia de oscilador en 1 MHz
#define IN_MIN 0                
#define IN_MAX 255              // Valores de entrada a Potenciometro
#define OUT_MIN1 40             
#define OUT_MAX1 160

uint8_t POT_3;                      
unsigned short contador_tmr0 = 0;          
uint8_t anchodepulso;

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);
void setup(void);
unsigned short map(short val, uint8_t in_min, uint8_t in_max, 
            unsigned short out_min, unsigned short out_max);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.ADIF){                  // Verificación de interrupción del módulo ADC
        if(ADCON0bits.CHS == 0){   // Verificación de canal AN2
            POT_3 = ADRESH;             // Almacenar el resgitro ADRESH en variable POT3
            anchodepulso = map(POT_3, IN_MIN, IN_MAX, OUT_MIN1, OUT_MAX1);
        }
        PIR1bits.ADIF = 0;              // Limpieza de bandera de interrupción
    }
    
    if (INTCONbits.T0IF){
        contador_tmr0++;
        if(contador_tmr0 < anchodepulso){
            PORTAbits.RA6 = 1;
        }
        else if(contador_tmr0 > anchodepulso){
            PORTAbits.RA6 = 0;
        }
        else if(contador_tmr0 > 267){
            contador_tmr0 = 0;
        }
        INTCONbits.T0IF = 0;            // Limpieza de bandera de interrupción TMR0
        TMR0 = 254;                     // Reatrdo de 0.015 ms
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){        
        if (ADCON0bits.GO == 0){
            __delay_us(40);
            ADCON0bits.GO = 1;          // On controlado con tiempo de TMR0
        }  
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){       
    // Configuración del oscilador interno
    OSCCONbits.IRCF = 0b0100;       // 1MHz
    OSCCONbits.SCS = 1;             // Reloj interno
    
    // Configuración de puertos
    ANSEL = 0b00000001;             // AN0 como analógica
    ANSELH = 0b00000000;            // I/O digitales
    TRISA = 0b00000001;             // AN0 como entrada
    PORTA = 0b00000000;             // Limpieza del PORTA
    
    // Configuración de interrucpiones
    PIR1bits.ADIF = 0;              // Limpiamos bandera de ADC
    PIE1bits.ADIE = 1;              // Habilitamos interrupcion de ADC
    INTCONbits.PEIE = 1;            // Habilitamos interrupciones de perifericos
    INTCONbits.GIE = 1;             // Habilitamos interrupciones globales
    INTCONbits.T0IE = 1;            // Habilitamos interrupcion de TMR0
    INTCONbits.T0IF = 0;            // Limpiamos bandera de TMR0
    // Configuracion TMR0
    OPTION_REGbits.T0CS = 0;        // Temporizador
    OPTION_REGbits.PSA = 0;         // Prescaler a TMR0
    OPTION_REGbits.PS= 0b000;       // 1:2
    TMR0 = 254;                     // Retardo de 0.015
            
    // Configuración ADC
    ADCON0bits.ADCS = 0b00000010;   // FOSC/32
    ADCON1bits.VCFG0 = 0;           // VDD
    ADCON1bits.VCFG1 = 0;           // VSS
    ADCON0bits.CHS = 0b0000;        // Selección de canal AN0
    ADCON1bits.ADFM = 0;            // Configuración de justificado a la izquierda
    ADCON0bits.ADON = 1;            // Habilitación del modulo ADC
    __delay_us(40);               // Delay de sample time

    return;
}

/*------------------------------------------------------------------------------
 * FUNCIONES 
 ------------------------------------------------------------------------------*/
unsigned short map(short x, uint8_t x0, uint8_t x1, 
            unsigned short y0, unsigned short y1){
    return (unsigned short)(y0+((float)(y1-y0)/(x1-x0))*(x-x0));
}
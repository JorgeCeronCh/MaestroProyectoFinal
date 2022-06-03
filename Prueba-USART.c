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
#include <stdlib.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000      // Frecuencia de oscilador en 1 MHz
#define IN_MIN 0                
#define IN_MAX 127              // Valores de entrada a Potenciometro
#define OUT_MIN1 135             
#define OUT_MAX1 580             // Valores para el servomotor MG996R ---> en realidad debería de ser 125 y 600 pero se disminuyeron para que giraran casi 180 grados

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/

char Lectura_UART[4];
int i=0;

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);


/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.RCIF){                  // Hay datos recibidos?
        PORTD = RCREG;        
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){

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
    ANSEL = 0b00001111;             // AN0 - AN3 como entrada analógicas
    ANSELH = 0b00000000;            // I/O digitales
    
    TRISA = 0b00001111;             // AN0 - AN4 como entrada
    TRISB = 0b00000111;             // RB0 - RB2 como entrada
    TRISC = 0b00010000;             // SDI entrada, SCK y SD0 como salida
    
    // RD0 y RD1 inidcan hacia cuál servomotor enviar la señal
    TRISD = 0b00000000;             // Como salida
    // NO SE USA -> PARA PRUEBAS
    TRISE = 0b00000000;             // Como salida
    
    PORTA = 0b00000000;             // Limpieza del PORTA
    PORTB = 0b00000000;             // Limpieza del PORTB
    PORTC = 0b00000000;             // Limpieza del PORTC
    PORTD = 0b00000000;             // Limpieza del PORTD
    PORTE = 0b00000000;             // Limpieza del PORTE
    
    // Configuración de interrucpiones
    INTCONbits.GIE = 1;             // Habilitamos interrupciones globales
    INTCONbits.PEIE = 1;            // Habilitamos interrupciones de perifericos

    PIE1bits.RCIE = 1;              // Habilitamos Interrupciones de recepción
    
    // Configuracion de comunicacion serial
    //SYNC = 0, BRGH = 1, BRG16 = 1, SPBRG=25 <- Valores de tabla 12-5
    TXSTAbits.SYNC = 0;         // Comunicación ascincrona (full-duplex)
    TXSTAbits.BRGH = 1;         // Baud rate de alta velocidad 
    BAUDCTLbits.BRG16 = 1;      // 16-bits para generar el baud rate
    
    SPBRG = 25;
    SPBRGH = 0;                 // Baud rate ~9600, error -> 0.16%
    
    RCSTAbits.SPEN = 1;         // Habilitamos comunicación
    TXSTAbits.TX9 = 0;          // Utilizamos solo 8 bits
    RCSTAbits.RC9 = 0; 
    TXSTAbits.TXEN = 1;         // Habilitamos transmisor
    RCSTAbits.CREN = 1;         // Habilitamos receptor
    return;
}

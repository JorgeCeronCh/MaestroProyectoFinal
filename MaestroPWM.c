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
#define OUT_MIN1 135             
#define OUT_MAX1 590             // Valores para el servomotor MG996R ---> en realidad debería de ser 125 y 600 pero se disminuyeron para que giraran casi 180 grados

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
unsigned short POT_1;     // Valor de lectura del potenciómetro 1 
unsigned short POT_2;     // Valor de lectura del potenciómetro 2 
uint8_t POT_3;            // Valor de lectura del potenciómetro 3 
uint8_t POT_4;            // Valor de lectura del potenciómetro 4 
uint8_t POT_5;            // Valor de lectura del potenciómetro 5 
uint8_t POT_6;            // Valor de lectura del potenciómetro 6
uint8_t escala;
int cont;

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);
unsigned short map(short val, uint8_t in_min, uint8_t in_max, 
            unsigned short out_min, unsigned short out_max);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.ADIF){                  // Verificación de interrupción del módulo ADC
        if(ADCON0bits.CHS == 0){        // Verificación de canal AN0
            POT_1 = map(ADRESH, IN_MIN, IN_MAX, OUT_MIN1, OUT_MAX1);    // Almacenar el resgitro ADRESH en variable POT1
            CCPR1L = (uint8_t)(POT_1>>2);                               // 8 bits mas significativos en CPR1L
            CCP1CONbits.DC1B = POT_1 & 0b11;                            // 2 bits menos significativos en DC1B
            ADCON0bits.CHS = 1;                                         // Cambio de AN0 a AN1
        } 
        else if(ADCON0bits.CHS == 1){   // Verificación de canal AN1
            POT_2 = map(ADRESH, IN_MIN, IN_MAX, OUT_MIN1, OUT_MAX1);    // Almacenar el resgitro ADRESH en variable POT2
            CCPR2L = (uint8_t)(POT_2>>2);                               // 8 bits mas significativos en CPR1L
            CCP2CONbits.DC2B0 = POT_2 & 0b01; 
            CCP2CONbits.DC2B1 = (POT_2 & 0b10)>>1;                      // 2 bits menos significativos en DC2B0 y DC2B1
            ADCON0bits.CHS = 2;                                         // Cambio de AN1 a AN2
        } 
        else if(ADCON0bits.CHS == 2){   // Verificación de canal AN2
            POT_3 = ADRESH;             // Almacenar el resgitro ADRESH en variable POT3
            ADCON0bits.CHS = 3;         // Cambio de AN2 a AN3
        } 
        else if(ADCON0bits.CHS == 3){   // Verificación de canal AN3
            POT_4 = ADRESH;             // Almacenar el resgitro ADRESH en variable POT4
            ADCON0bits.CHS = 4;         // Cambio de AN3 a AN4
        } 
        else if(ADCON0bits.CHS == 4){   // Verificación de canal AN4
            POT_5 = ADRESH;             // Almacenar el resgitro ADRESH en variable POT5
            ADCON0bits.CHS = 5;         // Cambio de AN4 a AN5
        } 
        else if(ADCON0bits.CHS == 5){   // Verificación de canal AN5
            POT_6 = ADRESH;             // Almacenar el resgitro ADRESH en variable POT6
            ADCON0bits.CHS = 0;         // Cambio de AN5 a AN0
        }
        PIR1bits.ADIF = 0;              // Limpieza de bandera de interrupción
    }
    
    if (INTCONbits.T0IF){
            
            if (ADCON0bits.GO == 0){
                ADCON0bits.GO = 1;          // On controlado con tiempo de TMR0
            }    
        escala++;
        
        if(escala < POT_3) {PORTCbits.RC6 = 1;}  
        else {PORTCbits.RC6 = 0;} 
        INTCONbits.T0IF = 0;            // Limpieza de bandera de interrupción TMR0
        TMR0 = 245;                     // Tiempo de 5.6 ms
        
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){        
        PORTAbits.RA6 = 1;              // Deshabilitar ESCLAVO2
        PORTAbits.RA7 = 0;              // Habilitar ESCLAVO1
        PORTDbits.RD0 = 1;              // Pin RD0 de salida en encendido para habilitar el CCP1 del ESCLAVO1
        PORTDbits.RD1 = 0;              // Pin RD1 de salida apagado pues no se habilita el CCP2 del ESCLAVO1
        PORTDbits.RD2 = 0;              // Pin RD2 de salida apagado pues no se habilita el PWM manual del ESCLAVO1
        SSPBUF = POT_4;                 // Cargamos valor del potenciómetro al buffer
        while(!SSPSTATbits.BF){}        // Esperamos a que termine el envio
                
        PORTAbits.RA6 = 1;              // Deshabilitar ESCLAVO2
        PORTAbits.RA7 = 1;              // Deshabilitar ESCLAVO1
                
        __delay_ms(25);                 // Sample time para el cambio
                
        PORTAbits.RA6 = 1;              // Deshabilitar ESCLAVO2
        PORTAbits.RA7 = 0;              // Habilitar ESCLAVO1
        PORTDbits.RD0 = 0;              // Pin RD0 de salida apagado pues no se habilita el CCP1 del ESCLAVO1
        PORTDbits.RD1 = 1;              // Pin RD1 de salida en encendido para habilitar el CCP2 del ESCLAVO1
        PORTDbits.RD2 = 0;              // Pin RD2 de salida apagado pues no se habilita el PWM manual del ESCLAVO1
        SSPBUF = POT_5;                 // Cargamos valor del potenciómetro al buffer
        while(!SSPSTATbits.BF){}        // Esperamos a que termine el envio
                
        PORTAbits.RA6 = 1;              // Deshabilitar ESCLAVO2
        PORTAbits.RA7 = 1;              // Deshabilitar ESCLAVO1
                
        __delay_ms(25);                 // Sample time para el cambio
        // ----------- COMENTAR PARA QUE FUNCIONEN 4 SERVOS DECENTES -----------
        PORTAbits.RA6 = 1;              // Habilitar ESCLAVO2
        PORTAbits.RA7 = 0;              // Deshabilitar ESCLAVO1
        PORTDbits.RD0 = 0;              // Pin RD0 de salida en encendido para habilitar el CCP1 del ESCLAVO2
        PORTDbits.RD1 = 0;              // Pin RD1 de salida apagado pues no se habilita el CCP2 del ESCLAVO2
        PORTDbits.RD2 = 1;              // Pin RD2 de salida encendido pues no se habilita el PWM manual del ESCLAVO1
        SSPBUF = POT_6;                 // Cargamos valor del potenciómetro al buffer
        while(!SSPSTATbits.BF){}        // Esperamos a que termine el envio
               
        PORTAbits.RA6 = 1;              // Deshabilitar ESCLAVO2
        PORTAbits.RA7 = 1;              // Deshabilitar ESCLAVO1
                
        __delay_ms(25);                 // Sample time para el cambio
        /*         
        PORTAbits.RA6 = 0;              // Habilitar ESCLAVO2
        PORTAbits.RA7 = 1;              // Deshabilitar ESCLAVO1
        PORTDbits.RD2 = 0;              // Pin RD2 de salida apagado pues no se habilita el CCP1 del ESCLAVO2
        PORTDbits.RD3 = 1;              // Pin RD3 de salida en encendido para habilitar el CCP2 del ESCLAVO2
        SSPBUF = POT_6;                 // Cargamos valor del potenciómetro al buffer
        while(!SSPSTATbits.BF){}        // Esperamos a que termine el envio
                
        PORTAbits.RA6 = 1;              // Deshabilitar ESCLAVO2
        PORTAbits.RA7 = 1;              // Deshabilitar ESCLAVO1
                
        __delay_ms(50);                 // Sample time para el cambio
         */
        // ----------------------- TERMINAR DE COMENTAR -----------------------
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
    ANSEL = 0b00111111;             // AN0 - AN5 como entrada analógicas
    ANSELH = 0b00000000;            // I/O digitales
    TRISA = 0b00101111;             // AN0 - AN4 como entrada
    TRISC = 0b00010000;             // SDI entrada, SCK y SD0 como salida
    TRISD = 0b00000000;             // AN5 como salida
    TRISE = 0b00000001;             // AN5 como entrada
    PORTA = 0b00000000;             // Limpieza del PORTA
    PORTE = 0b00000000;             // Limpieza del PORTE
    PORTD = 0b00000000;             // Limpieza del PORTD
    
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
    OPTION_REGbits.PS= 0b110;       // 1:128
    TMR0 = 237;                     // Empieza con 10 ms
            
    // Configuración ADC
    ADCON0bits.ADCS = 0b00000010;   // FOSC/32
    ADCON1bits.VCFG0 = 0;           // VDD
    ADCON1bits.VCFG1 = 0;           // VSS
    ADCON0bits.CHS = 0b0000;        // Selección de canal AN0
    ADCON1bits.ADFM = 0;            // Configuración de justificado a la izquierda
    ADCON0bits.ADON = 1;            // Habilitación del modulo ADC
    __delay_us(1000);               // Delay de sample time
        
    // Configuración de SPI    
    // Configuración del MAESTRO    
    // SSPCON<5:0>
    SSPCONbits.SSPM = 0b0000;       // SPI Maestro, Reloj -> Fosc/4 (250kbits/s)
    SSPCONbits.CKP = 0;             // Reloj inactivo en 0
    SSPCONbits.SSPEN = 1;           // Habilitamos pines de SPI (Para que no los utilice como RPG)
    // SSPSTAT<7:6>
    SSPSTATbits.CKE = 1;            // Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 1;            // Dato al final del pulso de reloj
    SSPBUF = 0b00000000;            // Enviamos un dato inicial (valor inicial de la variable)
    
    // Configuración PWM
    TRISCbits.TRISC2 = 1;           // Deshabilitar salida de CCP1 (Se pone como entrada)
    TRISCbits.TRISC1 = 1;           // Deshabilitar salida de CCP2 (Se pone como entrada)
    PR2 = 250;                      // Periodo de 4 ms
    
    // Configuracion CCP
    CCP1CON = 0;                    // Apagar CCP1
    CCP2CON = 0;                    // Apagar CCP2
    CCP1CONbits.P1M = 0;            // Modo sigle output
    CCP1CONbits.CCP1M = 0b1100;     // PWM
    CCP2CONbits.CCP2M = 0b1100;     // PWM
    
    CCPR1L = 125>>2;                // REVISAR   <-----------------
    CCP1CONbits.DC1B = 125 & 0b11; 
    CCPR2L = 125>>2;
    CCP2CONbits.DC2B0 = 125 & 0b01; 
    CCP2CONbits.DC2B1 = 125 & 0b10;   
    
    PIR1bits.TMR2IF = 0;            // Limpiar bandera de TMR2
    T2CONbits.T2CKPS = 0b01;        // Prescaler 1:4
    T2CONbits.TMR2ON = 1;           // Encender TMR2
    while (!PIR1bits.TMR2IF);       // Esperar un ciclo del TMR2
    PIR1bits.TMR2IF = 0;
    
    TRISCbits.TRISC2 = 0;           // Habilitar salida de PWM
    TRISCbits.TRISC1 = 0;           // Habilitar salida de PWM
    
    return;
}

/*------------------------------------------------------------------------------
 * FUNCIONES 
 ------------------------------------------------------------------------------*/
unsigned short map(short x, uint8_t x0, uint8_t x1, 
            unsigned short y0, unsigned short y1){
    return (unsigned short)(y0+((float)(y1-y0)/(x1-x0))*(x-x0));
}

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
#define IN_MAX 255              // Valores de entrada a Potenciometro
#define OUT_MIN1 135             
#define OUT_MAX1 580             // Valores para el servomotor MG996R ---> en realidad debería de ser 125 y 600 pero se disminuyeron para que giraran casi 180 grados

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
uint8_t MODO = 0;                           // Variable para el cambio de modo
unsigned short POT_1_M;                     // Valor de lectura del potenciómetro 1 
unsigned short POT_2_M;                     // Valor de lectura del potenciómetro 2 
uint8_t POT_1;                              // Valor de lectura del potenciómetro 1 
uint8_t POT_2;                              // Valor de lectura del potenciómetro 2 
uint8_t POT_3;                              // Valor de lectura del potenciómetro 3 
uint8_t POT_4;                              // Valor de lectura del potenciómetro 4 
uint8_t BANDERA_L1, BANDERA_L2;             // Bandera de lectura
uint8_t BANDERA_E1, BANDERA_E2;             // Bandera de escritura
uint8_t SELECCION_SERVO;                    // Variable para seleccion de servomotor
uint8_t VALORPOT_USART;                     // Variable que almacena el valor del servomotor
uint8_t CONT;


/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);
unsigned short map(short val, uint8_t in_min, uint8_t in_max, 
            unsigned short out_min, unsigned short out_max);
void ESCRITURA_EEPROM(uint8_t DIRECCION, uint8_t DATA);
uint8_t LECTURA_EEPROM(uint8_t DIRECCION);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(INTCONbits.RBIF){                    // Verificación de interrupción del PORTB
        if(!PORTBbits.RB0){                 // Verificación de interrupción de RB0
            MODO++;                         // Incremento para cambio de modo por presionar el botón
            if (MODO > 2){                  // Condicional que no exceda de tres estados 
                MODO = 0;                   // Reinicio del modo
            }
        }
        else if(!PORTBbits.RB1){            // Verificación de interrupción de RB1
            if(MODO == 0){
                BANDERA_E1 = 1;
            }
            else if(MODO == 1){
                BANDERA_L1 = 1;
            }           
        }
        else if(!PORTBbits.RB2){            // Verificación de interrupción de RB1
            if(MODO == 0){
                BANDERA_E2 = 1;
            }
            else if(MODO == 1){
                BANDERA_L2 = 1;
            }           
        }
        INTCONbits.RBIF = 0;                // Limpieza de bandera de interrupción del PORTB
    }

    if(PIR1bits.ADIF){                      // Verificación de interrupción del módulo ADC
        if(ADCON0bits.CHS == 0){        // Verificación de canal AN0
            if(MODO == 0){
                POT_1 = ADRESH;
                POT_1_M = map(POT_1, IN_MIN, IN_MAX, OUT_MIN1, OUT_MAX1);    // Almacenar el resgitro ADRESH en variable POT1
            }
            else if (MODO == 1){
                POT_1_M = map(POT_1, IN_MIN, IN_MAX, OUT_MIN1, OUT_MAX1);
            }
            
            CCPR1L = (uint8_t)(POT_1_M>>2);                               // 8 bits mas significativos en CPR1L
            CCP1CONbits.DC1B = POT_1_M & 0b11;                            // 2 bits menos significativos en DC1B
            
        } 
        else if(ADCON0bits.CHS == 1){   // Verificación de canal AN1
            if(MODO == 0){
                POT_2 = ADRESH;
                POT_2_M = map(POT_2, IN_MIN, IN_MAX, OUT_MIN1, OUT_MAX1);    // Almacenar el resgitro ADRESH en variable POT2
            }
            else if (MODO == 1){
                POT_2_M = map(POT_2, IN_MIN, IN_MAX, OUT_MIN1, OUT_MAX1);
            }
            CCPR2L = (uint8_t)(POT_2_M>>2);                               // 8 bits mas significativos en CPR1L 01010101"01" -> 01010101
            CCP2CONbits.DC2B0 = POT_2_M & 0b01; 
            CCP2CONbits.DC2B1 = (POT_2_M & 0b10)>>1;                      // 2 bits menos significativos en DC2B0 y DC2B1
        } 
        else if(ADCON0bits.CHS == 2){       // Verificación de canal AN2
            if(MODO == 0){
                POT_3 = ADRESH;             // Almacenar el resgitro ADRESH en variable POT3
            }
            /*else if (MODO == 1){
                __delay_ms(1000);
            }*/
            PORTDbits.RD0 = 0;              // Pin RD0 de salida en encendido para habilitar el CCP1 del ESCLAVO1
            SSPBUF = POT_3;                 // Cargamos valor del potenciómetro al buffer
            while(!SSPSTATbits.BF){}        // Esperamos a que termine el envio
            __delay_ms(50);                 // Sample time para el cambio
            
        } 
        else if(ADCON0bits.CHS == 3){       // Verificación de canal AN3
            if(MODO == 0){
                POT_4 = ADRESH;             // Almacenar el resgitro ADRESH en variable POT4
            }
            /*else if (MODO == 1){
                __delay_ms(1000);
            }*/
            PORTDbits.RD0 = 1;              // Pin RD0 de salida apagado pues no se habilita el CCP1 del ESCLAVO1
            SSPBUF = POT_4;                 // Cargamos valor del potenciómetro al buffer
            while(!SSPSTATbits.BF){}        // Esperamos a que termine el envio
            __delay_ms(50);                 // Sample time para el cambio
            
        }
        PIR1bits.ADIF = 0;                  // Limpieza de bandera de interrupción
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
            if(ADCON0bits.CHS == 0){        // Interrupción por AN0
                ADCON0bits.CHS = 1;    // Cambio de AN0 a AN1
            }
            else if (ADCON0bits.CHS == 1){  // Interrupción por AN1
                ADCON0bits.CHS = 2;    // Cambio de AN1 a AN0         
            }
            else if (ADCON0bits.CHS == 2){  // Interrupción por AN1
                ADCON0bits.CHS = 3;    // Cambio de AN1 a AN0         
            }
            else if (ADCON0bits.CHS == 3){  // Interrupción por AN1
                ADCON0bits.CHS = 0;    // Cambio de AN1 a AN0         
            }
             __delay_us(40);                // Sample time era de 10 ms
            ADCON0bits.GO = 1;              // On
        }
        if (MODO == 0){
            /*if (BANDERA_E1 == 1){
                ESCRITURA_EEPROM(1, POT_1);
                __delay_ms(1);
                ESCRITURA_EEPROM(2, POT_2);
                __delay_ms(1);
                ESCRITURA_EEPROM(3, POT_3);
                __delay_ms(1);
                ESCRITURA_EEPROM(4, POT_4);
                __delay_ms(1);
                
                BANDERA_E1 = 0;
            }
            if(BANDERA_E2 == 1){
                ESCRITURA_EEPROM(5, POT_1);
                __delay_ms(1);
                ESCRITURA_EEPROM(6, POT_2);
                __delay_ms(1);
                ESCRITURA_EEPROM(7, POT_3);
                __delay_ms(1);
                ESCRITURA_EEPROM(8, POT_4);
                __delay_ms(1);
                
                BANDERA_E2 = 0;
            }*/
            PORTEbits.RE0 = 1;
            PORTEbits.RE1 = 0;
            PORTEbits.RE2 = 0;
        }
        else if (MODO == 1){
            /*if (BANDERA_L1 == 1){
                
                POT_1 = LECTURA_EEPROM(1);
                __delay_ms(1);          // <-------------- PROBAR DELAY DE 1000 a 5000 ms
                POT_2 = LECTURA_EEPROM(2);
                __delay_ms(1);          // <-------------- PROBAR DELAY DE 1000 a 5000 ms
                POT_3 = LECTURA_EEPROM(3);
                __delay_ms(1);          // <-------------- PROBAR DELAY DE 1000 a 5000 ms
                POT_4 = LECTURA_EEPROM(4);
                __delay_ms(1);          // <-------------- PROBAR DELAY DE 1000 a 5000 ms
                        
                BANDERA_L1 = 0;
            }
            if (BANDERA_L2 == 1){
                POT_1 = LECTURA_EEPROM(5);
                __delay_ms(1);          // <-------------- PROBAR DELAY DE 1000 a 5000 ms
                POT_2 = LECTURA_EEPROM(6);
                __delay_ms(1);          // <-------------- PROBAR DELAY DE 1000 a 5000 ms
                POT_3 = LECTURA_EEPROM(7);
                __delay_ms(1);          // <-------------- PROBAR DELAY DE 1000 a 5000 ms
                POT_4 = LECTURA_EEPROM(8);
                __delay_ms(1);          // <-------------- PROBAR DELAY DE 1000 a 5000 ms
                
                BANDERA_L2 = 0;
            }*/
            PORTEbits.RE0 = 0;
            PORTEbits.RE1 = 1;
            PORTEbits.RE2 = 0;  
        }
        else if (MODO == 2){
            PORTEbits.RE0 = 0;
            PORTEbits.RE1 = 0;
            PORTEbits.RE2 = 1;
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
    ANSEL = 0b00001111;             // AN0 - AN3 como entrada analógicas
    ANSELH = 0b00000000;            // I/O digitales
    
    TRISA = 0b00001111;             // AN0 - AN3 como entrada
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
    
    //INTCONbits.T0IE = 1;            // Habilitamos interrupcion de TMR0
    //INTCONbits.T0IF = 0;            // Limpiamos bandera de TMR0
    
    PIE1bits.ADIE = 1;              // Habilitamos interrupcion de ADC
    PIR1bits.ADIF = 0;              // Limpiamos bandera de ADC
    
    INTCONbits.RBIE = 1;            // Habilitamos interrupción de PORTB
    INTCONbits.RBIF = 0;            // Limpiamos bandera de PORTB
    
    //PIE1bits.RCIE = 1;              // Habilitamos Interrupciones de recepción
    // Configuracion TMR0
    //OPTION_REGbits.T0CS = 0;        // Temporizador
    //OPTION_REGbits.PSA = 0;         // Prescaler a TMR0
    //OPTION_REGbits.PS= 0b110;       // 1:128
    //TMR0 = 237;                     // Empieza con 10 ms
   
    // Configuración PORTB
    OPTION_REGbits.nRBPU = 0;       // Habilitamos Pull-Ups
    WPUBbits.WPUB = 0b00000111;     // Pull-Ups en RB0 - RB2
    IOCBbits.IOCB = 0b00000111;     // Habilitamos interrupcion on change RB0 - RB2
            
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

uint8_t LECTURA_EEPROM(uint8_t DIRECCION){
    EEADR = DIRECCION;              // Cargar dirección
    EECON1bits.EEPGD = 0;           // Realizar lectura de la EEPROM
    EECON1bits.RD = 1;              // Obtención del dato de la EEPROM
    return EEDAT;                   // Retorno del dato extraído de la EEPROM 
}

void ESCRITURA_EEPROM(uint8_t DIRECCION, uint8_t DATA){
    EEADR = DIRECCION;              // Cargar dirección
    EEDAT = DATA;                   // Cargar dato a escribir
    EECON1bits.EEPGD = 0;           // Modo escritura a la EEPROM
    EECON1bits.WREN = 1;            // Habilitar escritura en la EEPROM
    
    INTCONbits.GIE = 0;             // Deshabilitar las interrupciones globales
    EECON2 = 0x55;      
    EECON2 = 0xAA;
    
    EECON1bits.WR = 1;              // Iniciar escritura
    
    while(!PIR2bits.EEIF); //Mientras no se haya completado la escritura espera
    PIR2bits.EEIF = 0; //Limpieza de bandera de escritura
    
    EECON1bits.WREN = 0;            // Deshabilitar escritura en la EEPROM
    INTCONbits.RBIF = 0;            // Limpiar interrupciones PORTB
    INTCONbits.GIE = 1;             // Habilitar las interrupciones globales
}
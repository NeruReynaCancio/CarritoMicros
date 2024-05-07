// Archivos de cabecera
#include <p30f4011.h>
#define FCY 2000000UL
#include <libpic30.h>

// Definiciones de pines
#define ATRAS PORTBbits.RB0   // LED de salida conectado al pin RB0
#define ADELANTE PORTEbits.RE0   // LED de salida conectado al pin RB0
#define TRIG PORTBbits.RB1  // Pin de salida para el pulso de trigger
#define ECHO PORTBbits.RB2  // Pin de entrada para recibir el eco del sensor ultrasonico
int Objetivo;
// Prototipos de funciones
void puertosIO();        // Configuracion de los puertos de entrada/salida
void TIMER1();           // Configuracion del Timer 1 como temporizador s?ncrono
void TIMER2();
int Obtener_Distancia(); // Funcion para obtener la distancia mediante un sensor ultrasonico
void Receptor_UART();
// Funcion principal
int main (){
    int Distancia;  // Variable para almacenar la distancia medida por el sensor ultrasonico
    int Distancia_Recorrida;
    // Configuracion inicial del sistema
    puertosIO();  // Configurar los puertos de entrada/salida
    TIMER1();     // Configurar el Timer 1
    TIMER2();
	Receptor_UART();
	U2RXREG=0;
    // Bucle principal
    while (1){
        Distancia = Obtener_Distancia();
		if (Distancia > 25){
            ADELANTE = 1;
			ATRAS =0;
        }
        else {
           	ADELANTE = 0;
			ATRAS=0;
        
        }
		
    }
}

// Implementacion de funciones

// Configuracion de pines de entrada/salida
void puertosIO(){	
    TRISBbits.TRISB0 = 0; // ATRAS
	TRISEbits.TRISE0 = 0; // ADELANTE
    TRISBbits.TRISB1 = 0; // TRIG
    TRISBbits.TRISB2 = 1; // ECHO
    ADPCFG = 7; 
}

int Obtener_Distancia(){
//hicimos uso del timer 1 para estimar el tiempo y as? poder configurar las distancias  
	int Duracion;
	int Distancia;
	int Tiempo;
	
	TRIG = 1;
	T2CONbits.TON=1; //Enciende el timer 1
	
	while (ECHO==0){}
	T1CONbits.TON=1;// enciende el timer
	while (ECHO==1){}
	T1CONbits.TON=0;//apaga el timer

	Tiempo=TMR1; //aqui se guardan los ciclos que alcanzo a contar el timer1
	Duracion = Tiempo/2; //tiempo de ida en que rebota

	
	if(Duracion<=14724) {// 14724 ciclos equivale a 400cm
		Distancia=Duracion/22;  //7 ciclos equivalen a un cm.
	}
	else if(Duracion<116){ //116 ciclos equivalen a 2cm (rango minimo del sensor)
		Distancia=0;
	
	}
	else {
		Distancia=0;
	}
	Duracion=0;
	TMR1=0;
	return Distancia;
	
	}


void TIMER1(){//temporizador sincrono
T1CON=0; //pone todos los bits de t1 en 0 
T1CONbits.TCKPS=1; //para que cuente de 8 en 8. Pre escaler. 1:64
//no le movemos ni a TGATE, ni a TCS(ya que es una se?al interna)
TMR1=0;  //donde va a empezar 
PR1=65535; //numero al que tiene que llegar
IFS0bits.T1IF=0; 
IEC0bits.T1IE=1; 
IPC0bits.T1IP=4;
T1CONbits.TON=0;
}

void TIMER2(){ 
T2CONbits.TON=0; //Apaga el timer 1
T2CONbits.TSIDL=0; //Continua en ahorro de energia 
T2CONbits.TGATE=0; //No acumula la entrada 
T2CONbits.TCKPS=3; //Preescaler 1:256 
T2CONbits.TCS=0; //Reloj Interno 
TMR2=0; //Inicia conteo en cero 
PR2=12800; //Conteo 10us FCLK de 20MHz, pulso 
IFS0bits.T2IF=0; //Borra la bandera 
IEC0bits.T2IE=1; //Aqui la habilita como interrupcion 
IPC1bits.T2IP=4; //Prioridad 4 
T2CONbits.TON=1; //Enciende el timer 1
}
void Receptor_UART(){   // Configuramos el modulo del  UART

	U2MODE = 0;              // Borra el registro de configuracion de UART 
    U2MODEbits.ABAUD = 1;    // Entramos al Modulo de Captura desde UxRx
    U2MODEbits.PDSEL = 1;    // Establece la Paridad par, 8 bits de datos
    U2BRG = 32;              // Baudaje a 9600 (1.35% de error)
    U2STA = 0;               // Borra el registro de estado de la UART
    U2STAbits.URXISEL =0;   // Interrupcion cuando hay un byte disponible en el buffer de 			 
							//recepcion (En otras palabras va leyendo de 1 en 1) 3= 5 palabras
    IEC1bits.U2RXIE = 1;     // Habilita la interrupcion de recepcion
    IPC6bits.U2RXIP = 4;     // Prioridad de la interrupcion de recepcion
    IFS1bits.U2RXIF = 0;     // Limpia la bandera de interrupcion de recepcion
    U2STAbits.URXDA=0;		 //Recibe si el buffer esta vacio
    U2MODEbits.UARTEN = 1;   // Habilita el modulo del receptor
    
}

// Manejadores de interrupciones (ISRs)

// ISR para Timer 1 (Interrupcion del Timer 1)
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void){
    IFS0bits.T1IF = 0;   // Bajar la bandera de interrupcion del Timer 1
    TMR1 = 0;            // Reiniciar el contador del Timer 1 
}

// ISR para Timer 2 (Interrupcion del Timer 2)
void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void){
    TRIG = 0;
	IFS0bits.T2IF = 0;   // Bajar la bandera de interrupcion del Timer 2
    TMR2 = 0;            // Reiniciar el contador del Timer 2
}

void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void) {
	  Objetivo=U2RXREG;
      IFS1bits.U2RXIF = 0; //Se baja la bandera de Interrupcion
      U2STAbits.OERR=0;    //Limpiamos OERR , en otras palabras borramos el bit de error (NO SE DEBE OLVIDAR PONERLO)
						   //Evitamos errores en Overflow 
}
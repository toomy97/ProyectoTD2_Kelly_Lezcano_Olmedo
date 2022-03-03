#include "Display_16x2_I2C.h"

extern I2C_HandleTypeDef hi2c1;

#define Direccion_Slave 0x4E

void Display_Enviar_Instruccion(char Instruccion)
{
	char data_u, data_l;
	uint8_t data_t[4];

	data_u=(Instruccion & 0xf0);
	data_l=((Instruccion<<4) & 0xf0);

	data_t[0] = data_u|0x0C; //E=1, RS=0
	data_t[1] = data_u|0x08; //E=0, RS=0
	data_t[2] = data_l|0x0C; //E=1, RS=0
	data_t[3] = data_l|0x08; //E=0, RS=0

	HAL_I2C_Master_Transmit (&hi2c1, Direccion_Slave,(uint8_t *) data_t, 4, 100);
}

void Display_Inicializacion(void)
{
	//Interface de 4 bits, solamente uso los bits de datos desde D4 hasta D7
	//Entonces los bits de D0 hasta D3 los pongo en cero (0)

//Comienza la Inicializacion

	HAL_Delay(20); //Esperar por mas de 15ms

	Display_Enviar_Instruccion(0x30); // RS RW D7 D6 D5 D4
								      //  0  0  0  0  1  1

	HAL_Delay(5); //Esperar por mas de 4.1ms

	Display_Enviar_Instruccion(0x30); // RS RW D7 D6 D5 D4
								      //  0  0  0  0  1  1

	HAL_Delay(1); //Esperar por mas de 100us

	Display_Enviar_Instruccion(0x30); // RS RW D7 D6 D5 D4
					                  //  0  0  0  0  1  1

	HAL_Delay(10);

//-----Function Set-----> Seteo la interface en el modo de 4 bits. DL=0 (Modo 4 bits)
	Display_Enviar_Instruccion(0x20); // RS RW D7 D6 D5   D4
								      //  0  0  0  0  1 DL=0

	HAL_Delay(10);

//-----Funcion Set-----> Seteo el numero de lineas del display y la fuente de los caracteres. N=1 (Display de 2 filas), F=0 (Caracteres de 5x8)
	Display_Enviar_Instruccion(0x28); // RS RW   D7   D6  D5 D4
								      //  0  0    0    0   1  0
								      //  0  0  N=1  F=0   X  X

	HAL_Delay(1);

//-----Display ON/OFF Control-----> D=0 (Display OFF), C=0 (Cursor apagado) y B=0 (Parpadeo del cursor apagado)
	Display_Enviar_Instruccion(0x08); // RS RW D7   D6   D5   D4
	                                  //  0  0  0    0    0    0
                                      //  0  0  1  D=0  C=0  B=0

	HAL_Delay(1);

//-----Display Clear-----
	Display_Enviar_Instruccion(0x01); // RS RW D7 D6 D5 D4
                                      //  0  0  0  0  0  0
                                      //  0  0  0  0  0  1

	HAL_Delay(1);

//-----Entry Mode Set-----> I/D=1 (Incrementa el cursor), S=0 (No se desplaza el cursor)
	Display_Enviar_Instruccion(0x06); // RS RW D7 D6    D5   D4
									  //  0  0  0  0     0    0
									  //  0  0  0  1 I/D=1  S=0

//Finaliza la Inicializacion

	HAL_Delay(1);

//-----Display ON/OFF Control-----> D=1 (Display ON), C=0 (Cursor apagado), B=0 (Parpadeo del cursor apagado)
	Display_Enviar_Instruccion(0x0C); // RS RW D7   D6   D5   D4
									  //  0  0  0    0    0    0
	                                  //  0  0  1  D=1  C=0  B=0

	HAL_Delay(1);
}

void Display_Ubicar_Cursor(int Fila, int Columna) //Ubico el cursor en una determinada Fila (0 o 1) y Columna (de 0 a 15)
{
    switch(Fila)
    {
        case 0:
            Columna |= 0x80;
            break;
        case 1:
            Columna |= 0xC0;
            break;
    }

    Display_Enviar_Instruccion(Columna);
}

void Display_Enviar_String(char *String)
{
	while (*String) Display_Enviar_Dato(*String++);
}

void Display_Enviar_Dato(char Dato)
{
	char data_u, data_l;
	uint8_t data_t[4];

	data_u = (Dato&0xf0);
	data_l = ((Dato<<4)&0xf0);

	data_t[0] = data_u|0x0D; //E=1, RS=0
	data_t[1] = data_u|0x09; //E=0, RS=0
	data_t[2] = data_l|0x0D; //E=1, RS=0
	data_t[3] = data_l|0x09; //E=0, RS=0

	HAL_I2C_Master_Transmit (&hi2c1, Direccion_Slave,(uint8_t *) data_t, 4, 100);
}

void Display_Borrar_Pantalla(void)
{
	Display_Enviar_Instruccion(0x01);
}

#include "stm32f4xx_hal.h"

void Display_Enviar_Instruccion(char Instruccion);

void Display_Inicializacion(void);

void Display_Ubicar_Cursor(int Fila, int Columna);

void Display_Enviar_String(char *String);

void Display_Enviar_Dato(char Dato);

void Display_Borrar_Pantalla(void);

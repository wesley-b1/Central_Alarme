/*
 * i2c.c
 *
 * Created: 25/08/2021 16:15:40
 *  Author: Lucas
 */ 
#include <avr/io.h>
#include "i2c.h"

void I2C_Init()
{
    DDRC = (1 << 4) | (1 << 5); 
    TWSR=0x01;    //Prescaler = 4
    TWBR=18;    //Frequência de 100Khz para osc = 16Mhz
    TWCR=0x04;    //Habilita o módulo
}

void I2C_Start()
{
    TWCR = ((1<<TWINT) | (1<<TWSTA) | (1<<TWEN));
    while (!(TWCR & (1<<TWINT)));
}

void I2C_Stop(void)
{
    TWCR = ((1<< TWINT) | (1<<TWEN) | (1<<TWSTO));
    //Precisa esperar um tempo 50ms (Chamar interrupcao) depois dessa funcao
}

void I2C_Write(uint8_t Valor_DATA_I2C)
{
    TWDR = Valor_DATA_I2C ;
    TWCR = ((1<< TWINT) | (1<<TWEN));
    while (!(TWCR & (1 <<TWINT)));
}

uint8_t I2C_Read(uint8_t Valor_ACK)
{
    TWCR = ((1<< TWINT) | (1<<TWEN) | (Valor_ACK<<TWEA));
    while ( !(TWCR & (1 <<TWINT)));
    return TWDR;
}
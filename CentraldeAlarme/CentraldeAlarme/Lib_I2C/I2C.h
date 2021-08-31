/*
 * i2c.h
 *
 * Created: 25/08/2021 17:52:55
 *  Author: Lucas
 */ 



#ifndef I2C_H_
#define I2C_H_
#include <avr/io.h>

void I2C_Init();
void I2C_Start();
void I2C_Stop(void);
void I2C_Write(uint8_t Valor_DATA_I2C);
uint8_t I2C_Read(uint8_t Valor_ACK);

#endif /* I2C_H_ */
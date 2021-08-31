#include <avr/io.h>
#include "Lib_UART/uart_alarm.h"
#include <avr/interrupt.h>
#include "Lib_I2C/I2C.h"

#define DESA	0
#define ATIV	1
#define PROG	2
#define PANI	3
#define RECP	4
//---------------------------------------------------------------------------
//GLOBAIS
uint8_t aux_displ;
uint8_t aux_sensor=0b00000000;
char key_val = '-';
char user;
int sensor_aux[8]={0,0,0,0,0,0,0,0};
int estado = 0;
int temp_ativo=0;
int cont_0;
int cont_ativo=0;
int cont_r=0; // contador de 10 segundos para entrar na recuperação
int cont_a=0; // contador do tempo da sirene
int RESETA = 0; // flag para recuperação
int estado_aux;
int PANIC_flag = 0;
int alerta_flag = 0; // quando algum sensor ativa bandido
int Acabou_T_sirene = 0; // acabou timer sirene
int T_ativo=10; //TEMPO DE ATIVAÇÃO
int T_timeout = 99; //TEMPO DO TIMEOUT
int T_sirene = 5; //TEMPO DA SIRENE
int check_pass = 0;
int zona[3]= {0,0,0};//PARA HABILITAR OU DESABILITAR AS 3 ZONAS
int sensor_zona[8] = {0,3,3,3,3,3,3,0};			// vetor dos sensores - 3(desassociado)/ (0 - 2) zonas associadas
int sensor_ativado[8]={0,0,0,0,0,0,0,0};
uint8_t sensor_ativado_aux;
const char key[4][4]={	{'1','2','3','P'},
{'4','5','6','A'},
{'7','8','9','D'},
{'R','0','S','E'}};
char senha[4];							
char senha1[]={'1','2','3','4'};
char senha2[]={'2','2','2','2'};
char senha3[]={'3','3','3','3'};
char senha4[]={'4','4','4','4'};

int botao = 0;
uint8_t subida = 0xf;

//---------------------------------------------------------------------------
void arq_log (char user, char data, char *hora, char *config);
char _hminuto[6]={'0','0',':','0','0', '\0'};
char dia='0';
void dly(unsigned short tempo);
void Send_Comand(unsigned char msg );
void Display_Config();
void Display_Clear();
void Send_Char( unsigned char data );
void Send_String(const char *str);
void Send_Msg (const char *str1, const char *str2);
void display_desativado();
void display_ativado();
void display_senha();
void display_sensor();
void inserirsenha();
int verificarsenha();
int verificarsenhaM();
//--------------------------
//OPERAÇÕES DE PROGRAMAÇÃO
int OP_2();
void OP_3(int A_ou_D);
void OP_4(int A_ou_D);
void OP_5(int A_ou_D);
void OP_6();
void OP_7();
void OP_8();
//-----------------------------------

void desativado();
void ativado();
void programacao();
void panico();
int senhaPadrao();
int senhaM();
void recupera();

void setar_zona (int sensor);

void display_senha_incorreta();

ISR(PCINT1_vect){
	if(estado == ATIV & temp_ativo==1 & (PINC & (1 << PINC3))==0 & alerta_flag == 0){
		 alerta_flag = 1;
		 I2C_Start();
		 I2C_Write(0x40);
		 Display_Clear();
		 Send_Comand(0x80);
		 Send_String("    ALERTA! ");
		 I2C_Stop();
		 PORTB |= (1<<PORTB5);
		 I2C_Start();
		 I2C_Write(0x43);
		 sensor_ativado_aux = I2C_Read(0);
		 uart_enviaword("entrei");
		 I2C_Stop();
		 dly(100);
		 uint8_t auxiliar;
		 uint8_t temporario;
		 for (int i = 0;i<8;i++)
		 {
			 auxiliar |= 0xff;
			 auxiliar &= ~(1 << i);
			 temporario = (~(auxiliar) & ~(sensor_ativado_aux));
			 if (temporario == 0x00){
				sensor_ativado[i] = 0;
			}
			else
			{
				sensor_ativado[i] = 1;
				if(sensor_zona[i]==0)
				{
					uart_enviaword("zona 1");
					PORTC |= (1<<PORTC0);
				}
				else if (sensor_zona[i]==1)
				{
					uart_enviaword("zona 2");
					PORTC |= (1<<PORTC1);
				}
				else if (sensor_zona[i]==2)
				{
					uart_enviaword("zona 3");
					PORTC |= (1<<PORTC2);
				}
				
			}
		 }
	}
}

ISR(PCINT2_vect){
		if (subida == 0xf){
		for(int i=0; i<4; i++){
		
			PORTB |= 0x0f;
			PORTB &= ~(1 << i);
		
			if((PIND & 0xf0)>=112){

				if ((PIND >> 4) == 0b00001110)
				{key_val= key[i][0];
				}
				else if ((PIND >> 4) == 0b00001101)
				{key_val= key[i][1];
				}
				else if ((PIND >> 4) == 0b00001011)
				{key_val= key[i][2];
				}
				else if ((PIND >> 4) == 0b00000111)
				{key_val= key[i][3];
				}
			}
		}
		PORTB &= 0xf0;
		botao = 1;
	}
	subida ^= 0xf;
}


ISR(TIMER1_COMPA_vect)
{
	TCNT1 = 0;
	if (key_val == 'R' & botao == 1){
		cont_r++;
		if(cont_r == 10)
		RESETA = 1;
		
	}else{
	cont_r = 0;
	RESETA = 0;
	}
	if (alerta_flag == 1){
		cont_a++;
		if(cont_a == T_sirene){
		alerta_flag = 0;
		Acabou_T_sirene = 1;
		PORTB &= ~(1<<PORTB5);
		cont_a = 0;
		temp_ativo=0;
		estado=ATIV;
		main();
		}
	}
	
	if (estado==ATIV & key_val=='E')
	{
		cont_ativo++;
	}
	if (estado == PROG & botao == 0)
	{
		cont_prog++;
		if (cont_prog == T_timeout & T_timeout != 0 )
		{
			estado = DESA;
		}
	}
	else if (estado == PROG & botao == 1)
	{
		cont_prog = 0;
	}
}
// ----------------------------------------- MAIN -------------------------------------------------------------------
int main(void)
{
	if(estado==0)
	{
		TCCR1B = 0x0C;            // define 256 de prescale
		TCNT1 = 0;
		OCR1A = 62500;            // define 1seg de interruption
		TIMSK1 = 0x02;            // match com OCR1A
		I2C_Init();
		uart_config();
		DDRB |= 0x2F;
		DDRD = 0x0E;
		DDRC |= 0x07;
		PCICR = 0b00000110;
		PCMSK1 = 0x08;//SENSORES
		PCMSK2 = 0xF0;//TECLADO COLUNA
		sei();
		estado_aux = estado;
		// Config do display
		I2C_Start();
		I2C_Write(0x40);
		dly(50);
		Display_Config();
		display_desativado();
		I2C_Stop();
		dly(5);
	}
	
	
	while (1)
	{
		uart_enviachar(key_val);
		
		if ((key_val == 'P' & botao ==1) || estado == PROG)  
		{
			botao=0;
			estado = PROG;
			uart_enviaword("prog");
			programacao();
			
		} 
		else if ((key_val == 'A' & botao ==1) || estado == ATIV)
		{
			botao=0;
			temp_ativo=0;
			estado = ATIV;
			uart_enviaword(" ativado ");
			ativado();
			
		}
		else if ((key_val == 'D' & botao ==1) || estado == DESA)
		{
			botao=0;
			estado = DESA;
			uart_enviaword(" desativado ");
			desativado();

		} else if ((key_val == 'S' & botao ==1) || estado == PANI)
		{
			uart_enviachar(key_val);
			botao=0;
			estado = PANI;
			uart_enviaword("pani");
			panico();
			
		}
	}
}
// ----------------------------------------- FIM DA MAIN -------------------------------------------------------------------

void dly(unsigned short tempo)
{
	int cont = 0;
	
	TCCR0A = (0X02);//configurando o modo CTC com prescale de 128
	TCCR0B = (0X03);
	OCR0A = (0X0C);//configurando o valor de comparacção para 1ms a 8Mhz com prescale de 128 (63)

	TIFR0 |= ~(1 << OCF0A); //Inicializando o flag de comparação

	while ( cont != tempo)
	{
		while ((TIFR0 & (1 << OCF0A)) == 0)
		{
		}
		cont ++;
		
		TIFR0 |= ~(1 << OCF0A);
	}
	
}

void Send_Comand(unsigned char msg )// procedimento apenas para o envio de comandos para o display 28
{
	aux_displ = 0x40 | (msg >> 4); // enviando nibble mais significativo
	I2C_Write(aux_displ);
	dly(1);
	aux_displ &= ~(1<<6); // EN = 0
	I2C_Write(aux_displ);
	dly(5);

	aux_displ = 0x40 | (msg & 0x0F); // einviando o nibble menos significativo
	I2C_Write(aux_displ);
	dly(1);
	aux_displ &= ~(1<<6); // EN = 0
	I2C_Write(aux_displ);
	dly(5);
	
}

void Display_Config(){
	dly(50);
	I2C_Write(0x42);
	dly(2);
	I2C_Write(0x02);
	dly(1);
	Send_Comand(0x28);
	Send_Comand(0x0C);// Desligando o cursor
}

void Display_Clear()
{
	Send_Comand(0x01);		// Clear display
	dly(2);
	Send_Comand(0x80);		// Cursor at home position

}

void Send_Char( unsigned char data ) //função que envia um caractere para a posição do cursor
{
	aux_displ = 0xC0 | (data >> 4); // enviando nibble mais significativo  1100000 OR 00001111
	I2C_Write(aux_displ);
	dly(1);
	aux_displ &= ~(1<<6); // EN = 0
	I2C_Write(aux_displ);
	dly(5);

	aux_displ = 0xC0 | (data & 0x0F); // einviando o nibble menos significativo
	I2C_Write(aux_displ);
	dly(1);
	aux_displ &= ~(1<<6); // EN = 0
	I2C_Write(aux_displ);
	dly(5);
}

void Send_String(const char *str)		// Send string to LCD function
{
	int i;
	for(i=0;str[i]!=0;i++)		// Send each char of string till the NULL
	{
		Send_Char (str[i]);
	}
}

void Send_Msg (const char *str1, const char *str2)		// Send string to LCD function
{
	Display_Clear();
	Send_String(str1);
	Send_Comand(0xC0);
	Send_String(str2);
}

void display_desativado(){
	I2C_Write(0x40);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("    ELE-1717");
	Send_Comand(0xC0);
	Send_String("Central - Alarme");
	dly(100);
}

void display_ativado(){
	I2C_Write(0x40);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("    Central");
	Send_Comand(0xC0);
	Send_String("    Ativada");
	dly(100);
}

void display_senha(){
	I2C_Start();
	I2C_Write(0x40);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Senha: ----");
	Send_Comand(0xC0);
	Send_String("E - Confirma"); // COLOCAR O STOP DEPOIS
}
void display_sensor(){
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Sensor(0-7): -");
	Send_Comand(0xC0);
	Send_String("E - Confirma");
	Send_Comand(0x8D);// COLOCAR O STOP DEPOIS
}

void display_senha_incorreta(){
	I2C_Start();
	I2C_Write(0x40);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("     Senha");
	Send_Comand(0xC0);
	Send_String("   Incorreta");
	I2C_Stop();
	dly(50000);
}


void arq_log (char user, char data, char *hora, char *config)
{
	uart_config();
	uart_enviaword("Registro de evento \r\n");
	uart_enviaword("User: ");
	uart_enviachar(user);
	uart_enviaword("\r\n");
	uart_enviaword("Dia da semana: ");
	uart_enviachar(data);
	uart_enviaword("\r\n");
	uart_enviaword("Hora: ");
	uart_enviaword(hora);
	uart_enviaword("\r\n");
	uart_enviaword("Operação: ");
	uart_enviaword(config);
	uart_enviaword("\r\n \r\n");
}

//=======================================================================================
//==================================INICIO SENHAS===========================================
//=======================================================================================
int senhaPadrao(){
	int num = 0;
	display_senha();
	Send_Comand(0x87);
	
	while (1){
		if (botao == 1 & num<4)
		{
			senha[num] = key_val;
			Send_String("*");
			num++;
			botao = 0;
		}
		if (num == 4 & key_val== 'E' & botao==1)
		{
			botao=0;
			break;
		}
	}
	I2C_Stop();
	dly(100);
	return verificarsenha();
}

void inserirsenha() {
	
	botao = 0;
	int x = 0;
	Send_Comand(0x8C);
	while(x<4){
		if (botao == 1)
		{	senha[x] = key_val;
			Send_String("*");
			x++;
			botao = 0;
		}
	}
};

int senhaM(){
	int num = 0;
	estado_aux = estado;
	
	I2C_Start();
	I2C_Write(0x40);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Senha (M): ----");
	Send_Comand(0xC0);
	Send_String("E - Confirma");
	Send_Comand(0x8B);
	
	
	while (1){
		if (botao == 1 & num<4)
		{
			senha[num] = key_val;
			Send_String("*");
			num++;
			botao = 0;
		}
		if (num == 4 & key_val== 'E' & botao==1)
		{
			botao=0;
			break;
		}
	}
	I2C_Stop();
	dly(100);
	return verificarsenhaM();
}


int verificarsenha() {
	if (senha[0] == senha1[0] & senha[1] == senha1[1] & senha[2] == senha1[2] & senha[3] == senha1[3])
	{	
		user='0';
		return 1;
	}else if (senha[0] == senha2[0] & senha[1] == senha2[1] & senha[2] == senha2[2] & senha[3] == senha2[3])
	{	user='1';
		return 1;
	}else if (senha[0] == senha3[0] & senha[1] == senha3[1] & senha[2] == senha3[2] & senha[3] == senha3[3])
	{	user='2';
		return 1;
	}else if (senha[0] == senha4[0] & senha[1] == senha4[1] & senha[2] == senha4[2] & senha[3] == senha4[3])
	{	user='3';
		return 1;
		} else {
		
		return 0;
	}
	
}

int verificarsenhaM() {
	if (senha[0] == senha1[0] & senha[1] == senha1[1] & senha[2] == senha1[2] & senha[3] == senha1[3])
	{	uart_enviaword("Senha 1 correta");
		return 1;
	} 
	else {
		uart_enviaword("Senha incorreta");
		return 0;
	}
}
//=======================================================================================
//==================================FIM SENHAS===========================================
//=======================================================================================

//--------------------------------RTC-----------------------

void rtc ()
{
	uint8_t aux=0x00;
	dly(100);
	I2C_Start();
	I2C_Write(0xD0);
	I2C_Write(0x01);
	I2C_Start();
	I2C_Write(0xD1);
	aux = I2C_Read(1);
	_hminuto[3]= (aux >> 4) + 48;
	_hminuto[4]= (aux & 0x0F) + 48;
	aux=I2C_Read(1);
	_hminuto[0]= (aux >> 4) + 48;
	_hminuto[1]= (aux & 0x0F) + 48;
	//------------------------
	aux = I2C_Read(0) + 48;
	dia= aux;
	I2C_Stop();
	dly(100);
}


//-----------------------------ESTADOS--------------
void desativado(){
	if (estado_aux!=DESA)
	{
		if (senhaPadrao()==0)
		{
			display_senha_incorreta();
			estado=ATIV;
			return;
		}
		else
		{
			estado_aux=DESA;
		}
	}
	else
	{
		rtc ();
		I2C_Start();
		display_desativado();
		I2C_Stop();
		dly(100);
		PORTB &= ~(1<<PORTB5);
		arq_log(user, dia, _hminuto, "desativar alarme");
		PORTD &= ~(1<<PORTD3);
		while(estado == DESA){
			if (key_val == 'A' & botao==1){
				estado_aux=DESA;
				estado = ATIV;
				botao=0;
			}
			else if (RESETA == 1){
				recupera();
			}
			else if (key_val == 'P' & botao==1){
				estado_aux=DESA;
				estado = PROG;
				botao=0;
			}else if (key_val == 'S' & botao == 1 & PANIC_flag == 0){
				botao = 0;
				PANIC_flag = 1;
				panico();
				}
		}
	}
	return;
}
	

void ativado(){
	//Acabou_T_sirene=0;
	if (estado_aux!=ATIV)
	{
		if (senhaPadrao()==0)
		{
			display_senha_incorreta();
			estado=DESA;
			return;
		}
		else
		{
			estado_aux=ATIV;
		}
	}
	
	else
	{
		rtc();
		I2C_Start();
		display_ativado();
		I2C_Stop();
		while(cont_ativo<T_ativo);
		uart_enviaword("acabou timer \r\n");
		temp_ativo=1;
		cont_ativo=0;
		arq_log(user, dia, _hminuto, "ativar alarme");
		
		PORTD |= (1<<PORTD3);
		
		while(estado == ATIV){
			
			if (key_val == 'D' & botao==1)
			{
				uart_enviaword("entrei no if D");
				estado_aux=ATIV;
				estado = DESA;
				botao=0;
			}else if (key_val == 'S' & botao == 1 & PANIC_flag == 0){
			botao = 0;
			PANIC_flag = 1;
			panico();
			}
			
			else if (RESETA == 1)
			{
				recupera();
			}
			
		}
		return;
	}
}

//************************************** INICIO PROGRAMAÇÃO ************************************************************
void programacao(){
	int prog1 = 0;
	int num = 0;
	int x=0;
	int habilitar = 0;
	
	if (senhaM() == 0){
		display_senha_incorreta();
		estado=DESA;
		estado_aux=DESA;
		return;
		}
	PORTD |= (1<<PORTD2);
	I2C_Start();
	I2C_Write(0x40);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("OP(A/D): -");
	Send_Comand(0xC0);
	Send_String("F(2-8): -");
	Send_Comand(0x89);
	dly(10000);

//espera apertar A OU D
while(prog1 == 0){
	
	if (key_val == 'A' & botao == 1){
		Send_String("A");
		habilitar = 1; //habilitar=1 apertou A, habilitar=0 apertou D
		prog1++;
		botao=0;
	}
	else if (key_val == 'D' & botao == 1){
		Send_String("D");
		prog1++;
		botao=0;
	}else if (RESETA == 1){
		recupera();
	}
}
dly(10000);
Send_Comand(0xC8);

//espera escolher operação
while(prog1 == 1){
	if (key_val == '2' & botao == 1){ // OPERAÇÃO 2 - ALTERAR SENHA
		botao=0;
		x= OP_2();
		inserirsenha();
		
		switch(x)
		{
			case 0:
				for (int i=0; i<4; i++)
				{	
					senha1[i]=senha[i];
				}
				break;
				
			case 1:
				for (int i=0; i<4; i++)
				{
					senha2[i]=senha[i];
				}
				break;
			case 2:
				for (int i=0; i<4; i++)
				{
					senha3[i]=senha[i];
				}
				break;
			case 3:
				for (int i=0; i<4; i++)
				{
					senha4[i]=senha[i];
				}
				break;	
		}
		
		while(key_val!='E');
		prog1++;
		botao=0;
		estado=DESA;
		estado_aux=DESA;
	}
	else if (key_val == '3' & botao ==1){ // OPERAÇÃO 3 - HABILITAR/DESABILITAR SENSOR
		botao=0;
		OP_3(habilitar);// atualiza os sensores
		while(key_val!='E');
		prog1++;
		botao=0;
		estado=DESA;
		estado_aux=DESA;
		
	}
		else if (key_val == '4' & botao == 1){ // OPERAÇÃO SENSOR -> ZONA
			botao=0;
			Send_Char(key_val);
			OP_4(habilitar);
			while(key_val!='E');
			prog1++;
			botao=0;
			estado=DESA;
			estado_aux=DESA;
		}
		else if (key_val == '5' & botao == 1){ // OPERAÇÃO HABILITA/DESABILITA ZONA
			botao=0;
			Send_Char(key_val);
			OP_5(habilitar);
			while(key_val!='E');
			prog1++;
			botao=0;
			estado=DESA;
			estado_aux=DESA;
		}
		else if (key_val == '6' & botao == 1){ // OPERAÇÃO TEMP. ATIVO
			botao=0;
			Send_Char(key_val);
			OP_6();
			while(key_val!='E');
			prog1++;
			botao=0;
			estado=DESA;
			estado_aux=DESA;
		}
		else if (key_val == '7' & botao == 1){ // OPERAÇÃO TEMP. TIMEOUT
			botao=0;
			Send_Char(key_val);
			OP_7();
			while(key_val!='E');
			prog1++;
			botao=0;
			estado=DESA;
			estado_aux=DESA;
		}
		else if (key_val == '8' & botao == 1){ // OPERAÇÃO TEMP. SIRENE
			botao=0;
			Send_Char(key_val);
			OP_8();
			while(key_val!='E');
			prog1++;
			botao=0;
			estado=DESA;
			estado_aux=DESA;
		}else if (RESETA == 1){
		recupera();
	}
	}
	rtc();
	I2C_Stop();
	dly(100);

	arq_log(user, dia, _hminuto, "configurar central de alarme");
	PORTD &= ~(1<<PORTD2);
	return;
}
//************************************** INICIO PROGRAMAÇÃO ************************************************************		

int OP_2()
{
	int x=0;
	Send_String("2");
	dly(10000);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Qual senha(0-3)?");
	Send_Comand(0xC0);
	Send_String("-");
	//key_val = '2'; //teste
	while (1)// INSERIR QUAL SENHA ALTERAR
	{
		if (key_val == '0' & botao==1)
		{
			Send_Comand(0xC0);
			Send_String("0");
			x=0;
			dly(10000);
			break;
		}
		else if (key_val == '1' & botao==1)
		{
			Send_Comand(0xC0);
			Send_String("1");
			x=1;
			dly(10000);
			break;
		}
		else if (key_val == '2' & botao==1)
		{
			Send_Comand(0xC0);
			Send_String("2");
			x=2;
			dly(10000);
			break;
		}
		else if (key_val == '3' & botao==1)
		{
			Send_Comand(0xC0);
			Send_String("3");
			x=3;
			dly(10000);
			break;
		}
	}
	
	dly(10000);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Nova senha: ----");
	Send_Comand(0xC0);
	Send_String("E - Confirma");
	Send_Comand(0x8C);
	return x;
}

void OP_3(int A_ou_D)
{
	Send_String("3");
	dly(10000);
	display_sensor();
	//key_val = '2'; //teste - mudar para o botao
	if (A_ou_D == 1) // Habilita algum sensor
	{
		while(botao==0);
		switch(key_val){		// ESCOLHE O SENSOR
			case '0':
			Send_Char(key_val);
			sensor_aux[0] = 1;
			break;
			case '1':
			Send_Char(key_val);
			sensor_aux[1] = 1;
			break;
			case '2':
			Send_Char(key_val);
			sensor_aux[2] = 1;
			break;
			case '3':
			Send_Char(key_val);
			sensor_aux[3] = 1;
			break;
			case '4':
			Send_Char(key_val);
			sensor_aux[4] = 1;
			break;
			case '5':
			Send_Char(key_val);
			sensor_aux[5] = 1;
			break;
			case '6':
			Send_Char(key_val);
			sensor_aux[6] = 1;
			break;
			case '7':
			Send_Char(key_val);
			sensor_aux[7] = 1;
			break;
		}
		botao=0;
		for (int i = 0;i<8;i++)
		{
			if (sensor_aux[i] == 1) //é 1?
			{
				aux_sensor |= (1 << i);
			}
		}
	}
	else
	{
		while(botao==0);
		switch(key_val){		// ESCOLHE O SENSOR
			case '0':
			Send_Char(key_val);
			sensor_aux[0] = 0;
			break;
			case '1':
			Send_Char(key_val);
			sensor_aux[1] = 0;
			break;
			case '2':
			Send_Char(key_val);
			sensor_aux[2] = 0;
			break;
			case '3':
			Send_Char(key_val);
			sensor_aux[3] = 0;
			break;
			case '4':
			Send_Char(key_val);
			sensor_aux[4] = 0;
			break;
			case '5':
			Send_Char(key_val);
			sensor_aux[5] = 0;
			break;
			case '6':
			Send_Char(key_val);
			sensor_aux[6] = 0;
			break;
			case '7':
			Send_Char(key_val);
			sensor_aux[7] = 0;
			break;
		}
		botao=0;
		
		for (int i = 0;i<8;i++)
		{
			if (sensor_aux[i] == 0)
			{
				aux_sensor &= ~(1 << i);
			}
		}
	}
	
}

void OP_4(int A_ou_D)
{
	if (A_ou_D == 1)
	{
		Display_Clear();
		Send_Comand(0x80);
		Send_String("Sensor(0-7): -");
		Send_Comand(0x8D);
		while (botao==0);
		Send_Char(key_val);
		botao=0;
		dly(10000);
		Display_Clear();
		Send_Comand(0x80);
		Send_String("Zona(1-3): -");
		Send_Comand(0xC0);
		Send_String("E - Confirma");
		Send_Comand(0x8B);
		while (botao==0);
		switch(key_val){		// ESCOLHE O SENSOR
			case '0':
			setar_zona(0);
			break;
			case '1':
			setar_zona(1);
			break;
			case '2':
			setar_zona(2);
			break;
			case '3':
			setar_zona(3);
			break;
			case '4':
			setar_zona(4);
			break;
			case '5':
			setar_zona(5);
			break;
			case '6':
			setar_zona(6);
			break;
			case '7':
			setar_zona(7);
			break;
		}
		botao=0;
	}
	else
	{
		display_sensor();
		while (botao==0);
		Send_Char(key_val);
		int sensor = key_val-48;
		sensor_zona[sensor] = 3;
	}
	
	
}


void OP_5(int A_ou_D){//OPERAÇÃO 5 - HABILITA OU DESABILITA ZONA
	dly(10000);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Zona(1-3): -");
	Send_Comand(0xC0);
	Send_String("E - Confirma");
	Send_Comand(0x8B);
	
	while(botao==0);
	switch(key_val){// ESCOLHE A ZONA
		case '1':
		Send_Char(key_val);
		zona[0] = A_ou_D;
		break;
		case '2':
		Send_Char(key_val);
		zona[1] = A_ou_D;
		break;
		case '3':
		Send_Char(key_val);
		zona[2] = A_ou_D;
		break;
	}
	botao=0;
	
}

void OP_6(){//TEMPO DE ATIVAÇÃO
	int n_tempo =0;//CONTADOR DE DÍGITOS
	int aux_cent, aux_dec, aux_uni, aux_total;
	dly(10000);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Temp. Ativo: ---");
	Send_Comand(0xC0);
	Send_String("E - Confirma");
	Send_Comand(0x8D);
	while(n_tempo <3){//DEVE SER INSERIDO OS 3 DÍGITOS
		if(botao==1){//Verificando se o botão foi apertado
			
			Send_Char(key_val);
			if(n_tempo == 0){//DIGITO DA CENTENA
				aux_cent = (key_val - 48) *100;
				uart_enviachar(key_val);
			}
			if(n_tempo == 1){//DIGITO DA DEZENA
				aux_dec = (key_val - 48) *10;
				uart_enviachar(key_val);
			}
			if(n_tempo == 2){//DIGITO DA UNIDADE
				aux_uni = (key_val - 48);
				T_ativo = aux_uni + aux_dec + aux_cent;
				uart_enviachar(key_val);
			}
			n_tempo++;
			botao=0;

		}
	}
}

void OP_7(){//TEMPO DO TIMEOUT
	int n_tempo =0;//CONTADOR DE DÍGITOS
	int aux_dec, aux_uni, aux_total;
	dly(10000);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Temp. Timeout:--");
	Send_Comand(0xC0);
	Send_String("E - Confirma");
	Send_Comand(0x8E);
	while(n_tempo <3){//DEVE SER INSERIDO OS 2 DÍGITOS
		if(botao==1){//Verificando se o botão foi apertado
			
			Send_Char(key_val);
			n_tempo++;
			if(n_tempo == 1){//DIGITO DA DEZENA
				aux_dec = (key_val - 48) *10;
			}
			if(n_tempo == 2){//DIGITO DA UNIDADE
				aux_uni = (key_val - 48);
				T_timeout = aux_uni+aux_dec;
			}
			botao=0;
		}

	}
}
void OP_8(){//TEMPO DA SIRENE
	int n_tempo =0;//CONTADOR DE DÍGITOS
	int aux_cent, aux_dec, aux_uni, aux_total;
	dly(10000);
	Display_Clear();
	Send_Comand(0x80);
	Send_String("Temp. Sirene:---");
	Send_Comand(0xC0);
	Send_String("E - Confirma");
	Send_Comand(0x8D);
	while(n_tempo <3){//DEVE SER INSERIDO OS 3 DÍGITOS
		if(botao==1){//Verificando se o botão foi apertado
			
			Send_Char(key_val);
			if(n_tempo == 0){//DIGITO DA CENTENA
				aux_cent = (key_val - 48) *100;
			}
			if(n_tempo == 1){//DIGITO DA DEZENA
				aux_dec = (key_val - 48) *10;
			}
			if(n_tempo == 2){//DIGITO DA UNIDADE
				aux_uni = (key_val - 48);
				T_sirene = aux_uni+aux_dec+aux_cent;
			}
			n_tempo++;
			botao=0;

		}
	}
}
void setar_zona (int sensor)
{
	
	Send_Char(key_val);
	if(key_val == '1' & botao == 1)
	{
		sensor_zona[sensor] = 1;
	}
	else if (key_val == '2' & botao == 1)
	{
		sensor_zona[sensor] = 2;
	}
	else if (key_val == '3' & botao == 1)
	{
		sensor_zona[sensor] = 3;
	}
	
	botao=0;
}

void panico(){
	 I2C_Start();
	 I2C_Write(0x40);
	 Display_Clear();
	 Send_Comand(0x80);
	 Send_String("    PANICO! ");
	 Send_Comand(0xC0);
	 Send_String("S - Retornar");
	 I2C_Stop();
	 dly(100);
	 PORTB |= (1<<PORTB5);
	 key_val = 'x';
	 while (key_val != 'S' & botao == 0);
	 botao = 0;
	 PANIC_flag = 0;
	 estado = DESA;
	 estado_aux = DESA;
	 desativado();
}

void recupera(){
	 botao = 0;
	 estado = 0;
	 T_ativo=0; //TEMPO DE ATIVAÇÃO
	 T_timeout = 99; //TEMPO DO TIMEOUT
	 T_sirene = 0; //TEMPO DA SIRENE
	 I2C_Start();
	 I2C_Write(0x40);
	 Display_Clear();
	 Send_Comand(0x80);
	 Send_String("    Central");
	 Send_Comand(0xC0);
	 Send_String("   Reiniciada");
	 I2C_Stop();
	 dly(17500);
	
	for (int i = 0; i<3; i++){
		 zona[i]= '0';//PARA DESABILITAR AS 3 ZONAS
	}
	for (int i = 0; i<4; i++){
		 senha1[i]= 49+i;
		 senha2[i]='F';
		 senha3[i]='F';
		 senha4[i]='F'; 
	 }
	for (int i=0; i<8; i++){
		 sensor_zona[i] = '3';			// vetor dos sensores - 3(desassociado)/ (0 - 2) zonas associadas	 
	}
	estado = DESA;
	estado_aux = DESA;
	desativado();
}
#define F_CPU      8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

//USER DEFINE
uint8_t RFID_Blue[4] = {0x90, 0x3E, 0x1D, 0x26};
uint8_t RFID_Red[4] = {0x93, 0xAA, 0xB6, 0x0F};

uint8_t PASSWORD[5] = {'1', '2', '3', '4', '5'};


#define SPI_DDR		DDRB
#define SPI_PORT	PORTB
#define SPI_PIN		PINB
#define SPI_MOSI	PB3
#define SPI_MISO	PB4
#define SPI_SS		PB2
#define SPI_SCK		PB5
#define ENABLE_CHIP() (SPI_PORT &= (~(1<<SPI_SS)))
#define DISABLE_CHIP() (SPI_PORT |= (1<<SPI_SS))

#define UART_TX_PIN PD1
#define UART_RX_PIN PD0
#define UART_BAUD_RATE 9600

#define CARD_FOUND		1
#define CARD_NOT_FOUND	2
#define ERROR			3
#define WRONG_CARD		4
#define TRUE_CARD		5

#define MAX_LEN			16

//Card types
#define Mifare_UltraLight 	0x4400
#define Mifare_One_S50		0x0400
#define Mifare_One_S70		0x0200
#define Mifare_Pro_X		0x0800
#define Mifare_DESFire		0x4403

// Mifare_One card command word
# define ATmega_REQIDL          0x26               // find the antenna area does not enter hibernation
# define ATmega_REQALL          0x52               // find all the cards antenna area
# define ATmega_ANTICOLL        0x93               // anti-collision
# define ATmega_SElECTTAG       0x93               // election card
# define ATmega_AUTHENT1A       0x60               // authentication key A
# define ATmega_AUTHENT1B       0x61               // authentication key B
# define ATmega_READ            0x30               // Read Block
# define ATmega_WRITE           0xA0               // write block
# define ATmega_DECREMENT       0xC0               // debit
# define ATmega_INCREMENT       0xC1               // recharge
# define ATmega_RESTORE         0xC2               // transfer block data to the buffer
# define ATmega_TRANSFER        0xB0               // save the data in the buffer
# define ATmega_HALT            0x50               // Sleep

//Page 0 ==> Command and Status
#define Page0_Reserved_1 	0x00
#define CommandReg			0x01
#define ComIEnReg			0x02
#define DivIEnReg			0x03
#define ComIrqReg			0x04
#define DivIrqReg			0x05
#define ErrorReg			0x06
#define Status1Reg			0x07
#define Status2Reg			0x08
#define FIFODataReg			0x09
#define FIFOLevelReg		0x0A
#define WaterLevelReg		0x0B
#define ControlReg			0x0C
#define BitFramingReg		0x0D
#define CollReg				0x0E
#define Page0_Reserved_2	0x0F
//Page 1 ==> Command
#define Page1_Reserved_1	0x10
#define ModeReg				0x11
#define TxModeReg			0x12
#define RxModeReg			0x13
#define TxControlReg		0x14
#define TxASKReg			0x15
#define TxSelReg			0x16
#define RxSelReg			0x17
#define RxThresholdReg		0x18
#define	DemodReg			0x19
#define Page1_Reserved_2	0x1A
#define Page1_Reserved_3	0x1B
#define MfTxReg				0x1C
#define MfRxReg				0x1D
#define Page1_Reserved_4	0x1E
#define SerialSpeedReg		0x1F
//Page 2 ==> CFG
#define Page2_Reserved_1	0x20
#define CRCResultReg_1		0x21
#define CRCResultReg_2		0x22
#define Page2_Reserved_2	0x23
#define ModWidthReg			0x24
#define Page2_Reserved_3	0x25
#define RFCfgReg			0x26
#define GsNReg				0x27
#define CWGsPReg			0x28
#define ModGsPReg			0x29
#define TModeReg			0x2A
#define TPrescalerReg		0x2B
#define TReloadReg_1		0x2C
#define TReloadReg_2		0x2D
#define TCounterValReg_1	0x2E
#define TCounterValReg_2 	0x2F

//command set
#define Idle_CMD 				0x00
#define Mem_CMD					0x01
#define GenerateRandomId_CMD	0x02
#define CalcCRC_CMD				0x03
#define Transmit_CMD			0x04
#define NoCmdChange_CMD			0x07
#define Receive_CMD				0x08
#define Transceive_CMD			0x0C
#define Reserved_CMD			0x0D
#define MFAuthent_CMD			0x0E
#define SoftReset_CMD			0x0F



#define TWI_W 0
#define TWI_R 1
#define TWI_START (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)
#define TWI_STOP  (1<<TWINT)|(1<<TWSTO)|(1<<TWEN)
#define TWI_READ_NACK (1<<TWINT)|(1<<TWEN)
#define TWI_READ_ACK (1<<TWINT)|(1<<TWEN)|(1<<TWEA)

#define LCD_SLA   0x27

#define Green_Led 0
#define Red_Led 1

uint8_t KeyMatrix[4][3] = {
	{'1', '2', '3'},
	{'4', '5', '6'},
	{'7', '8', '9'},
	{'*', '0', '#'}
};

uint8_t ID_Card, byte, index, state = 0;
uint8_t str[MAX_LEN];
char Key[5], key_pressed;

///////////////////////////////////////// I2C ////////////////////////////////////////////
void TWI_Init(){
	TWSR = 0x00;													//Prescaler = 1
	TWBR = 0x20;													//SCL freq = 100kHz
}
void TWI_Start(uint8_t Addr ,  uint8_t RW)
{
	TWCR = TWI_START;												//TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)
	while(!(TWCR & (1<<TWINT)));
	if((TWSR&0xF8) != 0x08) TWI_Init();

	TWDR= (Addr << 1) | RW;
	TWCR = TWI_READ_NACK;											//TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
}
void TWI_Stop()
{
	TWCR = TWI_STOP;
	TWI_Init();
}
void TWI_Read_block(uint8_t *data, uint8_t len)
{
	for(int i = 0 ; i < len - 1 ; i++)
	{
		TWCR = TWI_READ_ACK;										//TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
		while(!(TWCR & (1<<TWINT)));
		*data = TWDR;
		data++;
	}
	
	TWCR = TWI_READ_NACK;											//TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	*data = TWDR;
}
void TWI_Write_block(uint8_t Data)
{
	TWDR = Data;
	TWCR = TWI_READ_NACK;												//TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
}


/////////////////////////////////////////////////////////     LCD      //////////////////////////////////////////////////////////////////
void LCD_Command(unsigned char cmd){
	//cli();
	TWI_Start(LCD_SLA,0);
	TWI_Write_block(0x0C | (0xF0 & cmd));
	TWI_Write_block(0x08 | (0xF0 & cmd));
	TWI_Write_block(0x0C | (cmd << 4));
	TWI_Write_block(0x08 | (cmd << 4));
	_delay_ms(1);
	TWI_Stop();
	//sei();
}
void LCD_Data(unsigned char data){
	//cli();
	TWI_Start(LCD_SLA,0);
	TWI_Write_block(0x0D | (0xF0 & data));
	TWI_Write_block(0x09 | (0xF0 & data));
	TWI_Write_block(0x0D | (data << 4));
	TWI_Write_block(0x09 | (data << 4));
	_delay_ms(1);
	TWI_Stop();
	//sei();
}
void LCD_Init(){
	_delay_ms(15);
	LCD_Command(0x02);	//Home
	LCD_Command(0x28);	//Funtion Set
	LCD_Command(0x0C);	//Display ON/OFF
	LCD_Command(0x06);	//Entry Mode Set
	LCD_Command(0x01);	//Clear
	_delay_ms(5);

}
void LCD_Clear(){
	LCD_Command(0x01);
	_delay_ms(2);
}
void LCD_Char(const char *msg){
	while((*msg)!=0){
		LCD_Data(*msg);
		msg++;
	}
}
void LCD_String(const char *msg,uint8_t len){
	while(len > 0){
		len--;
		if((*msg)!=0)
		LCD_Data(*msg);
		else
		LCD_Data(' ');
		msg++;
	}
}
void LCD_SetCursor(uint8_t x , uint8_t line){
	switch(line){
		case 0:
		LCD_Command(0x80 | x);
		break;
		
		case 1:
		LCD_Command(0x80 | (x + 0x40));
		break;
		
		case 2:
		LCD_Command(0x80 | (x + 0x14));
		break;
		
		case 3:
		LCD_Command(0x80 | (x + 0x54));
		break;
	}
}




/////////////////////////////////////////////////////////     RFID      //////////////////////////////////////////////////////////////////

void SPI_Init()
{
	SPI_DDR |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
	SPCR |= (1<<SPE)|(1<<MSTR)|(1<<SPR0);//prescaler 16
}
uint8_t SPI_Transmit(uint8_t data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}
void mfrc522_write(uint8_t reg, uint8_t data)
{
	ENABLE_CHIP();
	SPI_Transmit((reg<<1)&0x7E);
	SPI_Transmit(data);
	DISABLE_CHIP();
}
uint8_t mfrc522_read(uint8_t reg)
{
	uint8_t data;
	ENABLE_CHIP();
	SPI_Transmit(((reg<<1)&0x7E)|0x80);
	data = SPI_Transmit(0x00);
	DISABLE_CHIP();
	return data;
}
void mfrc522_reset()
{
	mfrc522_write(CommandReg,SoftReset_CMD);
}
void mfrc522_init()
{
	uint8_t byte;
	mfrc522_reset();
	
	mfrc522_write(TModeReg, 0x8D);
	mfrc522_write(TPrescalerReg, 0x3E);
	mfrc522_write(TReloadReg_1, 30);
	mfrc522_write(TReloadReg_2, 0);
	mfrc522_write(TxASKReg, 0x40);
	mfrc522_write(ModeReg, 0x3D);
	
	byte = mfrc522_read(TxControlReg);
	if(!(byte&0x03))
	{
		mfrc522_write(TxControlReg,byte|0x03);
	}
}
uint8_t mfrc522_to_card(uint8_t cmd, uint8_t *send_data, uint8_t send_data_len, uint8_t *back_data, uint32_t *back_data_len)
{
	uint8_t status = ERROR;
	uint8_t irqEn = 0x00;
	uint8_t waitIRq = 0x00;
	uint8_t lastBits;
	uint8_t n;
	uint8_t	tmp;
	uint32_t i;
	switch (cmd)
	{
		case MFAuthent_CMD:
		{
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case Transceive_CMD:	//Transmit FIFO data
		{
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
		break;
	}
	n=mfrc522_read(ComIrqReg);
	mfrc522_write(ComIrqReg,n&(~0x80));
	n=mfrc522_read(FIFOLevelReg);
	mfrc522_write(FIFOLevelReg,n|0x80);
	mfrc522_write(CommandReg, Idle_CMD);
	for (i=0; i<send_data_len; i++)
	{
		mfrc522_write(FIFODataReg, send_data[i]);
	}
	mfrc522_write(CommandReg, cmd);
	if (cmd == Transceive_CMD)
	{
		n=mfrc522_read(BitFramingReg);
		mfrc522_write(BitFramingReg,n|0x80);
	}
	i = 2000;
	do
	{
		n = mfrc522_read(ComIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x01) && !(n&waitIRq));
	tmp=mfrc522_read(BitFramingReg);
	mfrc522_write(BitFramingReg,tmp&(~0x80));
	if (i != 0)
	{
		if(!(mfrc522_read(ErrorReg) & 0x1B))
		{
			status = CARD_FOUND;
			if (n & irqEn & 0x01)	status = CARD_NOT_FOUND;
			if (cmd == Transceive_CMD)
			{
				n = mfrc522_read(FIFOLevelReg);
				lastBits = mfrc522_read(ControlReg) & 0x07;
				if (lastBits)	*back_data_len = (n-1)*8 + lastBits;
				else	*back_data_len = n*8;
				if (n == 0)	n = 1;
				if (n > MAX_LEN)	n = MAX_LEN;
				//Reading the received data in FIFO
				for (i=0; i<n; i++)		back_data[i] = mfrc522_read(FIFODataReg);
			}
		}
		else	status = ERROR;
	}
	return status;
}
uint8_t	mfrc522_request(uint8_t req_mode, uint8_t * tag_type)
{
	uint8_t  status;
	uint32_t backBits;//The received data bits
	mfrc522_write(BitFramingReg, 0x07);
	tag_type[0] = req_mode;
	status = mfrc522_to_card(Transceive_CMD, tag_type, 1, tag_type, &backBits);
	if ((status != CARD_FOUND) || (backBits != 0x10))
	{
		status = ERROR;
	}
	return status;
}
uint8_t mfrc522_get_card_serial(uint8_t * serial_out)
{
	uint8_t status;
	uint8_t i;
	uint8_t serNumCheck=0;
//	uint8_t UARTtransfer=0;
	uint32_t unLen;
	mfrc522_write(BitFramingReg, 0x00);
	serial_out[0] = ATmega_ANTICOLL;
	serial_out[1] = 0x20;
	status = mfrc522_to_card(Transceive_CMD, serial_out, 2, serial_out, &unLen);
	if (status == CARD_FOUND)
	{
		for (i=0; i<4; i++)	serNumCheck ^= serial_out[i];
		// for (i=0; i<4; i++)	UARTtransfer ^= serial_out[i]; //
		if (serNumCheck != serial_out[i])	status = ERROR;
	}
	return status;
}
uint8_t DEC2BCD(uint8_t dec) {
	uint8_t low = dec%10;
	uint8_t high = dec/10;
	return (high<<4)|low;
}
uint8_t CheckCard(uint8_t *card)
{
	uint8_t byte = 0;
	byte = mfrc522_request(ATmega_REQALL,str);
	if(byte == CARD_FOUND)
	{
		byte = mfrc522_get_card_serial(str);
		if(byte == CARD_FOUND)
		{
			for(byte=0;byte<4;byte++)
			{
				if(*card != str[byte]) return WRONG_CARD;
				card++;
			}
			return TRUE_CARD;
		}
		else return CARD_NOT_FOUND;
	}
	else return CARD_NOT_FOUND;
}
void GPIO_Init()
{
	//Keypad
	DDRD = 0xF8;
	PORTD = 0x07;
	//LED
	DDRC = (1<<Green_Led)|(1<<Red_Led);
	PORTC = !((1<<Green_Led)|(1<<Red_Led));
}
void On_LED(uint8_t led)
{
	
	PORTC |= (1<<led);
}
void Off_LED(uint8_t led)
{
	PORTC &= ~(1<<led);
}
/////////////////////////////////////////////////////////     UART      //////////////////////////////////////////////////////////////////
void Uart_Init()
{
	UBRR0H = 0;
	UBRR0L = 51;
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	UCSR0B = (1<<TXEN0);
}

void Uart_Tx(uint8_t *chr, uint8_t len){
	for(int i = 0; i < len; i++)
	{
		while (!(UCSR0A &(1<<UDRE0)));
		UDR0=*chr;
		chr++;
	}
}


void Uart_byte(uint8_t data1)
{
	while (!(UCSR0A &(1<<UDRE0)));
	UDR0= data1;
}

/////////////////////////////////////////////////////////     MAIN      //////////////////////////////////////////////////////////////////

int main(void)
{
	GPIO_Init();
	TWI_Init();
	SPI_Init();
	Uart_Init();
	_delay_ms(1000);
	mfrc522_init();
	LCD_Init();
	LCD_SetCursor(4,0);
	LCD_String("DO AN 1 ",7);
	LCD_SetCursor(9,1);
	LCD_String("2113964",7);
	
		
	byte = mfrc522_read(ComIEnReg);
	mfrc522_write(ComIEnReg,byte|0x20);
	byte = mfrc522_read(DivIEnReg);
	mfrc522_write(DivIEnReg,byte|0x80);
	_delay_ms(2000);
	LCD_Clear();
	LCD_SetCursor(2,0);
	LCD_String("Wifi Enabled)",12);
	LCD_SetCursor(1,1);
	LCD_String("RFID AT System",14);

		uint8_t RIGHT[] = "A";
		uint8_t WRONG[] = "B";
		uint8_t len = sizeof(RIGHT);


	while (1)
	{	
		    
		ID_Card = CheckCard(RFID_Blue);
		if(ID_Card == TRUE_CARD) state = 1;
		else if(ID_Card == WRONG_CARD) state = 2;
		
		
		if(state == 1)	//PASS
		{
			LCD_Clear();
			LCD_SetCursor(1,0);
			LCD_String("swipe-success",13);
			On_LED(Green_Led);
			LCD_SetCursor(0,1);
			LCD_String("UID: 90 3E 1D 26)",16);
			Uart_Tx( &RIGHT, 1);
			_delay_ms(4000);
			LCD_Clear();
			Off_LED(Green_Led);
			}
		if(state == 2)	//FAIL
		{
			LCD_Clear();
			LCD_SetCursor(3,0);
			LCD_String("swipe-fail",10);
			On_LED(Red_Led);
			LCD_SetCursor(0,1);
			LCD_String("UID: 93 AA B6 0F)",16);
			Uart_Tx( &WRONG, 1);
			_delay_ms(4000);
			LCD_Clear();
			Off_LED(Red_Led);
			}
		
		if(state)
		{
			state = 0;
			index = 0;
			LCD_SetCursor(2,0);
			LCD_String("Wifi Enabled)",12);
			LCD_SetCursor(1,1);
			LCD_String("RFID AT System",14);
						
			
		}
		_delay_ms(100);
		
		
		
	}
	}




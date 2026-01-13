//-------------------------------------------------------------------------------------------------
// Alphanumeric display with HD44780 driver
// Control in 4-bit mode without reading the busy flag
// with any assignment of control signals
//-------------------------------------------------------------------------------------------------

#include "HD44780.hpp"

//-------------------------------------------------------------------------------------------------
// A function that exposes a half-byte to a data bus
//-------------------------------------------------------------------------------------------------
void _LCD_OutNibble(unsigned char nibbleToWrite)
{
if(nibbleToWrite & 0x01)
	LCD_DB4_PORT |= LCD_DB4;
else
	LCD_DB4_PORT  &= ~LCD_DB4;
	if(nibbleToWrite & 0x02)
		LCD_DB5_PORT |= LCD_DB5;
	else
		LCD_DB5_PORT  &= ~LCD_DB5;
		if(nibbleToWrite & 0x04)
			LCD_DB6_PORT |= LCD_DB6;
		else
			LCD_DB6_PORT  &= ~LCD_DB6;
			if(nibbleToWrite & 0x08)
				LCD_DB7_PORT |= LCD_DB7;
			else
				LCD_DB7_PORT  &= ~LCD_DB7;
}

//-------------------------------------------------------------------------------------------------
// Write byte function to the display (no distinction between instructions / data)
//-------------------------------------------------------------------------------------------------
void _LCD_Write(unsigned char dataToWrite)
{
LCD_E_PORT |= LCD_E;
_LCD_OutNibble(dataToWrite >> 4);
LCD_E_PORT &= ~LCD_E;
LCD_E_PORT |= LCD_E;
_LCD_OutNibble(dataToWrite);
LCD_E_PORT &= ~LCD_E;
_delay_us(50);
}

//-------------------------------------------------------------------------------------------------
// Function to write the command to the display
//-------------------------------------------------------------------------------------------------
void LCD_WriteCommand(unsigned char commandToWrite)
{
	LCD_RS_PORT &= ~LCD_RS;
	_LCD_Write(commandToWrite);
}

//-------------------------------------------------------------------------------------------------
// Function to write the data to the display
//-------------------------------------------------------------------------------------------------
void LCD_WriteData(unsigned char dataToWrite)
{
	LCD_RS_PORT |= LCD_RS;
	_LCD_Write(dataToWrite);
}

//-------------------------------------------------------------------------------------------------
// The function of displaying the inscription on the display
//-------------------------------------------------------------------------------------------------
void LCD_WriteText(char * text)
{
	while(*text)
	  LCD_WriteData(*text++);
}

//-------------------------------------------------------------------------------------------------
// Screen coordinate setting function
//-------------------------------------------------------------------------------------------------
void LCD_GoTo(unsigned char x, unsigned char y)
{
	LCD_WriteCommand(HD44780_DDRAM_SET | (x + (0x40 * y)));
}

//-------------------------------------------------------------------------------------------------
// Display screen cleaning function
//-------------------------------------------------------------------------------------------------
void LCD_Clear(void)
{
	LCD_WriteCommand(HD44780_CLEAR);
	_delay_ms(2);
}

//-------------------------------------------------------------------------------------------------
// Function to restore the initial coordinates of the display
//-------------------------------------------------------------------------------------------------
void LCD_Home(void)
{
	LCD_WriteCommand(HD44780_HOME);
	_delay_ms(2);
}

//-------------------------------------------------------------------------------------------------
// HD44780 controller initialization procedure
//-------------------------------------------------------------------------------------------------
void LCD_Initalize(void)
{
	unsigned char i;
	LCD_DB4_DIR |= LCD_DB4; // |
	LCD_DB5_DIR |= LCD_DB5; // |
	LCD_DB6_DIR |= LCD_DB6; // |> Configuration of the direction of the leads (for AVT1615 Arduino shield)
	LCD_DB7_DIR |= LCD_DB7; // |
	LCD_E_DIR 	|= LCD_E;   // |
	LCD_RS_DIR 	|= LCD_RS;  // |
	_delay_ms(15); 			// waiting for the supply voltage to stabilize
	LCD_RS_PORT &= ~LCD_RS; // resetting the RS line
	LCD_E_PORT &= ~LCD_E;   // resetting the E line

	for(i = 0; i < 3; i++){ // repeating the instruction block three times
	  LCD_E_PORT |= LCD_E;  // E = 1
	  _LCD_OutNibble(0x03); // 8-bit mode
	  LCD_E_PORT &= ~LCD_E; // E = 0
	  _delay_ms(5); 		// wait 5ms
	}

	LCD_E_PORT |= LCD_E;	// E = 1
	_LCD_OutNibble(0x02); 	// 4-bit mode
	LCD_E_PORT &= ~LCD_E; 	// E = 0

	_delay_ms(1); 			// wait 1ms
	LCD_WriteCommand(HD44780_FUNCTION_SET | HD44780_FONT5x7 | HD44780_TWO_LINE | HD44780_4_BIT); // 4-bit interface, 2-lines, signes 5x7
	LCD_WriteCommand(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_OFF); // switching off display
	LCD_WriteCommand(HD44780_CLEAR); // cleaning the DDRAM memory
	_delay_ms(2);
	LCD_WriteCommand(HD44780_ENTRY_MODE | HD44780_EM_SHIFT_CURSOR | HD44780_EM_INCREMENT);// the address incrementation and the cursor move
	LCD_WriteCommand(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_ON | HD44780_CURSOR_OFF | HD44780_CURSOR_NOBLINK); // turn on LCD without cursor and blinking
}

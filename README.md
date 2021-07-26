# 23LC1024
Small summary on implementing basic functionality to communicate with a 23LC1024 1Mbit SPI Serial SRAM

In the first phase the microcontroller needs to be prepared so that it can communicate with the SRAM. To achieve this functionality the microcontroller needs the following functions:

```C
void 
initInputSPI(void) 
{
	DDRB |= 0b10110000;
	PORTB = 0b01000000;
}
```
`initInputSPI` sets the corresponding bits for the pull-up resistors and data direction register (DDR) so that communicating with the SRAM is possible.
```C
void 
spiInit(void)
{
	initInputSPI();
	SPCR =  0b01010000;
	SPSR |= 0b00000001;
}
```
After the pull-ups and DDR have been set the SPI can be finally initialized by setting the correct bits of the following registers:
|Reg/Bit| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|--|--|--|--|--|--|--|--|--|
| SPCR | SPIE | SPE | DORD | MSTR | CPOL | CPHA | SPR1 | SPR0 |
| SPSR | SPIF | WCOL | | | | | | SPI2X |

For the microcontroller to be able to communicate over SPI the AVR bus has to be enabled by setting the SPI Enable (SPE) bit in the SPI Control Register (SPCR) to 1. Besides that, the MSTR bit will also be set to 1 in order to put the AVR in a Bus-Master configuration. For simplicity, SPI Interrupt Enable (SPIE) will not be used in this example. The Dataorder register (DORD) will be set to 0 since the SRAM expects the MSB to be sent first as per the documentation (see picture). The AVR as well as the SRAM run at a frequency of 20Mhz. In order to maximize the frequency at which the two parts communicate the SPR1, SPR0 and SPI2X registers will be set accordingly as per the documentation, which will result in a frequency that is half of that of the CPU. In this case the result will be 10Mhz. SPR1 and SPR0 are set to 0 and SPI2X is set to 1.

In order to communicate successfully over the SPI the `spiSend` and `spiReceive` functions need to be implemented:

```C
  
uint8_t 
spiSend(uint8_t data) 
{  
	enterCriticalSection(); 
	
	SPDR = data;  
	while((SPSR & 0b10000000) != 0b10000000){}  
	uint8_t receivedData = SPDR;  

	leaveCriticalSection();  
	return receivedData;  

} 
```
After entering a critical section, so that the Interrupt Service Routine (ISR) is halted from running and to protect communication, the SPI Data Register (SPDR) can be populated with the corresponding data. After that, the AVR will handle the communication. The SPI Interrupt Flag (SPIF) bit from the SPSR is set to 0 after the communication has been completed successfully. Until that is the case, the program waits in an endless loop for the transmission to complete. After successfully sending the data over the SPI the SPDR will be populated with a response and it will be returned right after leaving the critical section. That is important in order to check for different status responses if there are any.
```C
uint8_t 
spiReceive(void)
{  
	enterCriticalSection();
	
	uint8_t output = spiSend(0xFF);  
	
	leaveCriticalSection();  
	return output;  
}
```
`spiReceive` sends a dummy byte over the SPI and waits for an answer that will be returned.

Now that the basic SPI communication is complete, the protocol used by the SRAM can be fully implemented.

```C
 void 
 initSRAM(void)
 {
	 enterCriticalSection();
 
	 spiInit();
	 PORTB &= 0b11101111;
	 spiSend(1);
	 spiSend(0);
	 PORTB |= 0b00010000;
	 
	 leaveCriticalSection();
 }
```
The SRAM needs to be initialized 

<< TO BE COMPLETED >>

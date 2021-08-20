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
The SRAM needs to be initialized first by setting the correspnding AVR PORT register accordingly. This means that the corresponding bit through which the connection has been made to the external SRAM has to be set to a 0 (PORTB) so that it acts as an open drain. Now the operating mode has to be set. For simplicity, the communication is going to work in byte mode and not in page mode. This mean that the AVR has to send two bytes over the SPI: 1 and 0. The fist 1 byte is the Write Mode Register Code, which notifies the SRAM that its operating mode is going to be updated. The second one tells the SRAM that the operating mode should be set to byte mode. This way the SRAM will update its operating mode accordingly. Afterwards, the PORTB register will be reverted to its initial state.

Now that the SRAM is ready and the SPI connection works accordingly, the read/write operations for the SRAM should be implemented according to the protocol.
```C
uint8_t 
readSRAM(uint16_t addr)
{
	enterCriticalSection();
	
	PORTB &= 0b11101111;
	spiSend(0x03);
	
	spiSend(0b00000000);
	spiSend((uint8_t) (addr >> 8));
	spiSend((uint8_t) addr);
	
	MemValue output = spiReceive();
	PORTB |= 0b00010000;
	
	leaveCriticalSection();
	return output;
}
```
The read function works by first setting the corresponding bit in PORTB to an open drain, then sending the Read Code provided by the SRAM documentation over the SPI, followed by the memory address that should be returned. Since the SRAM documentation specifies that 24 bit addresses should be used, even though the AVR uses 16 bit addresses, an empty byte will be sent first, followed by the first half of the 16 bit address, and then the second half. After sending the read command and the address in question, spiReceive() will be called and it will wait until a response has arrived. Before returning said output, the PORTB register should be reverted to its initial state.

```C
void 
writeSRAM(uint16_t addr, uint8_t value)
{
	enterCriticalSection();
	
	PORTB &= 0b11101111;
	spiSend(0x02);
	
	spiSend(0b00000000);
	spiSend((uint8_t) (addr >> 8));
	spiSend((uint8_t) addr);
	
	spiSend(value);
	PORTB |= 0b00010000;

	leaveCriticalSection();
}
```
The Write function works analogous to the Read function, with the only difference that there is no output to be waiting for since this time the AVR isn't reading anything back but just sending a value over the SPI after the Write code and address have been specified.


---
The critical section functions used in this example look as follow:
```C
void 
enterCriticalSection(void) 
{
	uint8_t gIEB = ((SREG >> 7)  << 7);
	
	SREG &= 0b01111111; // Deactivate interrupts
	
	if(criticalSectionCount < 255)
	{
		criticalSectionCount++;
	}
	else
	{
		os_error("Error: Too many critical sections");
	}
	
	TIMSK2 &= 0b11111101; // Deactivate scheduler
	
	SREG |= gIEB;
}
```

```C
void 
leaveCriticalSection(void) 
{
    uint8_t gIEB = ((SREG >> 7)  << 7);
    
    SREG &= 0b01111111;
	if(criticalSectionCount > 0)
	{
		criticalSectionCount--;
	}
	else
	{
		error("Error: too many critical sections left");
	}
	
	if (criticalSectionCount == 0) 
	{
		TIMSK2 |= (1 << 1); // Activate scheduler
	}
	
    SREG |= gIEB;
}
```

// [!] THIS CODE IS TO BE USED ONLY AS A REFERENCE [!]

void 
initInputSPI(void) 
{
	DDRB |= 0b10110000;
	PORTB = 0b01000000;
}

void 
spiInit(void)
{
	initInputSPI();
	SPCR =  0b01010000;
	SPSR |= 0b00000001;
}

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

uint8_t 
spiReceive(void)
{  
	enterCriticalSection();
	
	uint8_t output = spiSend(0xFF);  
	
	leaveCriticalSection();  
	return output;  
}

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

MemValue 
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

void 
enterCriticalSection(void) {
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

void leaveCriticalSection(void) {
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>  
#include <sys/mman.h>
#include <unistd.h>
#include <sys/fcntl.h>

#define GPIO0BASE	0x6000d000
#define CLKBASE		0x60006000
#define PWMBASE		0x7000a000
#define PWMCLK0		0x110

/* 
	GPIO Controller has 4 ports, {A, B, C, D} 
	with the following configuration
*/

typedef struct {
	unsigned int CNF[4];		//0 for GPIO, 1 for SFIO
	unsigned int OE[4];
	unsigned int OUT[4];
	unsigned int IN[4];		//Input Value (read only)
	unsigned int INTSTA[4];	//interrupt status
	unsigned int INTENB[4];	//interrupt enable
	unsigned int INTSEL[4]; 	//Edge or Level
	unsigned int INTCLR[4];
} gpioCtrl;

int main(void)
{
	int fdmem = open("/dev/mem", O_RDWR | O_SYNC);
	if (fdmem == -1)
	{
		printf("Could not open /dev/mem\nTry running as sudo");
		exit(1);
	}
	
	
	int fdgpio = open("/sys/class/gpio/export", O_WRONLY);
	if(fdgpio == -1)
	{
		printf("Could not export GPIO\n");
		exit(1);
	}
	
	write(fdgpio, "13", 2);
	write(fdgpio, "12", 2);
	close(fdgpio);
	
	fdgpio = open("/sys/class/gpio/gpio13/direction", O_WRONLY);
	if(fdgpio == -1)
	{
		printf("Could not export GPIO\n");
		exit(1);
	}
	write(fdgpio, "out", 3);
	close(fdgpio);
	fdgpio = open("/sys/class/gpio/gpio12/direction", O_WRONLY);
	if(fdgpio == -1)
	{
		printf("Could not export GPIO\n");
		exit(1);
	}
	write(fdgpio, "in", 2);
	close(fdgpio);
	
	
	int pagesize = getpagesize();	//expect 0x1000
	int pagemask = pagesize - 1;	//0x0FFF
	
	void * base = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fdmem, (GPIO0BASE & ~pagemask));
	if (base == NULL) {
        perror("mmap()");
        exit(1);
    }
	void * basepwm = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fdmem, (PWMBASE & ~pagemask));
	if (basepwm == NULL) {
        perror("mmap()");
        exit(1);
    }
	
	volatile gpioCtrl * gpioBase = (volatile gpioCtrl *) base + (GPIO0BASE & pagemask);
	volatile unsigned int * pwmBase = (unsigned int *) basepwm;
	
	/* PWM Ref CLK = 48 MHz, F_PWM = (REF / 256) / (PFM + 1)
	   F_PWM = 10 kHz => PFM = 18, 0x12
	   31 = Enable, 30:16 = Width (% of 256), 12:0 = PFM
	*/
	*pwmBase = 0x00000012;
	gpioBase->CNF[1] = 0x30;
	gpioBase->OE[1] = 0x20;
	gpioBase->OUT[1] = 0x20;
	printf("here\n");
	int i = 1;
	int duty = 0;
	unsigned int gpioInput = 0;
	char update = 'k';
	
	while((update = getchar()) != 'q')
	{
		if (update == 'q')
		{
			break;
		}
		duty = ((i%10) * 25) << 16;
		printf("Duty Cycle: %d%%\t%#x\n", (10 * (duty >> 16) / 25), duty);
		printf("%p\t%#x\n", pwmBase, *pwmBase);
		gpioInput = gpioBase->IN[1];
		printf("%#x\t%d\n", gpioInput, gpioInput & 0x10);
		if(gpioInput)
		{
			*pwmBase &= 0x80000012;
    		*pwmBase = 0x80000012 | duty;
		}
		i++;
		getchar();
	}
	
	/* unmap
	*/
	*pwmBase = 0x00000012;
	gpioBase->OUT[1] = 0x0;	
	munmap(base, pagesize);
	munmap(basepwm, pagesize);
	
	close(fdmem);
	

	/*
	fdgpio = open("/sys/class/gpio/unexport", O_WRONLY);
	write(fdgpio, "12", 2);
	write(fdgpio, "13", 2);
	close(fdgpio);
	*/
	return 0;
}
	
	
	
	

	
	
	
	
	
	
	
	
	
	
	
	
	

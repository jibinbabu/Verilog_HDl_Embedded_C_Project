#include <altera_avalon_sgdma.h>
#include  <altera_avalon_sgdma_descriptor.h> //include the sgdma descriptor
#include  <altera_avalon_sgdma_regs.h>//include the sgdma registers
#include  <altera_avalon_pio_regs.h> //include the PIO registers
#include <unistd.h>
#include <stdio.h>

#include "sys/alt_stdio.h"
#include "sys/alt_irq.h"
#include "system.h"


// Function Prototypes
void rx_ethernet_isr (void *context);


// Create a receive frame
unsigned char rx_frame[1024] = { 0 };

//Other variables
int in=0, print;

// Create sgdma transmit and receive devices
alt_sgdma_dev * sgdma_tx_dev;
alt_sgdma_dev * sgdma_rx_dev;

// Allocate descriptors in the descriptor_memory (onchip memory)
alt_sgdma_descriptor tx_descriptor		__attribute__ (( section ( ".descriptor_memory" )));
alt_sgdma_descriptor tx_descriptor_end	__attribute__ (( section ( ".descriptor_memory" )));

alt_sgdma_descriptor rx_descriptor  	__attribute__ (( section ( ".descriptor_memory" )));
alt_sgdma_descriptor rx_descriptor_end  __attribute__ (( section ( ".descriptor_memory" )));


/********************************************************************************
 * This program demonstrates use of the Ethernet in the DE2i-150 board.
********************************************************************************/
int main(void){

	// Open the sgdma transmit device
	sgdma_tx_dev = alt_avalon_sgdma_open ("/dev/sgdma_tx");
	if (sgdma_tx_dev == NULL) {
		alt_printf ("Error: could not open scatter-gather dma transmit device\n");
		//return -1;
	} else alt_printf ("Opened scatter-gather dma transmit device\n");

	// Open the sgdma receive device
	sgdma_rx_dev = alt_avalon_sgdma_open ("/dev/sgdma_rx");
	if (sgdma_rx_dev == NULL) {
		alt_printf ("Error: could not open scatter-gather dma receive device\n");
		//return -1;
	} else alt_printf ("Opened scatter-gather dma receive device\n");



	// Set interrupts for the sgdma receive device
	alt_avalon_sgdma_register_callback( sgdma_rx_dev, (alt_avalon_sgdma_callback) rx_ethernet_isr, 0x00000014, NULL );

	// Create sgdma receive descriptor
	alt_avalon_sgdma_construct_stream_to_mem_desc( &rx_descriptor, &rx_descriptor_end, (alt_u32 *)rx_frame, 0, 0 );

	// Set up non-blocking transfer of sgdma receive descriptor
	alt_avalon_sgdma_do_async_transfer( sgdma_rx_dev, &rx_descriptor );



	// Triple-speed Ethernet MegaCore base address
	volatile int * tse = (int *) TSE_BASE;

	// Specify the addresses of the PHY devices to be accessed through MDIO interface
	*(tse + 0x0F) = 0x10;

	// Disable read and write transfers and wait
	*(tse + 0x02) = *(tse + 0x02) | 0x00800220;
	while ( *(tse + 0x02) != ( *(tse + 0x02) | 0x00800220 ) );


	//MAC FIFO Configuration
	*(tse + 0x09 ) = TSE_TRANSMIT_FIFO_DEPTH-16;
	*(tse + 0x0E ) = 3;
	*(tse + 0x0D ) = 8;
	*(tse + 0x07 ) =TSE_RECEIVE_FIFO_DEPTH-16;
	*(tse + 0x0C ) = 8;
	*(tse + 0x0B ) = 8;
	*(tse + 0x0A ) = 0;
	*(tse + 0x08 ) = 0;

	// Initialize the MAC address
	*(tse + 0x18) = 0x17231C00;
	*(tse + 0x19) = 0x0000CB4A;

	// MAC function configuration
	*(tse + 0x05) = 1518;
	*(tse + 0x17) = 12;
	*(tse + 0x06) = 0xFFFF;
	*(tse + 0x02) = 0x00800220;


	// Software reset the PHY chip and wait
	*(tse + 0x02) =  0x00802220;
	while ( *(tse + 0x02) != ( 0x00800220 ) );

	// Enable read and write transfers, gigabit Ethernet operation and promiscuous mode
	
	*(tse + 0x02) = *(tse + 0x02) | 0x0080023B;

	while ( *(tse + 0x02) != ( *(tse + 0x02) | 0x0080023B ) );


	while (1) {

		print=in;
		in= SWITCH_BASE; //read the input from the switch
		IOWR_ALTERA_AVALON_PIO_DATA(LED_BASE, 0x0100); //switch on or switch off the LED

		if (in==1){

			if (print != in){
				IOWR_ALTERA_AVALON_PIO_DATA(LED_BASE, 0x0100);
				alt_printf( "Switch on LED \n" );		
			}
		}
		else{
			if (print != in) {
				IOWR_ALTERA_AVALON_PIO_DATA(LED_BASE, 0x0000);
				alt_printf( "Switch off LED \n" );
			}
		}
	}

	return 0;
}

/****************************************************************************************
 * Subroutine to read incoming Ethernet frames
****************************************************************************************/
void rx_ethernet_isr (void *context)
{

	//Include your code to show the values of the source and destination addresses of the received frame. For example:
	if(in==1){
		alt_printf( "Source address: %x,%x \n", rx_frame[112], rx_frame[64]);

	}



	// Wait until receive descriptor transfer is complete
	while (alt_avalon_sgdma_check_descriptor_status(&rx_descriptor) != 0)
		;

	// Create new receive sgdma descriptor
	alt_avalon_sgdma_construct_stream_to_mem_desc( &rx_descriptor, &rx_descriptor_end, (alt_u32 *)rx_frame, 0, 0 );

	// Set up non-blocking transfer of sgdma receive descriptor
	alt_avalon_sgdma_do_async_transfer( sgdma_rx_dev, &rx_descriptor );
}



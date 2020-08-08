#include "initialisation.h"
#include "USB.h"
#include "MidiHandler.h"
#include "DACHandler.h"
#include "Config.h"
#include "uartHandler.h"

USB usb;

volatile uint32_t SysTickVal;

bool noteDown = false;

MidiData midiArray[MIDIBUFFERSIZE];		// for debugging
MidiHandler midiHandler;
DACHandler dacHandler;
Config cfg;

uint32_t debugClock = 0;
uint32_t debugClDiff = 0;

volatile uint8_t uartCmdPos = 0;
volatile char uartCmd[100];
volatile bool uartCmdRdy = false;
bool dumped = false;

extern "C" {
#include "interrupts.h"
}


void usbSerialdata(uint8_t* datain, uint32_t size) {
	uartSendStr(IntToString(size) + ":" + std::string((char*)datain, size) + "\n");
}

extern uint32_t SystemCoreClock;
int main(void)
{
	SystemInit();							// Activates floating point coprocessor and resets clock
	SystemClock_Config();					// Configure the clock and PLL
	SystemCoreClockUpdate();				// Update SystemCoreClock (system clock frequency) derived from settings of oscillators, prescalers and PLL
	InitUART();
	InitSysTick();
	usb.InitUSB();
	InitIO();

//	dacHandler.initDAC();

//	cfg.RestoreConfig();
	midiHandler.setConfig();

	// Bind the usb.dataHandler function to the midiHandler's event handler
	usb.midiDataHandler = std::bind(&MidiHandler::eventHandler, &midiHandler, std::placeholders::_1, std::placeholders::_2);
	usb.cdcDataHandler = std::bind(usbSerialdata, std::placeholders::_1, std::placeholders::_2);

	while (1)
	{
//		midiHandler.gateTimer();
//		cfg.SaveConfig();		// Checks if configuration change is pending a save

		if (GPIOC->IDR & GPIO_IDR_ID13 && !dumped) {
			GPIOB->ODR |= GPIO_ODR_OD7;
			dumped = true;
#if (USB_DEBUG)
			usb.OutputDebug();
#endif
		} else {
			GPIOB->ODR &= ~GPIO_ODR_OD7;
			dumped = false;
		}

		// Check if a UART command has been received
		if (uartCmdRdy) {
			std::stringstream ss;
			for (uint8_t c = 0; c < 100; ++c) {
				if (uartCmd[c] == 10) {
					//can.pendingCmd = ss.str();
					//uartSendString("Received: " + ss.str());
					usb.SendData((uint8_t*)uartCmd, c, usb.CDC_In);
					break;
				}
				else
					ss << uartCmd[c];
			}
			uartCmdRdy = false;
		}
	}
}


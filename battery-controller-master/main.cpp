#include "BatteryController.hpp"
#include "EthernetPowerControl.h"
#include "Debug.hpp"
//#include "canfilter.h"
BatteryController bc;

void disable_outputs() {
	//bc.stateMachine.forceTransition(BCStateMachine::State::BC_ERROR);
}

int main() {
// Turns Ethernet PHY off for energy saving
	PHY_PowerDown();
	CONSOLE.printf("\n\n");

	/*
#if 0
	wait_ms(2000);
	bc.stateMachine.output.setGndContactor(true);
	wait_ms(2000);
	bc.stateMachine.output.setGndContactor(false);
	wait_ms(2000);
	bc.stateMachine.output.setPositiveContactor(true);
	wait_ms(2000);
	bc.stateMachine.output.setPositiveContactor(false);
	wait_ms(2000);
	bc.stateMachine.output.setPrechargeContactor(true);
	wait_ms(2000);
	bc.stateMachine.output.setPrechargeContactor(false);
	wait_ms(2000);
	bc.stateMachine.output.setChargeContactor(true);
	wait_ms(2000);
	bc.stateMachine.output.setChargeContactor(false);

	for(;;);
#endif
*/

//handling over current signal from comparators. 
	InterruptIn con_en(PinDefs::CON_ENABLE);
	con_en.fall(Callback<void()>(&disable_outputs)); 

//Set Fan speed maximum
	bc.stateMachine.output.setFan2(255);

//Shunt offset calibration
	bc.input.calshunt();

//Start battery controller
	bc.run();
}

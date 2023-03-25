#ifndef BC_OUTPUT_INTERFACE_HPP
#define BC_OUTPUT_INTERFACE_HPP

#include "IOTemplates.hpp"
#include "BCPinDefs.hpp"
#include "BCConfig.hpp"
#include "Debug.hpp"
#include <mbed.h>

class BCOutputInterface {
public:
    BCOutputInterface();

    /** Synchronously enable/disable GROUND contactor state.
     *
     * @param state True to turn contactor on.
     * @return True if contactor enabled correctly.
     */
    bool setGndContactor(bool state);

    /** Synchronously enable/disable PRECHARGE contactor state.
     *
     * @param state True to turn contactor on.
     * @return True if contactor enabled correctly.
     */
    bool setPrechargeContactor(bool state);

    /** Synchronously enable/disable POSITIVE contactor state.
     *
     * Must not be switched unless car voltage rail is close to battery voltage.
     *
     * @param state True to turn contactor on.
     * @return True if contactor enabled correctly.
     */
    bool setPositiveContactor(bool state);

    /** Asynchronously enable/disable CHARGE contactor states.
     *
     * @param state True to turn contactor on
     * @return True if contactor enabled correctly.
     */
    bool setChargeContactor(bool state);

    /** Run shutdown procedure - turn off charging, wait then disable other contactors.
     * @return True on successful shutdown.
     */
    bool shutdown();

    /** Set fan 1 speed.
     *
     * @param speed 0 for stopped, 255 for full speed.
     */
    void setFan1(uint8_t speed);

    /** Set fan 2 speed.
     *
     * @param speed 0 for stopped, 255 for full speed.
     */
    void setFan2(uint8_t speed);

private:
    /** Provide control for a contactor.
     *
     * When told to switch states, will drive contactor then wait for contactor to switch.
     * If contactor does not switch after 30ms (configurable), returns error.
     *
     * @tparam drive_pin Pin that controls contactor coil.
     * @tparam feedback_pin Pin to read to check contactor movement.  Default is to ignore this pin and not check contactor status.
     * @tparam switch_delay_ms Time to wait before panicking.
     * @tparam poll_time_ms Time to wait between polls.
     */
    template<PinName drive_pin, PinName feedback_pin = drive_pin, PinName indicate_pin = drive_pin, uint16_t switch_delay_ms = 30, uint8_t poll_time_ms = 2>
    class ContactorControl {
    public:
        ContactorControl() {
            IOTemplates::makeOutput<drive_pin>();

            // Not using feedback
            if(drive_pin != feedback_pin) {
                IOTemplates::makeInput<feedback_pin>();

                // Feedback pin is high when contactor is off.
                IOTemplates::setPullupMode<feedback_pin>(PullUp);
            }

            if(indicate_pin != drive_pin)
                IOTemplates::makeOutput<indicate_pin>();
        }
        /** Synchronously set contactor state.
         *
         * @param state True to turn contactor on.
         * @return True if contactor enabled correctly.
         */
        bool setState(bool state) {
            DEBUG("Enabling contactor!");
            fastSet(state);
            DEBUG("Waiting for switch.");

            return waitSwitch(state);
        }

        /** Block until contactor matches state or timeout is reached.
         *
         * @param state State being switched to.
         * @return True if contactor switches before timeout is reached.
         */

        bool waitSwitch(bool state) {
            // Not using feedback
            if(feedback_pin == drive_pin) {
                DEBUG("Assuming successful switch!");
                return true;
            }

            // Poll the contactor aux contacts waiting for a close.
            for(int i = 0; i < switch_delay_ms / poll_time_ms; ++i)
                // Waiting for the pin to be high if we're turning the contactor off
                //  or low if we're turning it on.
                if(IOTemplates::read<feedback_pin>() != state) {
                    DEBUG("Contactor switch success!");
                    return true;
                } else
                    Thread::wait(2);

            DEBUG("Contactor switch fail!");
            // Oh no, everyone panic!
            return false;
        }

        /** Set state but don't check status.
         * @param state True to turn contactor on.
         */
        void fastSet(bool state) {
            IOTemplates::write<drive_pin>(state);
            if(drive_pin != indicate_pin)
                IOTemplates::write<indicate_pin>(state);
        }

    };

    ContactorControl<PinDefs::CON1_DRIVE, PinDefs::CON1_SENSE, LED1> conGround;
    ContactorControl<PinDefs::CON2_DRIVE, PinDefs::CON2_SENSE, LED2> conPositive;
    ContactorControl<PinDefs::CON3_DRIVE, PinDefs::CON3_SENSE, LED3> conPrecharge;
    ContactorControl<PinDefs::CON4_DRIVE, PinDefs::CON4_DRIVE, LED4> conCharge;

#ifdef FAN1
    PwmOut fan1;
#endif
    PwmOut fan2;
};

#endif

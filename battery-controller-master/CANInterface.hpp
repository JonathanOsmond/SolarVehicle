#ifndef CAN_INTERFACE_HPP
#define CAN_INTERFACE_HPP

#include <mbed.h>
#include "Debug.hpp"
#include "IOTemplates.hpp"

class CANInterface : public CAN {
    public:
        CANInterface(PinName rd, PinName td, PinName rs, unsigned int can_tx_base = 0) : CAN(rd, td), CAN_TX_BASE(can_tx_base), rsp(rs) {
            rsp = 0;
        }

        template<typename MsgType>
            void send(const MsgType * msg) {
                CANMessage outgoing(CAN_TX_BASE + MsgType::ID, (char*) msg, sizeof(MsgType));

                for(int i=0; i<4; ++i) {
                    if(write(outgoing)) {
                        // DEBUG_CAN("CAN Sent", outgoing);
                        return;
                    } else {
                        wait_us(2000);
                    }
                }
                DEBUG_CAN("Failed CAN write!", outgoing);
            }

        template<typename MsgType>
            const MsgType * parseMessage(const CANMessage * msg) {
                if(msg->len == sizeof(MsgType))
                    return (MsgType *)msg->data;

                DEBUG_CAN("Incorrect CAN packet length!", &msg);
                return NULL;
            }
    private:
        const unsigned int CAN_TX_BASE;
        DigitalOut rsp;
};

#endif

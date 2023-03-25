#ifndef IOTEMPLATES_HPP
#define IOTEMPLATES_HPP

#include <PinNames.h>

#ifndef __LPC17xx_H__
#include "LPC17xx.h"
#endif

namespace {
    template<PinName pin>
    __attribute__((always_inline)) inline constexpr uint32_t getBit() {
        return 1 << ((int)pin & 0x1F);
    }

    template<PinName pin>
    __attribute__((always_inline)) inline LPC_GPIO_TypeDef * getPort() {
        return (LPC_GPIO_TypeDef *)((int)pin & ~0x1F);
    }

    template<PinName pin>
    __attribute__((always_inline)) inline constexpr uint32_t getConfPin() {
        return 3 << ((((uint32_t)pin - (uint32_t)P0_0) & 0xF) << 1);
    }

    template<PinName pin>
    __attribute__((always_inline)) inline constexpr uint32_t getConfPortIndex() {
        return ((uint32_t)pin - (uint32_t)P0_0) >> 4;
    }

    /* clang++ doesn't particularly like messing with pointer casts in constexpr functions.
     * Unfortunately, this is something we need to do to make this work, *however*:
     * Casting a number to a pointer just happens to be about the same as looking up in an
     *  array that happens to start from memory address 0... (and since it's 0, clang++
     *  doesn't complain since clearly this is just a boring old null pointer)
     */
    constexpr volatile uint32_t * MEM_HACK = 0;
    
    /** Access peripheral RAM with bitbanding.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     */
    __attribute__((always_inline)) inline constexpr volatile uint32_t & peripheralBitband(uint32_t address, uint8_t bit) {
        return MEM_HACK[(0x22000000 + (address - 0x20000000)*32 + bit*4) / sizeof(MEM_HACK)];
    }
}

namespace IOTemplates {

    /** Make a pin function as a GPIO in the output direction.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     */
    template<PinName pin>
    __attribute__((always_inline)) inline void makeOutput() {
        static_assert(pin != NC, "Invalid pin number!");
        PINCONARRAY->PINSEL[getConfPortIndex<pin>()] &= ~getConfPin<pin>();
        getPort<pin>()->FIODIR |= getBit<pin>();
    }

    /** Configure pullup/pulldown mode.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     */
    template<PinName pin>
    __attribute__((always_inline)) inline void setPullupMode(PinMode mode) {
        static_assert(pin != NC, "Invalid pin number!");
        PINCONARRAY->PINMODE[getConfPortIndex<pin>()] &= ~getConfPin<pin>();
        // TODO: Support open drain mode!
    }

    /** Make a pin function as a GPIO in the input direction.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     */
    template<PinName pin>
    __attribute__((always_inline)) inline void makeInput() {
        static_assert(pin != NC, "Invalid pin number!");
        PINCONARRAY->PINSEL[getConfPortIndex<pin>()] &= ~getConfPin<pin>();
        getPort<pin>()->FIODIR &= ~getBit<pin>();
    }

    /** Set output pin to high value.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     */
    template<PinName pin>
    __attribute__((always_inline)) inline void set() {
        static_assert(pin != NC, "Invalid pin number!");
        getPort<pin>()->FIOSET = getBit<pin>();
    }

    /** Set output pin to low value.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     */
    template<PinName pin>
    __attribute__((always_inline)) inline void clear() {
        static_assert(pin != NC, "Invalid pin number!");
        getPort<pin>()->FIOCLR = getBit<pin>();
    }

    /** Set output pin to given value.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     */
    template<PinName pin>
    __attribute__((always_inline)) inline void write(bool value) {
        static_assert(pin != NC, "Invalid pin number!");
        peripheralBitband((uint32_t) &(getPort<pin>()->FIOPIN), ((int)pin & 0x1F)) = value;
    }

    /** Toggle output pin's value.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     */
    template<PinName pin>
    __attribute__((always_inline)) inline void toggle() {
        static_assert(pin != NC, "Invalid pin number!");
        getPort<pin>()->FIOPIN ^= getBit<pin>();
    }

    /** Read input pin's value.
     *
     * @tparam pin Pin number (LED1-LED4 or 5-30)
     * @return True if pin is high.
     */
    template<PinName pin>
    __attribute__((always_inline)) inline constexpr bool read() {
        static_assert(pin != NC, "Invalid pin number!");
        return getPort<pin>()->FIOPIN & getBit<pin>();
    }
}

#endif

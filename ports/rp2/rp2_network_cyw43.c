#include "py/runtime.h"
#include "extmod/network_cyw43.h"
#include "extmod/modnetwork.h"
#include "lib/cyw43-driver/src/cyw43.h"

void cyw43_irq_deinit(void);
void cyw43_irq_init(void);

#if CYW43_PIN_WL_DYNAMIC
// Defined in cyw43_bus_pio_spi.c
extern int cyw43_set_pins_wl(uint pins[CYW43_PIN_INDEX_WL_COUNT]);
#endif

#if CYW43_PIO_CLOCK_DIV_DYNAMIC
// Defined in cyw43_bus_pio_spi.c
extern void cyw43_set_pio_clock_divisor(uint16_t clock_div_int, uint8_t clock_div_frac);
#endif

mp_obj_t rp2_network_cyw43_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_interface, ARG_pin_on, ARG_pin_out, ARG_pin_in, ARG_pin_wake, ARG_pin_clock, ARG_pin_cs, ARG_div, ARG_div_frac };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_interface, MP_ARG_INT, {.u_int = MOD_NETWORK_STA_IF} },
        #if CYW43_PIN_WL_DYNAMIC
        { MP_QSTR_pin_on, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_out, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_in, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_wake, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_clock, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_cs, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        #endif
        #if CYW43_PIO_CLOCK_DIV_DYNAMIC
        { MP_QSTR_div, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 2 } },
        { MP_QSTR_div_frac, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 0 } },
        #endif
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Set the pins
    #if CYW43_PIN_WL_DYNAMIC
    uint pins[CYW43_PIN_INDEX_WL_COUNT] = {
        // REG_ON, OUT, IN, WAKE, CLOCK, CS
        args[ARG_pin_on].u_obj == MP_OBJ_NULL ? CYW43_DEFAULT_PIN_WL_REG_ON : mp_hal_get_pin_obj(args[ARG_pin_on].u_obj),
        args[ARG_pin_out].u_obj == MP_OBJ_NULL ? CYW43_DEFAULT_PIN_WL_DATA_OUT : mp_hal_get_pin_obj(args[ARG_pin_out].u_obj),
        args[ARG_pin_in].u_obj == MP_OBJ_NULL ? CYW43_DEFAULT_PIN_WL_DATA_IN : mp_hal_get_pin_obj(args[ARG_pin_in].u_obj),
        args[ARG_pin_wake].u_obj == MP_OBJ_NULL ? CYW43_DEFAULT_PIN_WL_HOST_WAKE : mp_hal_get_pin_obj(args[ARG_pin_wake].u_obj),
        args[ARG_pin_clock].u_obj == MP_OBJ_NULL ? CYW43_DEFAULT_PIN_WL_CLOCK : mp_hal_get_pin_obj(args[ARG_pin_clock].u_obj),
        args[ARG_pin_cs].u_obj == MP_OBJ_NULL ? CYW43_DEFAULT_PIN_WL_CS : mp_hal_get_pin_obj(args[ARG_pin_cs].u_obj),
    };
    // re-initialise cyw43
    cyw43_irq_deinit();
    cyw43_set_pins_wl(pins);
    cyw43_irq_init();
    #endif

    #if CYW43_PIO_CLOCK_DIV_DYNAMIC
    // set the pio clock divisor
    cyw43_set_pio_clock_divisor((uint16_t)args[ARG_div].u_int, (uint16_t)args[ARG_div_frac].u_int);
    #endif

    if (n_args == 0 || mp_obj_get_int(all_args[ARG_interface]) == MOD_NETWORK_STA_IF) {
        return network_cyw43_get_interface(MOD_NETWORK_STA_IF);
    } else {
        return network_cyw43_get_interface(MOD_NETWORK_AP_IF);
    }
}

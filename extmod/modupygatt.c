#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "extmod/modupygatt.h"
#include "esp_bt_defs.h"

#if MICROPY_PY_BLUETOOTH

STATIC const mp_obj_type_t gatt_tool_backend_type;

typedef struct _mp_obj_upygatt_t {
    mp_obj_base_t base;
} mp_obj_upygatt_t;

// instantiated Bluetooth object
STATIC const mp_obj_upygatt_t upygatt_obj = {
    { &gatt_tool_backend_type },
};

// Easier (hopefully tail-called) error handling.
STATIC mp_obj_t pygatt_handle_errno(int errno_) {
    if (errno_ != 0) {
        mp_raise_OSError(errno_);
    }
    return mp_const_none;
}

STATIC mp_obj_t upygatt_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    return MP_OBJ_FROM_PTR(&upygatt_obj);
}

STATIC mp_obj_t gatt_tool_backend_stop(void) {
  mp_bt_disable();
  printf("Bluetooth stop\r\n");
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(gatt_tool_backend_stop_obj, gatt_tool_backend_stop);

STATIC mp_obj_t gatt_tool_backend_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  enum { ARG_reset_on_start, ARG_initialization_timeout };

  const mp_arg_t allowed_args[] = {
      { MP_QSTR_reset_on_start, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = mp_const_true } },
      { MP_QSTR_initialization_timeout, MP_ARG_INT, { .u_int = 3 } },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

  int errno_ = mp_bt_enable();
  if (errno_ != 0) {
      mp_raise_OSError(errno_);
  }

  printf("Bluetooth start %d\r\n", n_args);
  return mp_obj_new_bool(mp_bt_is_enabled());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(gatt_tool_backend_start_obj, 0, gatt_tool_backend_start);

STATIC mp_obj_t gatt_tool_backend_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  enum { ARG_timeout, ARG_run_as_root };

  const mp_arg_t allowed_args[] = {
      { MP_QSTR_timeout, MP_ARG_INT, { .u_int = 10 } },
      { MP_QSTR_run_as_root, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = mp_const_false } },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

  mp_int_t timeout = args[ARG_timeout].u_int;

  printf("scan: %d timeout=" UINT_FMT " - %d, run_as_root=%s\r\n", n_args, timeout, timeout, mp_obj_is_true(args[ARG_run_as_root].u_obj) ? "True" : "False");
  int errno_ = mp_bt_scan();
  if (errno_ != 0) {
      mp_raise_OSError(errno_);
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(gatt_tool_backend_scan_obj, 0, gatt_tool_backend_scan);

STATIC mp_obj_t gatt_tool_backend_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  enum { ARG_device, ARG_address_type };

  const mp_arg_t allowed_args[] = {
      { MP_QSTR_device, MP_ARG_REQUIRED | MP_ARG_OBJ, { .u_obj = mp_const_none } },
      { MP_QSTR_address_type, MP_ARG_INT, {.u_int = 2 } },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

  char *addr_str = (char *)mp_obj_str_get_str(args[ARG_device].u_obj);
  esp_bd_addr_t device;

  for (int i = 0, j = 0; i < strlen(str); i += 3) {
    if (strncmp(&str[i], ":", 1) != 0) {
      device[j++] = (((str[i]%32+9)%25*16+(str[i+1]%32+9)%25) & 0xFF);
    }
    else i--;
  }

  int errno_ = mp_bt_connect(device);
  if (errno_ != 0) {
      mp_raise_OSError(errno_);
  }

  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(gatt_tool_backend_connect_obj, 1, gatt_tool_backend_connect);

STATIC mp_obj_t gatt_tool_backend_char_write_handle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  enum { ARG_handle, ARG_value, ARG_wait_for_response, ARG_timeout };

  const mp_arg_t allowed_args[] = {
      { MP_QSTR_handle, /*MP_ARG_REQUIRED | */MP_ARG_OBJ | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
      { MP_QSTR_value, /*MP_ARG_REQUIRED | */MP_ARG_OBJ | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
      { MP_QSTR_wait_for_response, MP_ARG_OBJ | MP_ARG_KW_ONLY, { .u_obj = mp_const_false } },
      { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 1 } },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

  uint16_t handle = 0x000e;
  //uint8_t* value = {0x55, 0x00, 0xFF, 0xDF, 0x24, 0x0E, 0xC6, 0x94, 0xD1, 0x97, 0x43, 0xAA};
  bool wait_for_response = args[ARG_wait_for_response].u_obj;

  int errno_ = mp_bt_char_write_handle(handle, /* value, */wait_for_response);
  if (errno_ != 0) {
      mp_raise_OSError(errno_);
  }

  return mp_const_none;
}
//STATIC MP_DEFINE_CONST_FUN_OBJ_0(gatt_tool_backend_char_write_handle_obj, gatt_tool_backend_char_write_handle);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(gatt_tool_backend_char_write_handle_obj, /*2*/0, gatt_tool_backend_char_write_handle);

STATIC mp_obj_t gatt_tool_backend(void) {
  printf("GATTToolBackend init\r\n");
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(gatt_tool_backend_obj, gatt_tool_backend);

STATIC const mp_rom_map_elem_t gatt_tool_backend_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_sendline),                              MP_ROM_PTR(&gatt_tool_backend_sendline_obj) },
    // { MP_ROM_QSTR(MP_QSTR_supports_unbonded,                      MP_ROM_PTR(&gatt_tool_backend_supports_unbonded_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),                                 MP_ROM_PTR(&gatt_tool_backend_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                                  MP_ROM_PTR(&gatt_tool_backend_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan),                                  MP_ROM_PTR(&gatt_tool_backend_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect),                               MP_ROM_PTR(&gatt_tool_backend_connect_obj) },
    // { MP_ROM_QSTR(MP_QSTR_clear_bond),                            MP_ROM_PTR(&gatt_tool_backend_clear_bond_obj) },
    // { MP_ROM_QSTR(MP_QSTR__disconnect),                           MP_ROM_PTR(&gatt_tool_backend__disconnect_obj) },
    // //@at_most_one_device
    // { MP_ROM_QSTR(MP_QSTR_disconnect),                            MP_ROM_PTR(&gatt_tool_backend_disconnect_obj) },
    // //@at_most_one_device
    // { MP_ROM_QSTR(MP_QSTR_bond),                                  MP_ROM_PTR(&gatt_tool_backend_bond_obj) },
    // { MP_ROM_QSTR(MP_QSTR__save_charecteristic_callback),         MP_ROM_PTR(&gatt_tool_backend__save_charecteristic_callback_obj) },
    // //@at_most_one_device
    // { MP_ROM_QSTR(MP_QSTR_discover_characteristics),              MP_ROM_PTR(&gatt_tool_backend_discover_characteristics_obj) },
    // { MP_ROM_QSTR(MP_QSTR__handle_notification_string),           MP_ROM_PTR(&gatt_tool_backend__handle_notification_string_obj) },
    // //@at_most_one_device
    { MP_ROM_QSTR(MP_QSTR_char_write_handle),                     MP_ROM_PTR(&gatt_tool_backend_char_write_handle_obj) },
    // //@at_most_one_device
    // { MP_ROM_QSTR(MP_QSTR_char_read),                             MP_ROM_PTR(&gatt_tool_backend_char_read_obj) },
    // //@at_most_one_device
    // { MP_ROM_QSTR(MP_QSTR_char_read_handle),                      MP_ROM_PTR(&gatt_tool_backend_char_read_handle_obj) },
    // { MP_ROM_QSTR(MP_QSTR_char_reset),                            MP_ROM_PTR(&gatt_tool_backend_char_reset_obj) },
};
STATIC MP_DEFINE_CONST_DICT(gatt_tool_backend_locals_dict, gatt_tool_backend_locals_dict_table);

STATIC const mp_obj_type_t gatt_tool_backend_type = {
    { &mp_type_type },
    .name = MP_QSTR_upygatt,
    .make_new = upygatt_make_new,
    .locals_dict = (void*)&gatt_tool_backend_locals_dict,
};

STATIC const mp_rom_map_elem_t mp_module_upygatt_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_upygatt) },

    { MP_ROM_QSTR(MP_QSTR_GATTToolBackend), MP_ROM_PTR(&gatt_tool_backend_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_upygatt_globals, mp_module_upygatt_globals_table);

const mp_obj_module_t mp_module_upygatt = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_upygatt_globals,
};

#endif //MICROPY_PY_BLUETOOTH

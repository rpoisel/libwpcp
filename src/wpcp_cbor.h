#ifndef WPCP_CBOR_H
#define WPCP_CBOR_H

#include "wpcp.h"

enum wpcp_cbor_major_type_t {
  WPCP_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER = 0,
  WPCP_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER = 1,
  WPCP_CBOR_MAJOR_TYPE_BYTE_STRING = 2,
  WPCP_CBOR_MAJOR_TYPE_TEXT_STRING = 3,
  WPCP_CBOR_MAJOR_TYPE_ARRAY = 4,
  WPCP_CBOR_MAJOR_TYPE_MAP = 5,
  WPCP_CBOR_MAJOR_TYPE_TAG = 6,
  WPCP_CBOR_MAJOR_TYPE_FLOATING_POINT_AND_SIMPLE_TYPES = 7
};

enum wpcp_cbor_additonal_bytes_t {
  WPCP_CBOR_ADDITONAL_BYTES_1 = 24,
  WPCP_CBOR_ADDITONAL_BYTES_2 = 25,
  WPCP_CBOR_ADDITONAL_BYTES_4 = 26,
  WPCP_CBOR_ADDITONAL_BYTES_8 = 27
};

enum wpcp_cbor_indefinite_length_t {
  WPCP_CBOR_INDEFINITE_LENGTH = 31
};

enum wpcp_cbor_stop_code_t {
  WPCP_CBOR_STOP_CODE = 0xff
};

enum wpcp_cbor_simple_type_t {
  WPCP_CBOR_SIMPLE_VALUE_FALSE = 20,
  WPCP_CBOR_SIMPLE_VALUE_TRUE = 21,
  WPCP_CBOR_SIMPLE_VALUE_NULL = 22,
  WPCP_CBOR_SIMPLE_VALUE_UNDEFINED = 23,
};

enum wpcp_cbor_byte_size_t {
  WPCP_CBOR_BYTE_SIZE_1 = 1,
  WPCP_CBOR_BYTE_SIZE_2 = 2,
  WPCP_CBOR_BYTE_SIZE_3 = 3,
  WPCP_CBOR_BYTE_SIZE_5 = 5
};

struct wpcp_cbor_read_buffer_t {
  const uint8_t* begin;
  const uint8_t* end;
};

struct wpcp_cbor_write_buffer_item_t {
  struct wpcp_cbor_write_buffer_item_t* next;
  size_t allocated_size;
  size_t used_size;
  uint8_t data[32];
};

struct wpcp_cbor_write_buffer_t {
  size_t used_size;
  struct wpcp_cbor_write_buffer_item_t* last_item;
  struct wpcp_cbor_write_buffer_item_t first_item;
};


void wpcp_cbor_write_buffer_init(struct wpcp_cbor_write_buffer_t* buffer);

void wpcp_cbor_write_buffer_clear(struct wpcp_cbor_write_buffer_t* buffer);

void wpcp_cbor_write_buffer_write_array_header(struct wpcp_cbor_write_buffer_t* buffer, uint32_t count);

void wpcp_cbor_write_buffer_write_boolean(struct wpcp_cbor_write_buffer_t* buffer, bool value);

void wpcp_cbor_write_buffer_write_double(struct wpcp_cbor_write_buffer_t* buffer, double value);

void wpcp_cbor_write_buffer_write_map_header(struct wpcp_cbor_write_buffer_t* buffer, uint32_t count);

void wpcp_cbor_write_buffer_write_null(struct wpcp_cbor_write_buffer_t* buffer);

void wpcp_cbor_write_buffer_write_text_string(struct wpcp_cbor_write_buffer_t* buffer, const char* data, uint32_t length);

void wpcp_cbor_write_buffer_write_unsigned_integer(struct wpcp_cbor_write_buffer_t* buffer, uint32_t value);

void wpcp_cbor_write_buffer_write_wpcp_value(struct wpcp_cbor_write_buffer_t* buffer, const struct wpcp_value_t* value);

void wpcp_cbor_write_buffer_write_additional(struct wpcp_cbor_write_buffer_t* write_buffer, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);

void wpcp_cbor_write_buffer_write_data_item(struct wpcp_cbor_write_buffer_t* write_buffer, const struct wpcp_value_t* value, double timestamp, uint32_t status, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);

void wpcp_cbor_write_buffer_write_alarm_item(struct wpcp_cbor_write_buffer_t* write_buffer, const char* key, uint32_t key_length, bool retain, const struct wpcp_value_t* token, const struct wpcp_value_t* id, double timestamp, uint32_t priority, const char* message, uint32_t message_length, bool acknowledged, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);


uint8_t* wpcp_cbor_write_array_header(uint8_t* target, uint32_t length);

uint8_t* wpcp_cbor_write_unsigned_integer(uint8_t* target, uint32_t value);

uint8_t* wpcp_cbor_write_write_buffer(uint8_t* target, const struct wpcp_cbor_write_buffer_t* buffer);



void wpcp_clear_map_items(struct wpcp_key_value_pair_t* map, uint32_t map_count, struct wpcp_key_value_pair_t* additional, uint32_t additional_count);



struct wpcp_value_t* wpcp_value_alloc(void);

void wpcp_value_free(struct wpcp_value_t* value);


bool wpcp_cbor_read_buffer_read_array_header(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* length);

bool wpcp_cbor_read_buffer_read_map_header(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* length);

const char* wpcp_cbor_read_buffer_read_text_string(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* length);

bool wpcp_cbor_read_buffer_read_unsigned_integer(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* length);

struct wpcp_value_t* wpcp_cbor_read_buffer_read_wpcp_value(struct wpcp_cbor_read_buffer_t* buffer);

bool wpcp_cbor_read_buffer_read_map_items(struct wpcp_cbor_read_buffer_t* buffer, struct wpcp_key_value_pair_t* map, uint32_t map_count, struct wpcp_key_value_pair_t** additional, uint32_t* additional_count);

#endif

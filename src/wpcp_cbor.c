#include "wpcp_cbor.h"

#include "wpcp_util.h"

#if _WIN32
#pragma warning(push, 2)
#include <winsock2.h>
#pragma warning(pop)
#else
#include <netinet/in.h>
#endif
#include <string.h>

WPCP_BEGIN_EXTERN_C

#if SIZE_MAX > UINT32_MAX
#define wpcp_cbor_write_buffer_write_type_and_size_value wpcp_cbor_write_buffer_write_type_and_uint64_value
#else
#define wpcp_cbor_write_buffer_write_type_and_size_value wpcp_cbor_write_buffer_write_type_and_uint32_value
#endif

struct wpcp_value_t* wpcp_value_alloc(void)
{
  struct wpcp_value_t* ret;
  ret = wpcp_malloc(sizeof(*ret));
  return ret;
}

void wpcp_value_free(struct wpcp_value_t* value)
{
  while (value) {
    struct wpcp_value_t* next_value = value->next;

    if (value->type == WPCP_VALUE_TYPE_ARRAY || value->type == WPCP_VALUE_TYPE_MAP || value->type == WPCP_VALUE_TYPE_TAG)
      wpcp_value_free(value->data.first_child);

    wpcp_free(value);
    value = next_value;
  }
}

void wpcp_clear_map_items(struct wpcp_key_value_pair_t* map, uint32_t map_count, struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  uint32_t i;

  for (i = 0; i < map_count; ++i) {
    wpcp_value_free(map[i].value);
    map[i].value = NULL;
  }

  for (i = 0; i < additional_count; ++i)
    wpcp_value_free(additional[i].value);

  wpcp_free(additional);
}

WPCP_STATIC_INLINE enum wpcp_cbor_byte_size_t wpcp_cbor_byte_size(uint32_t value)
{
  if (value < WPCP_CBOR_ADDITONAL_BYTES_1)
    return WPCP_CBOR_BYTE_SIZE_1;
  if (value <= 0xff)
    return WPCP_CBOR_BYTE_SIZE_2;
  if (value <= 0xffff)
    return WPCP_CBOR_BYTE_SIZE_3;
  return WPCP_CBOR_BYTE_SIZE_5;
}

WPCP_STATIC_INLINE uint8_t wpcp_cbor_construct_header(enum wpcp_cbor_major_type_t type, uint8_t value)
{
  WPCP_ASSERT(value <= 0x1f);
  return (uint8_t) type << 5 | value;
}

WPCP_STATIC_INLINE float wpcp_cbor_decode_half(uint32_t value)
{
  uint32_t sign = value & 0x8000;
  uint32_t exponent = value & 0x7c00;
  uint32_t fraction = value & 0x03ff;

  if (exponent == 0x7c00)
    exponent = 0xff << 10;
  else if (exponent)
    exponent += (127 - 15) << 10;
  else if (fraction)
    return fraction * 5.960464477539063e-8f; /* = ldexpf(fraction, -24); */

  value = sign << 16 | exponent << 13 | fraction << 13;
  return *((float*)&value);
}

const void* wpcp_cbor_read_buffer_read_raw(struct wpcp_cbor_read_buffer_t* buffer, size_t length)
{
  const uint8_t* ret = buffer->begin;
  buffer->begin += length;
  if (buffer->begin > buffer->end)
    return NULL;
  return ret;
}

WPCP_STATIC_INLINE bool wpcp_cbor_read_buffer_read_break(struct wpcp_cbor_read_buffer_t* buffer)
{
  if (buffer->begin == buffer->end || buffer->begin[0] != WPCP_CBOR_STOP_CODE)
    return false;
  buffer->begin +=1;
  return true;
}

WPCP_STATIC_INLINE bool wpcp_cbor_read_buffer_read_uint8(struct wpcp_cbor_read_buffer_t* buffer, uint8_t* value)
{
  const uint8_t* ret = wpcp_cbor_read_buffer_read_raw(buffer, sizeof(uint8_t));
  if (!ret)
    return false;
  *value = ret[0];
  return true;
}

WPCP_STATIC_INLINE bool wpcp_cbor_read_buffer_read_uint16(struct wpcp_cbor_read_buffer_t* buffer, uint16_t* value)
{
  const uint16_t* ret = wpcp_cbor_read_buffer_read_raw(buffer, sizeof(uint16_t));
  if (!ret)
    return false;
  *value = ntohs(ret[0]);
  return true;
}

WPCP_STATIC_INLINE bool wpcp_cbor_read_buffer_read_uint32(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* value)
{
  const uint32_t* ret = wpcp_cbor_read_buffer_read_raw(buffer, sizeof(uint32_t));
  if (!ret)
    return false;
  *value = ntohl(ret[0]);
  return true;
}

WPCP_STATIC_INLINE bool wpcp_cbor_read_buffer_read_uint64(struct wpcp_cbor_read_buffer_t* buffer, uint64_t* value)
{
  const uint32_t* ret = wpcp_cbor_read_buffer_read_raw(buffer, sizeof(uint64_t));
  if (!ret)
    return false;
  *value = ((uint64_t)ntohl(ret[0])) << 32 | ntohl(ret[1]);
  return true;
}

bool wpcp_cbor_read_buffer_read_header_and_uint32(struct wpcp_cbor_read_buffer_t* buffer, enum wpcp_cbor_major_type_t* type, uint32_t* value)
{
  uint8_t first_byte;
  if (!wpcp_cbor_read_buffer_read_uint8(buffer, &first_byte))
    return false;

  *type = (enum wpcp_cbor_major_type_t) first_byte >> 5;
  first_byte &= 0x1f;

  if (first_byte < WPCP_CBOR_ADDITONAL_BYTES_1) {
    *value = first_byte;
  } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_1) {
    uint8_t temp;
    if (!wpcp_cbor_read_buffer_read_uint8(buffer, &temp))
      return false;
    *value = temp;
  } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_2) {
    uint16_t temp;
    if (!wpcp_cbor_read_buffer_read_uint16(buffer, &temp))
      return false;
    *value = temp;
  } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_4) {
    if (!wpcp_cbor_read_buffer_read_uint32(buffer, value))
      return false;
  } else
    return false;

  return true;
}

WPCP_STATIC_INLINE bool wpcp_cbor_read_buffer_read_specific_header_and_uint32(struct wpcp_cbor_read_buffer_t* buffer, enum wpcp_cbor_major_type_t type, uint32_t* value)
{
  enum wpcp_cbor_major_type_t major_type;

  if (!wpcp_cbor_read_buffer_read_header_and_uint32(buffer, &major_type, value))
    return false;

  return major_type == type;
}

bool wpcp_cbor_read_buffer_read_array_header(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* length)
{
  return wpcp_cbor_read_buffer_read_specific_header_and_uint32(buffer, WPCP_CBOR_MAJOR_TYPE_ARRAY, length);
}

bool wpcp_cbor_read_buffer_read_map_header(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* length)
{
  return wpcp_cbor_read_buffer_read_specific_header_and_uint32(buffer, WPCP_CBOR_MAJOR_TYPE_MAP, length);
}

const char* wpcp_cbor_read_buffer_read_text_string(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* length)
{
  if (!wpcp_cbor_read_buffer_read_specific_header_and_uint32(buffer, WPCP_CBOR_MAJOR_TYPE_TEXT_STRING, length))
    return false;

  return wpcp_cbor_read_buffer_read_raw(buffer, *length);
}

bool wpcp_cbor_read_buffer_read_unsigned_integer(struct wpcp_cbor_read_buffer_t* buffer, uint32_t* value)
{
  return wpcp_cbor_read_buffer_read_specific_header_and_uint32(buffer, WPCP_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value);
}

struct wpcp_value_t* wpcp_cbor_read_buffer_read_string(struct wpcp_cbor_read_buffer_t* buffer, struct wpcp_value_t* allocated_value, uint8_t first_byte, uint64_t length)
{
  struct wpcp_value_t* ret = allocated_value;

  if (first_byte == WPCP_CBOR_INDEFINITE_LENGTH) {
    struct wpcp_value_t* first = NULL;
    struct wpcp_value_t** last = &first;

    while (!wpcp_cbor_read_buffer_read_break(buffer)) {
      *last = wpcp_cbor_read_buffer_read_wpcp_value(buffer);
      if (!(*last) || (*last)->type != ret->type) {
        ret = NULL;
        break;
      }
      length += (*last)->value.length;
      last = &((*last)->next);
    }

    if (ret) {
      size_t offset = sizeof(*ret);
      ret = wpcp_realloc(ret, offset + length);
      if (ret) {
        WPCP_ASSERT(length <= SIZE_MAX);
        ret->value.length = (size_t) length;
        ret->data.byte_string = ret + 1;
        while (first) {
          struct wpcp_value_t* next = first->next;
          WPCP_ASSERT(first->type == ret->type);
          memcpy(((uint8_t*)ret) + offset, first->data.byte_string, first->value.length);
          offset += first->value.length;
          wpcp_free(first);
          first = next;
        }
      }
    }

    wpcp_value_free(first);
  } else {
    WPCP_ASSERT(length <= SIZE_MAX);
    ret->value.length = (size_t) length;
    ret->data.byte_string = wpcp_cbor_read_buffer_read_raw(buffer, ret->value.length);
    if (!ret->data.byte_string)
      ret = NULL;
  }

  if (!ret)
    wpcp_value_free(allocated_value);

  return ret;
}

struct wpcp_value_t* wpcp_cbor_read_buffer_read_wpcp_values(struct wpcp_cbor_read_buffer_t* buffer, struct wpcp_value_t* allocated_value, uint8_t first_byte, uint64_t length)
{
  struct wpcp_value_t* ret = allocated_value;
  struct wpcp_value_t* last = NULL;
  ret->value.length = 0;
  ret->data.first_child = NULL;

  for (;;) {
    if (first_byte == WPCP_CBOR_INDEFINITE_LENGTH) {
      if (wpcp_cbor_read_buffer_read_break(buffer))
        break;
    } else {
      if (ret->value.length == length)
        break;
    }

    struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(buffer);
    if (!value) {
      ret = NULL;
      break;
    }

    if (last)
      last->next = value;
    else
      ret->data.first_child = value;
    last = value;
    ret->value.length += 1;
  }

  if (ret && ret->type == WPCP_VALUE_TYPE_MAP) {
    if (ret->value.length % 2)
      ret = NULL;
    else
      ret->value.length /= 2;
  }

  if (!ret)
    wpcp_value_free(allocated_value);

  return ret;
}

struct wpcp_value_t* wpcp_cbor_read_buffer_read_wpcp_value(struct wpcp_cbor_read_buffer_t* buffer)
{
  uint8_t first_byte;
  uint64_t value;
  struct wpcp_value_t* ret;
  enum wpcp_cbor_major_type_t type;

  if (!wpcp_cbor_read_buffer_read_uint8(buffer, &first_byte))
    return NULL;

  type = (enum wpcp_cbor_major_type_t) first_byte >> 5;
  first_byte &= 0x1f;

  if (first_byte < WPCP_CBOR_ADDITONAL_BYTES_1) {
    value = first_byte;
  } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_1) {
    uint8_t temp;
    if (!wpcp_cbor_read_buffer_read_uint8(buffer, &temp))
      return NULL;
    value = temp;
  } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_2) {
    uint16_t temp;
    if (!wpcp_cbor_read_buffer_read_uint16(buffer, &temp))
      return NULL;
    value = temp;
  } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_4) {
    uint32_t temp;
    if (!wpcp_cbor_read_buffer_read_uint32(buffer, &temp))
      return NULL;
    value = temp;
  } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_8) {
    if (!wpcp_cbor_read_buffer_read_uint64(buffer, &value))
      return NULL;
#if SIZE_MAX < UINT64_MAX
    if (type == WPCP_VALUE_TYPE_BYTE_STRING || type == WPCP_VALUE_TYPE_TEXT_STRING || type == WPCP_VALUE_TYPE_ARRAY || type == WPCP_VALUE_TYPE_MAP) {
      if (value > SIZE_MAX)
        return NULL;
    }
#endif
  } else if (first_byte == WPCP_CBOR_INDEFINITE_LENGTH) {
    if (type == WPCP_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER || type == WPCP_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER || type == WPCP_CBOR_MAJOR_TYPE_TAG || type == WPCP_CBOR_MAJOR_TYPE_FLOATING_POINT_AND_SIMPLE_TYPES)
      return NULL;
    value = 0;
  } else
    return NULL;

  ret = wpcp_value_alloc();

  if (!ret)
    return NULL;

  ret->next = NULL;

  switch (type) {
  case WPCP_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER:
    ret->type = WPCP_VALUE_TYPE_UINT64;
    ret->value.uint = value;
    break;

  case WPCP_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER:
    ret->type = WPCP_VALUE_TYPE_INT64;
    ret->value.sint = ~value;
    break;

  case WPCP_CBOR_MAJOR_TYPE_BYTE_STRING:
    ret->type = WPCP_VALUE_TYPE_BYTE_STRING;
    ret = wpcp_cbor_read_buffer_read_string(buffer, ret, first_byte, value);
    break;

  case WPCP_CBOR_MAJOR_TYPE_TEXT_STRING:
    ret->type = WPCP_VALUE_TYPE_TEXT_STRING;
    ret = wpcp_cbor_read_buffer_read_string(buffer, ret, first_byte, value);
    break;

  case WPCP_CBOR_MAJOR_TYPE_ARRAY:
    ret->type = WPCP_VALUE_TYPE_ARRAY;
    ret = wpcp_cbor_read_buffer_read_wpcp_values(buffer, ret, first_byte, value);
    break;

  case WPCP_CBOR_MAJOR_TYPE_MAP:
    ret->type = WPCP_VALUE_TYPE_MAP;
    ret = wpcp_cbor_read_buffer_read_wpcp_values(buffer, ret, first_byte, value * 2);
    break;

  case WPCP_CBOR_MAJOR_TYPE_TAG:
    ret->type = WPCP_VALUE_TYPE_TAG;
    ret->value.uint = value;
    ret->data.first_child = wpcp_cbor_read_buffer_read_wpcp_value(buffer);
    if (!ret->data.first_child) {
      wpcp_value_free(ret);
      ret = NULL;
    }
    break;

  case WPCP_CBOR_MAJOR_TYPE_FLOATING_POINT_AND_SIMPLE_TYPES:
    if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_2) {
      ret->type = WPCP_VALUE_TYPE_FLOAT;
      ret->value.flt = wpcp_cbor_decode_half((uint32_t) value);
    } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_4) {
      uint32_t value32 = (uint32_t) value;
      ret->type = WPCP_VALUE_TYPE_FLOAT;
      ret->value.flt = *((float*) &value32);
    } else if (first_byte == WPCP_CBOR_ADDITONAL_BYTES_8) {
      ret->type = WPCP_VALUE_TYPE_DOUBLE;
      ret->value.dbl = *((double*) &value);
    } else {
      switch (value) {
      case WPCP_CBOR_SIMPLE_VALUE_FALSE:
        ret->type = WPCP_VALUE_TYPE_FALSE;
        break;

      case WPCP_CBOR_SIMPLE_VALUE_TRUE:
        ret->type = WPCP_VALUE_TYPE_TRUE;
        break;

      case WPCP_CBOR_SIMPLE_VALUE_NULL:
        ret->type = WPCP_VALUE_TYPE_NULL;
        break;

      case WPCP_CBOR_SIMPLE_VALUE_UNDEFINED:
        ret->type = WPCP_VALUE_TYPE_UNDEFINED;
        break;

      default:
        ret->type = WPCP_VALUE_TYPE_SIMPLE_VALUE;
        ret->value.uint = value;
        break;
      }
    }
    break;
  }

  return ret;
}

bool wpcp_cbor_read_buffer_read_map_items(struct wpcp_cbor_read_buffer_t* buffer, struct wpcp_key_value_pair_t* map, uint32_t map_count, struct wpcp_key_value_pair_t** additional, uint32_t* additional_count)
{
  uint32_t i, j;
  uint32_t item_count;

  *additional = NULL;
  *additional_count = 0;

  if (!wpcp_cbor_read_buffer_read_map_header(buffer, &item_count))
    return false;

  for (i = 0; i < item_count; ++i) {
    uint32_t key_length;
    struct wpcp_value_t* value;
    const char* key = wpcp_cbor_read_buffer_read_text_string(buffer, &key_length);
    if (!key)
      return false;

    value =  wpcp_cbor_read_buffer_read_wpcp_value(buffer);
    if (!value)
      return false;

    for (j = 0; j < map_count; ++j) {
      if (map[j].key_length == key_length && !memcmp(key, map[j].key, key_length)) {
        wpcp_value_free(map[j].value);
        map[j].value = value;
        break;
      }
    }

    if (j == map_count) {
      struct wpcp_key_value_pair_t* item;

      if (!*additional)
        *additional = wpcp_malloc((item_count - i) * sizeof(*additional[0]));

      if (!*additional) {
        wpcp_value_free(value);
        return false;
      }

      item = *additional + *additional_count;
      item->key = key;
      item->key_length = key_length;
      item->value = value;
      *additional_count += 1;
    }
  }

  return true;
}

WPCP_STATIC_INLINE uint8_t* wpcp_cbor_write_header(uint8_t* target, uint8_t header)
{
  target[0] = header;
  return target + sizeof(header);
}

WPCP_STATIC_INLINE uint8_t* wpcp_cbor_write_header_and_uint8(uint8_t* target, uint8_t header, uint8_t value)
{
  target = wpcp_cbor_write_header(target, header);
  target[0] = value;
  return target + sizeof(value);
}

WPCP_STATIC_INLINE uint8_t* wpcp_cbor_write_header_and_uint16(uint8_t* target, uint8_t header, uint16_t value)
{
  target = wpcp_cbor_write_header(target, header);
  ((uint16_t*)target)[0] = htons(value);
  return target + sizeof(value);
}

WPCP_STATIC_INLINE uint8_t* wpcp_cbor_write_header_and_uint32(uint8_t* target, uint8_t header, uint32_t value)
{
  target = wpcp_cbor_write_header(target, header);
  ((uint32_t*)target)[0] = htonl(value);
  return target + sizeof(value);
}

WPCP_STATIC_INLINE uint8_t* wpcp_cbor_write_header_and_uint64(uint8_t* target, uint8_t header, uint64_t value)
{
  target = wpcp_cbor_write_header(target, header);
  ((uint32_t*)target)[0] = htonl(value >> 32);
  ((uint32_t*)target)[1] = htonl((uint32_t) value);
  return target + sizeof(value);
}

WPCP_STATIC_INLINE uint8_t* wpcp_cbor_write_type_with_value(uint8_t* target, enum wpcp_cbor_major_type_t type, uint32_t value)
{
  uint8_t first_byte = wpcp_cbor_construct_header(type, 0);

  switch (wpcp_cbor_byte_size(value)) {
  case WPCP_CBOR_BYTE_SIZE_1:
    return wpcp_cbor_write_header(target, first_byte | (uint8_t) value);

  case WPCP_CBOR_BYTE_SIZE_2:
    return wpcp_cbor_write_header_and_uint8(target, first_byte | WPCP_CBOR_ADDITONAL_BYTES_1, (uint8_t) value);

  case WPCP_CBOR_BYTE_SIZE_3:
    return wpcp_cbor_write_header_and_uint16(target, first_byte | WPCP_CBOR_ADDITONAL_BYTES_2, (uint16_t) value);

  case WPCP_CBOR_BYTE_SIZE_5:
    return wpcp_cbor_write_header_and_uint32(target, first_byte | WPCP_CBOR_ADDITONAL_BYTES_4, value);

#ifndef WPCP_DISABLE_ASSERT
  default:
    WPCP_ASSERT(false);
    return NULL;
#endif
  }
}

uint8_t* wpcp_cbor_write_array_header(uint8_t* target, uint32_t length)
{
  return wpcp_cbor_write_type_with_value(target, WPCP_CBOR_MAJOR_TYPE_ARRAY, length);
}

uint8_t* wpcp_cbor_write_unsigned_integer(uint8_t* target, uint32_t value)
{
  return wpcp_cbor_write_type_with_value(target, WPCP_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value);
}

uint8_t* wpcp_cbor_write_write_buffer(uint8_t* target, const struct wpcp_cbor_write_buffer_t* buffer)
{
  const struct wpcp_cbor_write_buffer_item_t* item = &buffer->first_item;

  if (!buffer->last_item)
    return NULL;

  do {
    memcpy(target, item->data, item->used_size);
    target += item->used_size;
    item = item->next;
  } while (item);

  return target;
}

static uint8_t* wpcp_cbor_write_buffer_ensure_space(struct wpcp_cbor_write_buffer_t* buffer, size_t size)
{
  struct wpcp_cbor_write_buffer_item_t* last_item = buffer->last_item;

  if (last_item) {
    size_t free_space = last_item->allocated_size - last_item->used_size;
    uint8_t* target = last_item->data + last_item->used_size;
    buffer->used_size += size;

    if (size > free_space) {
      size_t allocated_size = size > 256 ? size : 256;
      struct wpcp_cbor_write_buffer_item_t* new_item;
      new_item = wpcp_malloc(sizeof(*new_item) - sizeof(new_item->data) + allocated_size);
      buffer->last_item = new_item;
      if (!new_item)
        return NULL;

      new_item->next = NULL;
      new_item->allocated_size = allocated_size;
      new_item->used_size = size;
      last_item->next = new_item;
      return new_item->data;
    }
    else
      last_item->used_size += size;

    return target;
  }

  return NULL;
}

void wpcp_cbor_write_buffer_init(struct wpcp_cbor_write_buffer_t* buffer)
{
  buffer->used_size = 0;
  buffer->last_item = &buffer->first_item;
  buffer->first_item.allocated_size = sizeof(buffer->first_item.data);
  buffer->first_item.used_size = 0;
  buffer->first_item.next = NULL;
}

void wpcp_cbor_write_buffer_clear(struct wpcp_cbor_write_buffer_t* buffer)
{
  struct wpcp_cbor_write_buffer_item_t* item = buffer->first_item.next;

  while (item) {
    struct wpcp_cbor_write_buffer_item_t* next_item = item->next;
    wpcp_free(item);
    item = next_item;
  }
}

void wpcp_cbor_write_buffer_write_raw(struct wpcp_cbor_write_buffer_t* buffer, const void* data, size_t size)
{
  uint8_t* target = wpcp_cbor_write_buffer_ensure_space(buffer, size);
  if (!target)
    return;
  memcpy(target, data, size);
}

WPCP_STATIC_INLINE void wpcp_cbor_write_buffer_write_header(struct wpcp_cbor_write_buffer_t* buffer, uint8_t header)
{
  uint8_t* target = wpcp_cbor_write_buffer_ensure_space(buffer, 1);
  if (!target)
    return;
  wpcp_cbor_write_header(target, header);
}

WPCP_STATIC_INLINE void wpcp_cbor_write_buffer_write_header_and_uint8(struct wpcp_cbor_write_buffer_t* buffer, uint8_t header, uint8_t value)
{
  uint8_t* target = wpcp_cbor_write_buffer_ensure_space(buffer, 2);
  if (!target)
    return;
  wpcp_cbor_write_header_and_uint8(target, header, value);
}

WPCP_STATIC_INLINE void wpcp_cbor_write_buffer_write_header_and_uint16(struct wpcp_cbor_write_buffer_t* buffer, uint8_t header, uint16_t value)
{
  uint8_t* target = wpcp_cbor_write_buffer_ensure_space(buffer, 3);
  if (!target)
    return;
  wpcp_cbor_write_header_and_uint16(target, header, value);
}

WPCP_STATIC_INLINE void wpcp_cbor_write_buffer_write_header_and_uint32(struct wpcp_cbor_write_buffer_t* buffer, uint8_t header, uint32_t value)
{
  uint8_t* target = wpcp_cbor_write_buffer_ensure_space(buffer, 5);
  if (!target)
    return;
  wpcp_cbor_write_header_and_uint32(target, header, value);
}

WPCP_STATIC_INLINE void wpcp_cbor_write_buffer_write_header_and_uint64(struct wpcp_cbor_write_buffer_t* buffer, uint8_t header, uint64_t value)
{
  uint8_t* target = wpcp_cbor_write_buffer_ensure_space(buffer, 9);
  if (!target)
    return;
  wpcp_cbor_write_header_and_uint64(target, header, value);
}

void wpcp_cbor_write_buffer_write_type_and_uint8_value(struct wpcp_cbor_write_buffer_t* buffer, enum wpcp_cbor_major_type_t type, uint8_t value)
{
  if (value < WPCP_CBOR_ADDITONAL_BYTES_1)
    wpcp_cbor_write_buffer_write_header(buffer, wpcp_cbor_construct_header(type, value));
  else
    wpcp_cbor_write_buffer_write_header_and_uint8(buffer, wpcp_cbor_construct_header(type, WPCP_CBOR_ADDITONAL_BYTES_1), value);
}

void wpcp_cbor_write_buffer_write_type_and_uint32_value(struct wpcp_cbor_write_buffer_t* buffer, enum wpcp_cbor_major_type_t type, uint32_t value)
{
  if (value < WPCP_CBOR_ADDITONAL_BYTES_1)
    wpcp_cbor_write_buffer_write_header(buffer, wpcp_cbor_construct_header(type, (uint8_t) value));
  else if (value <= 0xff)
    wpcp_cbor_write_buffer_write_header_and_uint8(buffer, wpcp_cbor_construct_header(type, WPCP_CBOR_ADDITONAL_BYTES_1), (uint8_t) value);
  else if (value <= 0xffff)
    wpcp_cbor_write_buffer_write_header_and_uint16(buffer, wpcp_cbor_construct_header(type, WPCP_CBOR_ADDITONAL_BYTES_2), (uint16_t) value);
  else
    wpcp_cbor_write_buffer_write_header_and_uint32(buffer, wpcp_cbor_construct_header(type, WPCP_CBOR_ADDITONAL_BYTES_4), value);
}

void wpcp_cbor_write_buffer_write_type_and_uint64_value(struct wpcp_cbor_write_buffer_t* buffer, enum wpcp_cbor_major_type_t type, uint64_t value)
{
  if (value <= 0xffffffff)
    wpcp_cbor_write_buffer_write_type_and_uint32_value(buffer, type, (uint32_t) value);
  else
    wpcp_cbor_write_buffer_write_header_and_uint64(buffer, wpcp_cbor_construct_header(type, WPCP_CBOR_ADDITONAL_BYTES_8), value);
}

void wpcp_cbor_write_buffer_write_float(struct wpcp_cbor_write_buffer_t* buffer, float value)
{
  uint32_t uint = *((uint32_t*)&value);
  wpcp_cbor_write_buffer_write_header_and_uint32(buffer, wpcp_cbor_construct_header(WPCP_CBOR_MAJOR_TYPE_FLOATING_POINT_AND_SIMPLE_TYPES, WPCP_CBOR_ADDITONAL_BYTES_4), uint);
}

void wpcp_cbor_write_buffer_write_double(struct wpcp_cbor_write_buffer_t* buffer, double value)
{
  uint64_t uint = *((uint64_t*)&value);
  wpcp_cbor_write_buffer_write_header_and_uint64(buffer, wpcp_cbor_construct_header(WPCP_CBOR_MAJOR_TYPE_FLOATING_POINT_AND_SIMPLE_TYPES, WPCP_CBOR_ADDITONAL_BYTES_8), uint);
}

void wpcp_cbor_write_buffer_write_array_header(struct wpcp_cbor_write_buffer_t* buffer, uint32_t length)
{
  wpcp_cbor_write_buffer_write_type_and_uint32_value(buffer, WPCP_CBOR_MAJOR_TYPE_ARRAY, length);
}

void wpcp_cbor_write_buffer_write_map_header(struct wpcp_cbor_write_buffer_t* buffer, uint32_t length)
{
  wpcp_cbor_write_buffer_write_type_and_uint32_value(buffer, WPCP_CBOR_MAJOR_TYPE_MAP, length);
}

void wpcp_cbor_write_buffer_write_unsigned_integer(struct wpcp_cbor_write_buffer_t* buffer, uint32_t value)
{
  wpcp_cbor_write_buffer_write_type_and_uint32_value(buffer, WPCP_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value);
}

void wpcp_cbor_write_buffer_write_text_string(struct wpcp_cbor_write_buffer_t* buffer, const char* data, uint32_t length)
{
  wpcp_cbor_write_buffer_write_type_and_uint32_value(buffer, WPCP_CBOR_MAJOR_TYPE_TEXT_STRING, length);
  wpcp_cbor_write_buffer_write_raw(buffer, data, length);
}

void wpcp_cbor_write_buffer_write_simple_type(struct wpcp_cbor_write_buffer_t* buffer, enum wpcp_cbor_simple_type_t value)
{
  wpcp_cbor_write_buffer_write_header(buffer, wpcp_cbor_construct_header(WPCP_CBOR_MAJOR_TYPE_FLOATING_POINT_AND_SIMPLE_TYPES, value));
}

void wpcp_cbor_write_buffer_write_boolean(struct wpcp_cbor_write_buffer_t* buffer, bool value)
{
  wpcp_cbor_write_buffer_write_simple_type(buffer, value ? WPCP_CBOR_SIMPLE_VALUE_TRUE : WPCP_CBOR_SIMPLE_VALUE_FALSE);
}

void wpcp_cbor_write_buffer_write_null(struct wpcp_cbor_write_buffer_t* buffer)
{
  wpcp_cbor_write_buffer_write_simple_type(buffer, WPCP_CBOR_SIMPLE_VALUE_NULL);
}

void wpcp_cbor_write_buffer_write_wpcp_values(struct wpcp_cbor_write_buffer_t* result, uint64_t count, const struct wpcp_value_t* value)
{
  while (count--) {
    WPCP_ASSERT(value);
    wpcp_cbor_write_buffer_write_wpcp_value(result, value);
    value = value->next;
  }
}

void wpcp_cbor_write_buffer_write_wpcp_value(struct wpcp_cbor_write_buffer_t* buffer, const struct wpcp_value_t* value)
{
  switch (value->type) {
  case WPCP_VALUE_TYPE_UINT64:
    wpcp_cbor_write_buffer_write_type_and_uint64_value(buffer, WPCP_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value->value.uint);
    break;

  case WPCP_VALUE_TYPE_INT64:
    if (value->value.sint < 0)
      wpcp_cbor_write_buffer_write_type_and_uint64_value(buffer, WPCP_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER, ~value->value.uint);
    else
      wpcp_cbor_write_buffer_write_type_and_uint64_value(buffer, WPCP_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value->value.uint);
    break;

  case WPCP_VALUE_TYPE_BYTE_STRING:
    wpcp_cbor_write_buffer_write_type_and_size_value(buffer, WPCP_CBOR_MAJOR_TYPE_BYTE_STRING, value->value.length);
    wpcp_cbor_write_buffer_write_raw(buffer, value->data.byte_string, value->value.length);
    break;

  case WPCP_VALUE_TYPE_TEXT_STRING:
    wpcp_cbor_write_buffer_write_type_and_size_value(buffer, WPCP_CBOR_MAJOR_TYPE_TEXT_STRING, value->value.length);
    wpcp_cbor_write_buffer_write_raw(buffer, value->data.text_string, value->value.length);
    break;

  case WPCP_VALUE_TYPE_FALSE:
    wpcp_cbor_write_buffer_write_simple_type(buffer, WPCP_CBOR_SIMPLE_VALUE_FALSE);
    break;

  case WPCP_VALUE_TYPE_TRUE:
    wpcp_cbor_write_buffer_write_simple_type(buffer, WPCP_CBOR_SIMPLE_VALUE_TRUE);
    break;

  case WPCP_VALUE_TYPE_NULL:
    wpcp_cbor_write_buffer_write_simple_type(buffer, WPCP_CBOR_SIMPLE_VALUE_NULL);
    break;

  case WPCP_VALUE_TYPE_UNDEFINED:
    wpcp_cbor_write_buffer_write_simple_type(buffer, WPCP_CBOR_SIMPLE_VALUE_UNDEFINED);
    break;

  case WPCP_VALUE_TYPE_SIMPLE_VALUE:
    wpcp_cbor_write_buffer_write_type_and_uint8_value(buffer, WPCP_CBOR_MAJOR_TYPE_FLOATING_POINT_AND_SIMPLE_TYPES, (uint8_t) value->value.uint);
    break;

  case WPCP_VALUE_TYPE_FLOAT:
    wpcp_cbor_write_buffer_write_float(buffer, value->value.flt);
    break;

  case WPCP_VALUE_TYPE_DOUBLE:
    wpcp_cbor_write_buffer_write_double(buffer, value->value.dbl);
    break;

  case WPCP_VALUE_TYPE_TAG:
    wpcp_cbor_write_buffer_write_type_and_uint64_value(buffer, WPCP_CBOR_MAJOR_TYPE_TAG, value->value.uint);
    wpcp_cbor_write_buffer_write_wpcp_value(buffer, value->data.first_child);
    break;

  case WPCP_VALUE_TYPE_ARRAY:
    wpcp_cbor_write_buffer_write_type_and_size_value(buffer, WPCP_CBOR_MAJOR_TYPE_ARRAY, value->value.length);
    wpcp_cbor_write_buffer_write_wpcp_values(buffer, value->value.length, value->data.first_child);
    break;

  case WPCP_VALUE_TYPE_MAP:
    wpcp_cbor_write_buffer_write_type_and_size_value(buffer, WPCP_CBOR_MAJOR_TYPE_MAP, value->value.length);
    wpcp_cbor_write_buffer_write_wpcp_values(buffer, value->value.length * 2, value->data.first_child);
    break;
  }
}

void wpcp_cbor_write_buffer_write_additional(struct wpcp_cbor_write_buffer_t* write_buffer, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  uint32_t i;

  for (i = 0; i < additional_count; ++i) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, additional[i].key, additional[i].key_length);
    wpcp_cbor_write_buffer_write_wpcp_value(write_buffer, additional[i].value);
  }
}

void wpcp_cbor_write_buffer_write_data_item(struct wpcp_cbor_write_buffer_t* write_buffer, const struct wpcp_value_t* value, double timestamp, uint32_t status, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  uint32_t count = (value ? 1 : 0) + (timestamp ? 1 : 0) + (status ? 1 : 0) + additional_count;

  wpcp_cbor_write_buffer_write_map_header(write_buffer, count);

  if (value) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "value", 5);
    wpcp_cbor_write_buffer_write_wpcp_value(write_buffer, value);
  }

  if (timestamp) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "timestamp", 9);
    wpcp_cbor_write_buffer_write_double(write_buffer, timestamp);
  }

  if (status) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "status", 6);
    wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, status);
  }

  wpcp_cbor_write_buffer_write_additional(write_buffer, additional, additional_count);
}

void wpcp_cbor_write_buffer_write_alarm_item(struct wpcp_cbor_write_buffer_t* write_buffer, const char* key, uint32_t key_length, bool retain, const struct wpcp_value_t* token, const struct wpcp_value_t* id, double timestamp, uint32_t priority, const char* message, uint32_t message_length, bool acknowledged, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  uint32_t count = 2 + (token ? 1 : 0) + (id ? 1 : 0) + (timestamp ? 1 : 0) + (priority ? 1 : 0) + (message ? 1 : 0) + 1 + additional_count;

  wpcp_cbor_write_buffer_write_map_header(write_buffer, count);

  wpcp_cbor_write_buffer_write_text_string(write_buffer, "key", 3);
  wpcp_cbor_write_buffer_write_text_string(write_buffer, key, key_length);

  wpcp_cbor_write_buffer_write_text_string(write_buffer, "retain", 6);
  wpcp_cbor_write_buffer_write_boolean(write_buffer, retain);

  if (token) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "token", 5);
    wpcp_cbor_write_buffer_write_wpcp_value(write_buffer, token);
  }

  if (id) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "id", 2);
    wpcp_cbor_write_buffer_write_wpcp_value(write_buffer, id);
  }

  if (timestamp) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "timestamp", 9);
    wpcp_cbor_write_buffer_write_double(write_buffer, timestamp);
  }

  if (priority) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "priority", 8);
    wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, priority);
  }

  if (message) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "message", 7);
    wpcp_cbor_write_buffer_write_text_string(write_buffer, message, message_length);
  }

  wpcp_cbor_write_buffer_write_text_string(write_buffer, "acknowledged", 12);
  wpcp_cbor_write_buffer_write_boolean(write_buffer, acknowledged);

  wpcp_cbor_write_buffer_write_additional(write_buffer, additional, additional_count);
}

WPCP_END_EXTERN_C

#include "lesson.h"

#include <limits.h>
#include <string.h>

static int tcpip_l11_bytes_equal(const uint8_t left[4], const uint8_t right[4]) {
  return memcmp(left, right, 4u) == 0;
}

static int tcpip_l11_route_is_canonical(const tcpip_l11_route *route) {
  if (route->prefix_length > 32u) {
    return 0;
  }
  const unsigned whole_bytes = (unsigned)route->prefix_length / 8u;
  const unsigned remaining_bits = (unsigned)route->prefix_length % 8u;
  unsigned index = whole_bytes;
  if (remaining_bits != 0u) {
    const uint8_t host_mask = (uint8_t)((1u << (8u - remaining_bits)) - 1u);
    if ((route->network[index] & host_mask) != 0u) {
      return 0;
    }
    index += 1u;
  }
  while (index < 4u) {
    if (route->network[index] != 0u) {
      return 0;
    }
    index += 1u;
  }
  return 1;
}

static int tcpip_l11_prefix_matches(
    const uint8_t network[4], const uint8_t destination[4], uint8_t prefix_length) {
  const unsigned whole_bytes = (unsigned)prefix_length / 8u;
  const unsigned remaining_bits = (unsigned)prefix_length % 8u;
  if (whole_bytes != 0u && memcmp(network, destination, whole_bytes) != 0) {
    return 0;
  }
  if (remaining_bits == 0u) {
    return 1;
  }
  const uint8_t mask = (uint8_t)(0xffu << (8u - remaining_bits));
  return (network[whole_bytes] & mask) == (destination[whole_bytes] & mask);
}

tcpip_l11_status tcpip_l11_longest_prefix(
    const tcpip_l11_route *routes,
    size_t count,
    const uint8_t destination_ip[4],
    size_t *index) {
  if (index == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  *index = SIZE_MAX;
  if ((routes == NULL && count != 0u) || destination_ip == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }

  for (size_t route_index = 0u; route_index < count; route_index += 1u) {
    if (!tcpip_l11_route_is_canonical(&routes[route_index])) {
      return TCPIP_L11_MALFORMED;
    }
  }

  size_t best = SIZE_MAX;
  for (size_t route_index = 0u; route_index < count; route_index += 1u) {
    const tcpip_l11_route *candidate = &routes[route_index];
    if (!tcpip_l11_prefix_matches(
            candidate->network, destination_ip, candidate->prefix_length)) {
      continue;
    }
    if (best == SIZE_MAX || candidate->prefix_length > routes[best].prefix_length ||
        (candidate->prefix_length == routes[best].prefix_length &&
         candidate->metric < routes[best].metric)) {
      best = route_index;
    }
  }
  if (best == SIZE_MAX) {
    return TCPIP_L11_TRUNCATED;
  }
  *index = best;
  return TCPIP_L11_OK;
}

static int tcpip_l11_nat_is_valid(const tcpip_l11_nat *nat) {
  return nat != NULL && nat->first_port != 0u && nat->storage != NULL &&
         nat->capacity != 0u;
}

static int tcpip_l11_protocol_is_supported(uint8_t protocol) {
  return protocol == 6u || protocol == 17u;
}

static int tcpip_l11_tuple_is_valid(const tcpip_l11_tuple *tuple) {
  return tuple != NULL && tcpip_l11_protocol_is_supported(tuple->protocol) &&
         tuple->source_port != 0u && tuple->destination_port != 0u;
}

static int tcpip_l11_mapping_is_valid(
    const tcpip_l11_nat *nat, const tcpip_l11_nat_mapping *mapping) {
  if (mapping->active == 0u) {
    return 1;
  }
  return mapping->active == 1u && tcpip_l11_protocol_is_supported(mapping->protocol) &&
         mapping->private_port != 0u && mapping->remote_port != 0u &&
         mapping->public_port >= nat->first_port;
}

static tcpip_l11_status tcpip_l11_validate_mappings(const tcpip_l11_nat *nat) {
  for (size_t index = 0u; index < nat->capacity; index += 1u) {
    if (!tcpip_l11_mapping_is_valid(nat, &nat->storage[index])) {
      return TCPIP_L11_MALFORMED;
    }
    if (nat->storage[index].active == 0u) {
      continue;
    }
    for (size_t previous = 0u; previous < index; previous += 1u) {
      if (nat->storage[previous].active != 0u &&
          nat->storage[previous].public_port == nat->storage[index].public_port) {
        return TCPIP_L11_MALFORMED;
      }
    }
  }
  return TCPIP_L11_OK;
}

tcpip_l11_status tcpip_l11_nat_init(
    tcpip_l11_nat *nat,
    uint16_t first_port,
    const uint8_t public_ip[4],
    tcpip_l11_nat_mapping *storage,
    size_t capacity) {
  if (nat == NULL || first_port == 0u || public_ip == NULL || storage == NULL ||
      capacity == 0u) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  if (capacity > SIZE_MAX / sizeof(*storage)) {
    return TCPIP_L11_CAPACITY;
  }

  tcpip_l11_nat initialized;
  memcpy(initialized.public_ip, public_ip, sizeof(initialized.public_ip));
  initialized.first_port = first_port;
  initialized.storage = storage;
  initialized.capacity = capacity;
  memset(storage, 0, capacity * sizeof(*storage));
  *nat = initialized;
  return TCPIP_L11_OK;
}

static int tcpip_l11_mapping_matches_outbound(
    const tcpip_l11_nat_mapping *mapping, const tcpip_l11_tuple *tuple) {
  return mapping->active != 0u && mapping->protocol == tuple->protocol &&
         mapping->private_port == tuple->source_port &&
         mapping->remote_port == tuple->destination_port &&
         tcpip_l11_bytes_equal(mapping->private_ip, tuple->source_ip) &&
         tcpip_l11_bytes_equal(mapping->remote_ip, tuple->destination_ip);
}

static int tcpip_l11_port_in_use(const tcpip_l11_nat *nat, uint16_t port) {
  for (size_t index = 0u; index < nat->capacity; index += 1u) {
    if (nat->storage[index].active != 0u && nat->storage[index].public_port == port) {
      return 1;
    }
  }
  return 0;
}

static int tcpip_l11_find_available_port(const tcpip_l11_nat *nat, uint16_t *port) {
  for (uint32_t candidate = nat->first_port; candidate <= UINT16_MAX; candidate += 1u) {
    const uint16_t candidate_port = (uint16_t)candidate;
    if (!tcpip_l11_port_in_use(nat, candidate_port)) {
      *port = candidate_port;
      return 1;
    }
  }
  return 0;
}

tcpip_l11_status tcpip_l11_nat_translate_outbound(
    tcpip_l11_nat *nat,
    uint64_t now,
    const tcpip_l11_tuple *tuple,
    tcpip_l11_tuple *out) {
  if (out == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  tcpip_l11_tuple input;
  if (tuple != NULL) {
    input = *tuple;
  }
  memset(out, 0, sizeof(*out));
  if (!tcpip_l11_nat_is_valid(nat) || tuple == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  if (!tcpip_l11_tuple_is_valid(&input)) {
    return TCPIP_L11_MALFORMED;
  }
  tcpip_l11_status status = tcpip_l11_validate_mappings(nat);
  if (status != TCPIP_L11_OK) {
    return status;
  }

  size_t mapping_index = SIZE_MAX;
  size_t free_index = SIZE_MAX;
  for (size_t index = 0u; index < nat->capacity; index += 1u) {
    if (tcpip_l11_mapping_matches_outbound(&nat->storage[index], &input)) {
      mapping_index = index;
      break;
    }
    if (free_index == SIZE_MAX && nat->storage[index].active == 0u) {
      free_index = index;
    }
  }

  uint16_t public_port = 0u;
  tcpip_l11_nat_mapping new_mapping;
  memset(&new_mapping, 0, sizeof(new_mapping));
  if (mapping_index != SIZE_MAX) {
    public_port = nat->storage[mapping_index].public_port;
  } else {
    if (free_index == SIZE_MAX || !tcpip_l11_find_available_port(nat, &public_port)) {
      return TCPIP_L11_CAPACITY;
    }
    new_mapping.active = 1u;
    new_mapping.protocol = input.protocol;
    memcpy(new_mapping.private_ip, input.source_ip, sizeof(new_mapping.private_ip));
    memcpy(new_mapping.remote_ip, input.destination_ip, sizeof(new_mapping.remote_ip));
    new_mapping.private_port = input.source_port;
    new_mapping.remote_port = input.destination_port;
    new_mapping.public_port = public_port;
    new_mapping.last_used = now;
  }

  tcpip_l11_tuple translated = input;
  memcpy(translated.source_ip, nat->public_ip, sizeof(translated.source_ip));
  translated.source_port = public_port;

  if (mapping_index != SIZE_MAX) {
    nat->storage[mapping_index].last_used = now;
  } else {
    nat->storage[free_index] = new_mapping;
  }
  *out = translated;
  return TCPIP_L11_OK;
}

static int tcpip_l11_mapping_matches_inbound(
    const tcpip_l11_nat_mapping *mapping, const tcpip_l11_tuple *tuple) {
  return mapping->active != 0u && mapping->protocol == tuple->protocol &&
         mapping->public_port == tuple->destination_port &&
         mapping->remote_port == tuple->source_port &&
         tcpip_l11_bytes_equal(mapping->remote_ip, tuple->source_ip);
}

tcpip_l11_status tcpip_l11_nat_translate_inbound(
    tcpip_l11_nat *nat,
    uint64_t now,
    const tcpip_l11_tuple *tuple,
    tcpip_l11_tuple *out) {
  if (out == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  tcpip_l11_tuple input;
  if (tuple != NULL) {
    input = *tuple;
  }
  memset(out, 0, sizeof(*out));
  if (!tcpip_l11_nat_is_valid(nat) || tuple == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  if (!tcpip_l11_tuple_is_valid(&input)) {
    return TCPIP_L11_MALFORMED;
  }
  if (!tcpip_l11_bytes_equal(input.destination_ip, nat->public_ip)) {
    return TCPIP_L11_TRUNCATED;
  }
  tcpip_l11_status status = tcpip_l11_validate_mappings(nat);
  if (status != TCPIP_L11_OK) {
    return status;
  }

  size_t mapping_index = SIZE_MAX;
  for (size_t index = 0u; index < nat->capacity; index += 1u) {
    if (tcpip_l11_mapping_matches_inbound(&nat->storage[index], &input)) {
      mapping_index = index;
      break;
    }
  }
  if (mapping_index == SIZE_MAX) {
    return TCPIP_L11_TRUNCATED;
  }

  tcpip_l11_tuple translated = input;
  memcpy(
      translated.destination_ip,
      nat->storage[mapping_index].private_ip,
      sizeof(translated.destination_ip));
  translated.destination_port = nat->storage[mapping_index].private_port;
  nat->storage[mapping_index].last_used = now;
  *out = translated;
  return TCPIP_L11_OK;
}

tcpip_l11_status tcpip_l11_nat_expire(
    tcpip_l11_nat *nat, uint64_t now, uint64_t idle, size_t *count) {
  if (count == NULL) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  *count = 0u;
  if (!tcpip_l11_nat_is_valid(nat)) {
    return TCPIP_L11_INVALID_ARGUMENT;
  }
  const tcpip_l11_status status = tcpip_l11_validate_mappings(nat);
  if (status != TCPIP_L11_OK) {
    return status;
  }

  size_t expired = 0u;
  for (size_t index = 0u; index < nat->capacity; index += 1u) {
    tcpip_l11_nat_mapping *mapping = &nat->storage[index];
    if (mapping->active != 0u && now >= mapping->last_used &&
        now - mapping->last_used >= idle) {
      memset(mapping, 0, sizeof(*mapping));
      expired += 1u;
    }
  }
  *count = expired;
  return TCPIP_L11_OK;
}

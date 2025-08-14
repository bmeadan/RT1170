#ifndef __NETWORK_ENTITY_H__
#define __NETWORK_ENTITY_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void set_slave_network_entity_id(uint8_t id);
void set_network_entity_id(uint8_t id);

uint8_t get_network_entity_id();
uint8_t get_max_network_entity_id();

#ifdef __cplusplus
}
#endif

#endif /* __NETWORK_ENTITY_H__ */

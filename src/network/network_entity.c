#include "network_entity.h"
#include "fsl_common.h"

static volatile uint8_t network_entity_id = 0;
static volatile uint8_t max_network_entity_id = 0;

void set_slave_network_entity_id(uint8_t id) {
    max_network_entity_id = MAX(max_network_entity_id, id);
}

void set_network_entity_id(uint8_t id) {
    network_entity_id = id;
    max_network_entity_id = MAX(max_network_entity_id, id);
}

uint8_t get_network_entity_id() {
    return network_entity_id;
}

uint8_t get_max_network_entity_id() {
    return max_network_entity_id;
}
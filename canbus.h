#ifndef CANBUS_H
#define CANBUS_H

#include <cstdint>

void can_begin();
bool can_enable(bool enabled);
void can_send_message(uint16_t id, const uint8_t* data, uint8_t dataLength);
bool can_get_id(const char* canid, uint16_t* cid, uint8_t* sid);

#endif
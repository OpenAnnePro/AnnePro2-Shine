#ifndef COMMANDS_INCLUDED
#define COMMANDS_INCLUDED

#include "protocol.h"

/* Called as a protocol callback to handle incoming messages */
extern void commandCallback(const message_t *msg);

/* Send arbitrary data to the main chip, which can transmit them over USB to PC
 * for debugging. */
extern void sendDebug(const char *payload, uint8_t size);

#endif

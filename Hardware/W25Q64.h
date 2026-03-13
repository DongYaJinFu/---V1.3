#ifndef __W25Q64_H
#define __W25Q64_H

void W25Q64_GPIO_Init(void);
void W25Q64_ReadID(uint8_t *MID, uint16_t *DID);

#endif

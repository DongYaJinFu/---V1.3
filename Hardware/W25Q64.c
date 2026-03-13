#include "stm32f10x.h"
#include "W25Q64_Ins.h"

void W25Q64_W_CS(uint8_t BitValue)
{
	/*根据BitValue的值，将CE置高电平或者低电平*/
	GPIO_WriteBit(GPIOF, GPIO_Pin_14, (BitAction)BitValue);
}

uint8_t W25Q64_R_MISO(void)
{
	/*取MISO引脚的高低电平并返回*/
	return GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_15);
}

void W25Q64_W_SCK(uint8_t BitValue)
{
	/*根据BitValue的值，将SCK置高电平或者低电平*/
	GPIO_WriteBit(GPIOF, GPIO_Pin_12, (BitAction)BitValue);
}

void W25Q64_W_MOSI(uint8_t BitValue)
{
	/*根据BitValue的值，将MOSI置高电平或者低电平*/
	GPIO_WriteBit(GPIOF, GPIO_Pin_13, (BitAction)BitValue);
}

void W25Q64_GPIO_Init(void)
{
	/*开启GPIO时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	
	/*将CE、CSN、SCK、MOSI引脚初始化为推挽输出模式*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	/*将MISO引脚初始化为上拉输入模式*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	/*置引脚初始化后的默认电平*/
	W25Q64_W_CS(1);		    //CS默认为1，不选中从机
	W25Q64_W_SCK(0);		//SCK默认为0，对应SPI模式0
	W25Q64_W_MOSI(0);		//MOSI默认电平随意，1和0均可
}

uint8_t W25Q64_SPI_SwapByte(uint8_t Byte)
{
    uint8_t i, ByteReceive = 0x00;

    for(i = 0; i < 8; i++)
    {
        W25Q64_W_MOSI(Byte & (0x80 >> i));
        W25Q64_W_SCK(1);
        if(W25Q64_R_MISO() == 1)
        {
            ByteReceive |= (0x80 >> i);
        }
        W25Q64_W_SCK(0);
    }
    return ByteReceive;
}

void W25Q64_ReadID(uint8_t *MID, uint16_t *DID)
{
	W25Q64_W_CS(0);				       			    //SPI起始
	W25Q64_SPI_SwapByte(W25Q64_JEDEC_ID);	        //交换发送读取ID的指令
	*MID = W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE);	//交换接收MID，通过输出参数返回
	*DID = W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE);	//交换接收DID高8位
	*DID <<= 8;									    //高8位移到高位
	*DID |= W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE);	//或上交换接收DID的低8位，通过输出参数返回
	W25Q64_W_CS(1); 							    //SPI终止
}


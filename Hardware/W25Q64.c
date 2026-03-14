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

void W25Q64_WriteEnable(void)
{
	W25Q64_W_CS(0);								    //SPI起始
	W25Q64_SPI_SwapByte(W25Q64_WRITE_ENABLE);		//交换发送写使能的指令
	W25Q64_W_CS(1);									//SPI终止
}

void W25Q64_WaitBusy(void)
{
	uint32_t Timeout;
	W25Q64_W_CS(0);													//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);				//交换发送读状态寄存器1的指令
	Timeout = 100000;												//给定超时计数时间
	while ((W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE) & 0x01) == 0x01)	//循环等待忙标志位
	{
		Timeout --;													//等待时，计数值自减
		if (Timeout == 0)											//自减到0后，等待超时
		{
			/*超时的错误处理代码，可以添加到此处*/
			break;													//跳出等待，不等了
		}
	}
	W25Q64_W_CS(1);													//SPI终止
}

void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
{
	uint16_t i;

	W25Q64_WriteEnable();							//写使能

	W25Q64_W_CS(0);									//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_PAGE_PROGRAM);		//交换发送页编程的指令
	W25Q64_SPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	W25Q64_SPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	W25Q64_SPI_SwapByte(Address);					//交换发送地址7~0位
	for (i = 0; i < Count; i ++)					//循环Count次
	{	
		W25Q64_SPI_SwapByte(DataArray[i]);			//依次在起始地址后写入数据
	}	
	W25Q64_W_CS(1);									//SPI终止

	W25Q64_WaitBusy();								//等待忙
}

void W25Q64_SectorErase(uint32_t Address)
{
	W25Q64_WriteEnable();							//写使能
	
	W25Q64_W_CS(0);									//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);	//交换发送扇区擦除的指令
	W25Q64_SPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	W25Q64_SPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	W25Q64_SPI_SwapByte(Address);					//交换发送地址7~0位
	W25Q64_W_CS(1);									//SPI终止

	W25Q64_WaitBusy();								//等待忙
}

void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
	uint32_t i;
	W25Q64_W_CS(0);												//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_READ_DATA);						//交换发送读取数据的指令
	W25Q64_SPI_SwapByte(Address >> 16);							//交换发送地址23~16位
	W25Q64_SPI_SwapByte(Address >> 8);							//交换发送地址15~8位
	W25Q64_SPI_SwapByte(Address);								//交换发送地址7~0位
	for (i = 0; i < Count; i ++)								//循环Count次
	{
		DataArray[i] = W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE);	//依次在起始地址后读取数据
	}
	W25Q64_W_CS(1);												//SPI终止
}


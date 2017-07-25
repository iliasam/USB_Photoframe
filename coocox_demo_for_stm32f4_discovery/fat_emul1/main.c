#include "usbd_msc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usb_conf.h"
#include "emfat.h"
#include "stm32f4_discovery.h"
#include "bmp_header.h"
#include "main.h"
#include "string_render.h"
#include "image_render.h"


const uint32_t bmp_size = BMP_FULL_SIZE;
const uint32_t bmp_width = BMP_WIDTH;
const uint32_t bmp_height = BMP_HEIGHT;

USB_OTG_CORE_HANDLE USB_OTG_dev;

// экземпляр виртуальной файловой системы FAT16
emfat_t emfat;

void bmp_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata);
void empty_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata);

// Структура ФС
// Filesystem structure
static emfat_entry_t entries[] =
{
	// name          dir    lvl offset  size             max_size        user  read               write
	{ "",            true,  0,  0,      0,               0,            0,     NULL,             NULL }, // root
	{ "image.bmp",  false, 1,  0,      BMP_FULL_SIZE,        BMP_FULL_SIZE,      0,     bmp_read_proc,    NULL },
	{ "empty.txt",  false, 1,  0,      EMPTY_SIZE,      EMPTY_SIZE,    0,     empty_read_proc,  NULL },//do not delete, it must be last in this list
	{ NULL }
};



int main(void)
{
	RCC_ClocksTypeDef RCC_Clocks;
	STM_EVAL_LEDInit(LED4);
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED5);
	STM_EVAL_LEDInit(LED6);

	RCC_GetClocksFreq(&RCC_Clocks);

	init_string_render();
	update_picture_sizes();

	STM_EVAL_LEDOn(LED4);

	emfat_init(&emfat, "emfat", entries);
	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_MSC_cb, &USR_cb);
	STM_EVAL_LEDOn(LED5);

    while(1)
    {
    }
}


//*********************************************************************************
// callback функции для формирования файлов
// callback function to generate file data

//generate BMP image
//сформировать BMP изображение
void bmp_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
	int len = 0;

	uint32_t image_sector;
	if (offset > BMP_FULL_SIZE) return;
	if (offset + size > BMP_FULL_SIZE)
		len = BMP_FULL_SIZE - offset;
	else
		len = size;

	image_sector = (offset/(uint32_t)SECTOR_SIZE )- (uint32_t)2;

	if (offset == 0)//header sector number 1
	{
		memcpy(dest, &bmp_header, 512);

		memcpy((dest+2), &bmp_size, 4);
		memcpy((dest+18), &bmp_width, 4);
		memcpy((dest+22), &bmp_height, 4);
		image_sector = 0;
	}
	else if (offset == 512){memcpy(dest, &bmp_header[512], 512);image_sector = 0;update_text();}//header sector number 2
	else if (offset == 1024)//header sector number 3 + data
	{
		memcpy(dest, &bmp_header[1024], BMP_OFFSET);
		string_prosess_sector(dest+BMP_OFFSET,image_sector,len);
		picture_prosess_sector(dest+BMP_OFFSET,image_sector,len);
	}
	else
	{
		string_prosess_sector(dest,image_sector,len);
		picture_prosess_sector(dest,image_sector,len);
	}
}

//generate dummy txt file (only for size)
//сформировать текстовый файл (только для радания размера диска)
void empty_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
	int len = 0;
	if (offset > EMPTY_SIZE) return;
	if (offset + size > EMPTY_SIZE)
		len = EMPTY_SIZE - offset;
	else
		len = size;
	memset(dest, 65, len);//65 - "A"
}

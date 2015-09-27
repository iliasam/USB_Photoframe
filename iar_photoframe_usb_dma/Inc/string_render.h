#ifndef _SRTING_RENDER
#define _STRING_RENDER

#include "main.h"
#include <mcufont.h>

#define BUFFER_HEIGHT (uint16_t)48


typedef struct {
	char *text;
	uint16_t x;
	uint16_t y;
	uint8_t visible;
} test_struct;

typedef struct {
    const char *fontname;
    const char *filename;
    const char *text;
    bool justify;
    enum mf_align_t alignment;
    int width;
    int margin;
    int anchor;
    int scale;
} options_t;

typedef struct {
    options_t *options;
    uint8_t *buffer;
    uint16_t width;
    uint16_t height;
    uint16_t y;
    const struct mf_font_s *font;
} state_t;

void clean_buffer(void);
void draw_strings(uint16_t cur_line);

void pixel_callback(int16_t x, int16_t y, uint8_t count, uint8_t alpha,void *state);
uint8_t character_callback(int16_t x, int16_t y, mf_char character, void *state);
bool line_callback(const char *line, uint16_t count, void *state);

void init_string_render(void);
void string_prosess_sector(uint8_t *dest,uint32_t sector, int len);

void update_text(void);
void update_err_message(wifi_status_type status);

void draw_temperatute(uint16_t x,uint16_t y, int8_t temperature, test_struct *text);

#endif

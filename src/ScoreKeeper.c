/*
 * Pebble Stopwatch - the big, ugly file.
 * Copyright (C) 2013 Katharine Berry
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xC2, 0x5A, 0x8D, 0x50, 0x10, 0x5F, 0x45, 0xBF, 0xC9, 0x92, 0xCE, 0xF9, 0x58, 0xAC, 0x93, 0xAD }
PBL_APP_INFO(MY_UUID,
             "Score Keeper", "zandegran",
             1, 21, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_STANDARD_APP);

static Window window;
AppContextRef app;

// Main display
static TextLayer ScoreA_layer;
static TextLayer ScoreB_layer;
static BmpContainer button_labels;
static BmpContainer Background;

// Lap time display
#define LAP_TIME_SIZE 5


// The documentation claims this is defined, but it is not.
// Define it here for now.
#ifndef APP_TIMER_INVALID_HANDLE
    #define APP_TIMER_INVALID_HANDLE 0xDEADBEEF
#endif
#define TIMER_UPDATE 1

// Actually keeping track of time
static int clicksa=0;
static int clicksb=0;
static bool vibe=true;
static AppTimerHandle  update_timer = APP_TIMER_INVALID_HANDLE;
// We want hundredths of a second, but Pebble won't give us that.
// Pebble's timers are also too inaccurate (we run fast for some reason)
// Instead, we count our own time but also adjust ourselves every pebble
// clock tick. We maintain our original offset in hundredths of a second
// from the first tick. This should ensure that we always have accurate times.

// Global animation lock. As long as we only try doing things while
// this is zero, we shouldn't crash the watch.

#define TIMER_UPDATE 1
#define FONT_BIG_TIME RESOURCE_ID_FONT_DEJAVU_SANS_BOLD_SUBSET_36

#define BUTTON_DOWN BUTTON_ID_DOWN
#define BUTTON_RESET BUTTON_ID_SELECT
#define BUTTON_UP BUTTON_ID_UP

void config_provider(ClickConfig **config, Window *window);
void handle_init(AppContextRef ctx);
void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie);
void pbl_main(void *params);
void update_vibe_icon()
{
    layer_remove_from_parent(&button_labels.layer.layer);
    bmp_deinit_container(&button_labels);
    if (vibe)
    {
        bmp_init_container(RESOURCE_ID_IMAGE_BUTTON_LABELS, &button_labels);
    }
    else
    {
        bmp_init_container(RESOURCE_ID_IMAGE_BUTTON_LABELS_NO_VIBE, &button_labels);
    }
    layer_set_frame(&button_labels.layer.layer, GRect(130, 0, 14, 140));
    layer_add_child(&window.layer, &button_labels.layer.layer);
}
void handle_init(AppContextRef ctx) {
    app = ctx;

    window_init(&window, "ScoreKeeper");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);
    window_set_fullscreen(&window, false);

    resource_init_current_app(&APP_RESOURCES);

    // Arrange for user input.
    window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);

    // Get our fonts
    GFont big_font = fonts_load_custom_font(resource_get_handle(FONT_BIG_TIME));

    // Root layer
    Layer *root_layer = window_get_root_layer(&window);
    // Add BackGround
    bmp_init_container(RESOURCE_ID_IMAGE_BG, &Background);
    layer_set_frame(&Background.layer.layer, GRect(0, 0, 144, 140));
    layer_add_child(root_layer, &Background.layer.layer);
    // Set up the Score.
    text_layer_init(&ScoreA_layer, GRect(20, 18, 90, 40));
    text_layer_set_background_color(&ScoreA_layer, GColorBlack);
    text_layer_set_font(&ScoreA_layer, big_font);
    text_layer_set_text_color(&ScoreA_layer, GColorWhite);
    text_layer_set_text(&ScoreA_layer, "  0");
    text_layer_set_text_alignment(&ScoreA_layer, GTextAlignmentCenter);
    layer_add_child(root_layer, &ScoreA_layer.layer);
    text_layer_init(&ScoreB_layer, GRect(20, 81, 90, 40));
    text_layer_set_background_color(&ScoreB_layer, GColorBlack);
    text_layer_set_font(&ScoreB_layer, big_font);
    text_layer_set_text_color(&ScoreB_layer, GColorWhite);
    text_layer_set_text(&ScoreB_layer, "  0");
    text_layer_set_text_alignment(&ScoreB_layer, GTextAlignmentCenter);
    layer_add_child(root_layer, &ScoreB_layer.layer);

 

    // Add some button labels
    update_vibe_icon();

    
}

void handle_deinit(AppContextRef ctx) {
    bmp_deinit_container(&button_labels);
}

void vibe_handler(ClickRecognizerRef recognizer, Window *window) {
    if(vibe)
    {
        vibes_short_pulse();
        vibe=false;
        
    }
    else
    {
        vibe=true;
    }
    update_vibe_icon();
}
void reset_handler(ClickRecognizerRef recognizer, Window *window) {
    clicksa=0;
    clicksb=0;
    if(vibe)
        vibes_long_pulse();
    update_timer = app_timer_send_event(app, 100, TIMER_UPDATE);
}

void Aincrement_handler(ClickRecognizerRef recognizer, Window *window) {
    
    clicksa+=1;
    if(vibe)
        vibes_short_pulse();
    update_timer = app_timer_send_event(app, 100, TIMER_UPDATE);
}

void Adecrement_handler(ClickRecognizerRef recognizer, Window *window) {
    if(clicksa>0)
    {
        clicksa-=1;
        if(vibe)
            vibes_short_pulse();
        update_timer = app_timer_send_event(app, 100, TIMER_UPDATE);
    }
    
}
void Bincrement_handler(ClickRecognizerRef recognizer, Window *window) {
    
    clicksb+=1;
    if(vibe)
        vibes_short_pulse();
    update_timer = app_timer_send_event(app, 100, TIMER_UPDATE);
}

void Bdecrement_handler(ClickRecognizerRef recognizer, Window *window) {
    if(clicksb>0)
    {
        clicksb-=1;
        if(vibe)
            vibes_short_pulse();
        update_timer = app_timer_send_event(app, 100, TIMER_UPDATE);
    }
}
void itoa2(int num, char* buffer) {
    const char digits[10] = "0123456789";
    if(num > 999) {
        buffer[0] = '9';
        buffer[1] = '9';
        buffer[2] = '9';

        return;
    } else if(num > 99) {
        
        buffer[0] = digits[num / 100];
        if(num%100 > 9) {
            buffer[1] = digits[(num%100) / 10];
        } else {
            buffer[1] = '0';
        }
        buffer[2] = digits[num%10];
    }
    
    else if(num > 9) {
        buffer[0]=' ';
        buffer[1] = digits[num / 10];
        
        buffer[2] = digits[num % 10];
    } else {
        buffer[0] = ' ';

        buffer[1] = ' ';
        
        buffer[2] = digits[num % 10];
    }
}
void update_Scores() {
    static char ScoreA[] = " 0 ";
    static char ScoreB[] = " 0 ";
    itoa2(clicksa, &ScoreA[0]);
    itoa2(clicksb, &ScoreB[0]);
    // Now draw the strings.
    text_layer_set_text(&ScoreA_layer,ScoreA);
    text_layer_set_text(&ScoreB_layer,ScoreB);
}
void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
    (void)handle;
            update_Scores();
    
}






void config_provider(ClickConfig **config, Window *window) {
    //config[BUTTON_RESET]->click.handler = (ClickHandler)reset_handler;
    config[BUTTON_UP]->click.handler = (ClickHandler)Aincrement_handler;
    config[BUTTON_UP]->long_click.handler = (ClickHandler)Adecrement_handler;
    config[BUTTON_UP]->long_click.delay_ms = 700;
    config[BUTTON_DOWN]->click.handler = (ClickHandler)Bincrement_handler;
    config[BUTTON_DOWN]->long_click.handler = (ClickHandler)Bdecrement_handler;
    config[BUTTON_DOWN]->long_click.delay_ms = 700;
    config[BUTTON_RESET]->click.handler = (ClickHandler)vibe_handler;
    config[BUTTON_RESET]->long_click.handler = (ClickHandler)reset_handler;
    config[BUTTON_RESET]->long_click.delay_ms = 1000;
    (void)window;
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .timer_handler = &handle_timer
  };
  app_event_loop(params, &handlers);
}

#include "Sleep.h"
#include "../../display_EPD.h"
#include "editor/editor.h"
#include "gallant.h"

//
#include "app/app.h"
#include "display/display.h"
#include "keyboard/keyboard.h"

#define WAKEUP_GPIO GPIO_NUM_27
#define WAKE_GPIO 2 
bool sleep_rendered = false;


void Sleep_setup()
{
  
    app_log("Sleep Screen Setup...\n"); 
    sleep_rendered = false;
}

//
void Sleep_render()
{
    //
    if (sleep_rendered)
        return;

    Rect_t area = {
        .x = 0,
        .y = 0,
        .width = gallant_width,
        .height = gallant_height,
    };
    epd_poweron();
    epd_clear();

    // Display the Mavis Gallant screensaver.
    epd_draw_grayscale_image(area, (uint8_t *)gallant_data);
    epd_poweroff();

    // SAVE FILE
    Editor::getInstance().saveFile();
  
    sleep_rendered = true;
    
     // Go to Sleep mode
    epd_poweroff_all(); */
    esp_deep_sleep_start();
   
}

//
void Sleep_keyboard(char key)
{
    // Any Keystroke moves to word processor
    app_log("Wake up!");
     // Any Keystroke moves to word processor
    JsonDocument &app = app_status();
    app["screen"] = WORDPROCESSOR;
     
}

#include "Sleep.h"
#include "../../display_EPD.h"
#include "editor/editor.h"

//
#include "app/app.h"
#include "display/display.h"
#include "keyboard/keyboard.h"
#define WAKEUP_GPIO GPIO_NUM_27
#define WAKE_GPIO 2 
bool sleep_rendered = false;
//
void Sleep_setup()
{
    //const uint64_t wakeup_mask = 1ULL << GPIO_NUM_27;
    //
    app_log("Sleep Screen Setup...\n");
   /* esp_err_t reply = esp_sleep_enable_ext1_wakeup(((uint64_t)(((uint64_t)1) << BUTTON_1)), ESP_EXT1_WAKEUP_ANY_LOW);
    if (reply ==ESP_OK)
    app_log("EPD Wakeup mode success.\n");
    else if (reply == ESP_ERR_INVALID_ARG)
    app_log("EPD Invalid Args. \n");
    else
    app_log("ESP Invalid state.\n");*/
    // render just once
    sleep_rendered = false;
}

//
void Sleep_render()
{
    //
    if (sleep_rendered)
        return;

    

    // SAVE FILE
    Editor::getInstance().saveFile();

    // Turn on the display
    epd_poweron();

    // Clear Screen
    epd_clear();

    // Render Image
    int32_t x = 20;
    int32_t y = 50;
    write_string(display_EPD_font(), "Please, turn off.", &x, &y, NULL);
     //
    sleep_rendered = true;
    
     // Go to Sleep mode
    epd_poweroff_all();
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

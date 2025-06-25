#include "WordProcessor.h"
#include "../../display_EPD.h"

//
#include "app/app.h"
#include "editor/editor.h"
#include "keyboard/keyboard.h"
#include "display/display.h"
#include "keyboard/hid/nimble/ble.h"
#include "service/Tools/Tools.h"

//
int last_sleep = millis();
//
#define MARGIN_X 20
#define MARGIN_Y 40

//
const int cols = 47;
const int rows = 7;

// status bar
const int status_height = 30;
const int statusY = EPD_HEIGHT - status_height - 5;

// font charecteristics - T.
const int fontNewLineSize = display_EPD_font()->advance_y;
const int fontWidth = display_EPD_font()->glyph->advance_x;

// clear screen
bool clear_full = false;

// bool clear_request = true;
bool cleared = true;
bool backspaced = false;

// flag: refresh word count only when it has chaged. -T.
int lastWordCount = -1; 

// label -T.
static String fileStatus = "NOT SAVED";
// flag: has the file status changed? -T.
bool fileStatusChanged = false;
//
int startLine = -1;

//
void WP_setup()
{
    app_log("Word Processor GUI Setup\n");

    // editor instantiate
    Editor &editor = Editor::getInstance();

    // start position has to be reset
    // before switching files through menu

    startLine = -1;

    editor.screenBuffer.rows = rows;
    editor.screenBuffer.cols = cols;

    // display init
    GFXfont *font = display_EPD_font();
    display_initialize(MARGIN_X, MARGIN_Y, 60, 20);

    // load file from the editor
    JsonDocument &app = app_status();
    int file_index = app["config"]["file_index"].as<int>();

    //
    Editor::getInstance().loadFile(format("/%d.txt", file_index));

    //
    app_log("Word Processor Initialized %d.txt\n", file_index);

    // clear background
    clear_full = true;
}

//
void WP_render()
{
    // perform full screen clear routine
    if (clear_full)
    {
        //
        epd_poweron();
        epd_clear();
        epd_poweroff_all();
        // Render the static and dynamic
        // parts of the status bar
        WP_render_static_status();
        WP_render_dynamic_status(true);

        clear_full = false;
        cleared = true;
    }

    //
    WP_check_saved();
    WP_check_sleep();

    // Bottom Status
    WP_render_status();
    // BLINK CURSOR
    WP_render_cursor();

    // RENDER TEXT
    WP_render_text();

    cleared = false;
}

// DRAW LINE OF TEXT
void WP_render_text_line(int i, int cursorY, uint8_t *framebuffer)
{
    char *line = Editor::getInstance().screenBuffer.line_position[i];
    int length = Editor::getInstance().screenBuffer.line_length[i];

    //
    display_setline(i - startLine);

    // render
    if (line != NULL && length > 0)
    {
        char row[256];

        // Copy the line content to row, but not more than 255 characters
        int copyLength = (length < 255) ? length : 255;
        strncpy(row, line, copyLength);

        // Null-terminate the string
        row[copyLength] = '\0';
        char utf8[512];
        convert_extended_ascii_to_utf8(row, utf8, 512);

        int cursorX = display_x();
        int cursorY = display_y();

        //
        if (framebuffer == NULL)
            epd_poweron();
        //
        writeln(display_EPD_font(), utf8, &cursorX, &cursorY, framebuffer);

        //
        if (framebuffer == NULL)
            epd_poweroff_all();

        //
        debug_log("WP_render_text_line::i %d x %d y %d [%s]\n", i, cursorX, cursorY, row);
    }
    else
    {
        debug_log("WP_render_text_line::line null or empty line position %d\n", i);
    }
}

void WP_clear_row(int row)
{
    epd_poweron();

    // delete a line and redraw the line
    Rect_t area =
        display_rect(
            0,
            display_lineheight() * row,
            EPD_WIDTH,
            display_lineheight());

    epd_clear_quick(area, 4, 50);

    epd_poweroff_all();
}
/**
 * @brief   Clear the lines  below the current cursor.
 * Author: T. 
 *
 * @param[in]  cursorLine  Current cursor line.
 * @return     None.
 */

void WP_clear_below_line(int cursorLine)
{
    int relativeLine = max(cursorLine - startLine, 0);
    // If it's not the last line, update the rest of the lines
    // of text since there positions may have changed.
    if (relativeLine < rows)
    {

        int startPositionNextLine = (15 * relativeLine) + (relativeLine * fontNewLineSize) + fontNewLineSize;
        int heightRemainingText = EPD_HEIGHT - startPositionNextLine - status_height;
        app_log("Font size: %d, cursorLine %d, startLine %d \n ", fontNewLineSize, cursorLine, startLine);
        // Clear the area below the current line
        Rect_t area = display_rect(
            0, startPositionNextLine,
            EPD_WIDTH,
            heightRemainingText);

        epd_poweron();
        epd_clear_quick(area, 4, 50);
        epd_poweroff_all();
    }
}

//
bool editing = false;

/**
 * @brief   Render status of the file (SAVED or NOT SAVED) on the status bar.
 * Author: T. 
 *
 * @param[in]  fileStatus  Current file status.
 * @return     None.
 */


void WP_render_fileSavedStatus(String fileStatus)
{
    
    int cursorX = 650;
    // status start Y position
    int cursorY = statusY + status_height - 8;
    // clear the status area
    Rect_t area = display_rect(
        cursorX,
        statusY,
        170,
        status_height);

    //
    epd_poweron();
    epd_clear_quick(area, 4, 50);

    // display the text
    writeln((GFXfont *)&systemFont, fileStatus.c_str(), &cursorX, &cursorY, NULL);
    epd_poweroff_all();
}


void WP_render_text()
{
    //
    JsonDocument &app = app_status();

    // Cursor Information
    static int cursorPos_prev = 0;
    static int cursorLine_prev = 0;
    static int cursorLinePos_prev = 0;
    static int bufferSize_prev = 0;
    int cursorPos = Editor::getInstance().fileBuffer.cursorPos;
    int cursorLine = Editor::getInstance().fileBuffer.cursorLine;
    int cursorLinePos = Editor::getInstance().fileBuffer.cursorLinePos;
    int bufferSize = Editor::getInstance().fileBuffer.getBufferSize();

    //
    int totalLine = Editor::getInstance().screenBuffer.total_line;
    int rows = Editor::getInstance().screenBuffer.rows;

    // when first turned on
    // start editing from last 2nd Line
    if (startLine == -1)
    {
        //
        startLine = max(cursorLine - rows + 1, 0);
        debug_log("WP_render_text::Start Line Init: %d cursorLine %d\n", startLine, cursorLine);
    }

    //
    // when reaching the end of the screen reset the startLine
    if (cursorLine - startLine > rows)
    {

        // Clear the text area; status bar remains intact - T.
        Rect_t area = display_rect(
            0,
            0, EPD_WIDTH,
            EPD_HEIGHT - status_height);

        epd_poweron();
        epd_clear_quick(area, 4, 50);
        epd_poweroff_all();

        //
        cleared = true;

        startLine = max(cursorLine - 1, 0);
        debug_log("WP_render_text::New Page cursorLine %d startLine %d rows %d totalLine %d\n",
                  cursorLine,
                  startLine,
                  rows,
                  totalLine);
    }

    // when cursor reaches the top and it's time to show the previous page
    else if (cursorLine < startLine)
    {
        //
        debug_log("WP_render_text::need to show previous page cursorLine %d prev %d startLine %d totalLine %d\n",
                  cursorLine,
                  cursorLine_prev,
                  startLine,
                  totalLine);

        // startLine should go one page before
        // keep the last line
        startLine = max(startLine - rows - 1, 0);

        debug_log("WP_render_text::startLine updated %d \n",
                  startLine);

        // // Clear the text area; status bar remains intact - T.
        Rect_t area = display_rect(
            0,
            0, EPD_WIDTH,
            EPD_HEIGHT - status_height);

        epd_poweron();
        epd_clear_quick(area, 4, 50);
        epd_poweroff_all();

        cleared = true;
    }

    //
    // Middle part of the text will be rendered
    // Only when refresh background is called
    //
    if (cleared)
    {
        // Draw from the first line
        display_setline(0);

        //
        for (int i = startLine; i <= totalLine; i++)
        {
            // stop when exceeding row
            if (i - startLine > rows)
            {
                debug_log("WP_render_text::reached the end of the text startLine %d i %d row %d\n", startLine, i, rows);
                break;
            }

            if (i >= 0)
                WP_render_text_line(i, display_y(), display_EPD_framebuffer());

            // increase the line
            display_newline();
        }

        //
        // render frame to the display
        display_draw_buffer();
    }

    // handle backspace
    else if (backspaced)
    {
        //
        backspaced = false;
        int currentLine = max(0, cursorLine - startLine);

        // clear the currentLine and the previousLine
        debug_log("WP_render_text::Handle Backspace\n");

        // if line is change then also clear the previous line

        if (Editor::getInstance().screenBuffer.sizePreviousLineChange)
        {
            // Draw from the currentLine
            display_setline(currentLine - 1);
            WP_clear_row(currentLine - 1);

            // and redraw the line
            WP_render_text_line(cursorLine - 1, display_y(), NULL);
        }
        // delete a line and redraw the line
        WP_clear_row(currentLine);

        // and redraw the lines after the current line
        WP_render_text_line(cursorLine, display_y(), NULL);
        // If it was not the last char that was deleted
        if (cursorLinePos != bufferSize)
        {

            // If it's not the last line, update the rest of the lines
            // of text since there positions may have changed.
            WP_clear_below_line(cursorLine);

            // Redraw the lines below the current line.
            for (int i = cursorLine + 1; i <= totalLine; i++)
            {
                // stop when exceeding row
                if (i - startLine > rows)
                {
                    debug_log("WP_render_text::reached the end of the text startLine %d i %d row %d\n", startLine, i, rows);
                    break;
                }
                WP_render_text_line(i, display_y(), display_EPD_framebuffer());
                // increase the line
                display_newline();
            }

            // render frame to the display
            display_draw_buffer();
        }
    }

    // Partial Refresh Logic
    else if (bufferSize != bufferSize_prev)
    {
        // new line
        // when new line is detected than redraw the previous line
        // in case when there is a word wrap then what was written at the previous line is moved to the current line
        if (cursorLine > cursorLine_prev)
        {
            // clear the currentLine and the previousLine
            debug_log("WP_render_text::new line is detected. clear the previous line to complete word wrap.\n");

            WP_clear_row(max(cursorLine_prev - startLine, 0));

            // and redraw the line
            WP_render_text_line(cursorLine_prev, display_y(), NULL);

            WP_clear_below_line(cursorLine);
            // Redraw the lines below the current line.
            for (int i = cursorLine + 1; i <= totalLine; i++)
            {
                // stop when exceeding row
                if (i - startLine > rows)
                {
                    debug_log("WP_render_text::reached the end of the text startLine %d i %d row %d\n", startLine, i, rows);
                    break;
                }
                WP_render_text_line(i, display_y(), display_EPD_framebuffer());
                // increase the line
                display_newline();
            }

            // render frame to the display
            display_draw_buffer();
        }
        ////////////////////////////////////////////////
        // if currently writing in the middle for editing then erase the current line
        // and subsequent lines until the end of the screen
        if (cursorPos != bufferSize)
        {
            // mark editing flag
            editing = true;

            //
            int currentLine = max(0, cursorLine - startLine);

            debug_log("WP_render_text::editing currentLine %d cursorLine %d cursorLineprev %d startLine %d\n", currentLine, cursorLine, cursorLine_prev, startLine);
            // If the previous line changed due to word wrap... -T.
            if (Editor::getInstance().screenBuffer.sizePreviousLineChange)
            {
                // Clear  the previous line T.
                display_setline(currentLine - 1);
                WP_clear_row(currentLine - 1);

                // and redraw it T.
                WP_render_text_line(cursorLine - 1, display_y(), NULL);
            }
            // Draw from the currentLine
            display_setline(currentLine);
            // delete a line and redraw the line
            WP_clear_row(currentLine);
            // and redraw the line
            WP_render_text_line(cursorLine, display_y(), NULL);
            // If the next line changed? .... T.
            if (Editor::getInstance().screenBuffer.sizeNextLineChanged)
            {
                // Clear all line below the current line -T.
                WP_clear_below_line(cursorLine);
                // And redraw them. -T.
                for (int i = cursorLine + 1; i <= totalLine; i++)
                {

                    // stop when exceeding row
                    if (i - startLine > rows)
                    {
                        debug_log("WP_render_text::reached the end of the text startLine %d i %d row %d\n", startLine, i, rows);
                        break;
                    }
                    WP_render_text_line(i, display_y(), display_EPD_framebuffer());
                    // increase the line
                    display_newline();
                }
                // render frame to the display
                display_draw_buffer();
            }
        }

        ////////////////////////////////////////////////

        //
        // Draw the new character entered
        else if (cursorPos != cursorPos_prev && cursorPos == bufferSize)
        {
            // render entire line
            int cursorX = MARGIN_X + display_fontwidth() * cursorLinePos_prev;
            display_setline(cursorLine - startLine);
            int cursorY = display_y();

            char *line = Editor::getInstance().screenBuffer.line_position[cursorLine];
            int line_length = Editor::getInstance().screenBuffer.line_length[cursorLine];

       
            // Copy the line content to row, but not more than 255 characters
            char row[256];
            int copyLength = (line_length < 255) ? line_length : 255;
            strncpy(row, line, copyLength);
            row[copyLength] = '\0';
            char utf8[512];
            convert_extended_ascii_to_utf8(row + cursorLinePos_prev, utf8, 512);                                  //
            epd_poweron();
            writeln(display_EPD_font(), utf8, &cursorX, &cursorY, NULL);
            epd_poweroff();
        }
        if (cursorLine_prev != cursorLine && cursorPos == bufferSize)
        {
            cursorLine_prev = cursorLine;
              WP_render_text_line(cursorLine, display_y(), NULL);
        }
          
            
    }
    cursorLine_prev = cursorLine;
    // reset prev flags
    cursorPos_prev = cursorPos;
    cursorLinePos_prev = cursorLinePos;
    bufferSize_prev = bufferSize;
}

//
// Render Cursor
#define CURSOR_MARGIN 15
#define CURSOR_THICKNESS 5
#define CURSOR_DELAY 50
void WP_render_cursor()
{
    JsonDocument &app = app_status();

    // don't render at the round when screen is cleared
    // if (cleared)
    //  return;

    // Cursor information
    static int renderedCursorX = -1;
    static int last = 0;

    static int cursorLinePos_prev = 0;
    static int cursorPos_prev = 0;
    int cursorLinePos = Editor::getInstance().fileBuffer.cursorLinePos;
    int cursorLine = Editor::getInstance().fileBuffer.cursorLine;
    int cursorPos = Editor::getInstance().fileBuffer.cursorPos;

    static Rect_t area;

    //
    //

    // Calculate Cursor X position
    // reached the line where cursor is
    // distance X is cursorPos - pos
    int cursorX = MARGIN_X;
    if (Editor::getInstance().fileBuffer.buffer[max(cursorPos - 1, 0)] != '\n' && cursorLinePos != 0)
    {
        // where to display the cursor
        cursorX = MARGIN_X + cursorLinePos * display_fontwidth() + 5;

        //
        // debug_log("WP_render_cursor::cursorX %d\n", cursorX);
    }

    // Delete previous cursor line
    if (cursorPos != cursorPos_prev)
    {
        if (renderedCursorX > 0)
        {
            //
            // debug_log("Delete previous cursor line\n");

            //
            area = display_rect(
                area.x - 10,
                area.y - 5,
                area.width + 20,
                area.height + 10);

            epd_poweron();
            epd_clear_quick(area, 8, 50);
            epd_poweroff_all();

            // render the cursor
            renderedCursorX = -1;
        }

        // reset the timer
        last = millis();

        //
        cursorPos_prev = cursorPos;
        cursorLinePos_prev = cursorLinePos;
    }

    // when there are no types for a short duration then
    // display the cursor
    if (renderedCursorX == -1 && last + CURSOR_DELAY < millis())
    {
        // Cursor will be always at the bottom of the screen
        area = display_rect(
            MARGIN_X + cursorLinePos * display_fontwidth(),
            MARGIN_Y + CURSOR_MARGIN + +display_lineheight() * (max(cursorLine - startLine, 0)),
            display_fontwidth() * abs(cursorLinePos - cursorLinePos_prev + 1),
            CURSOR_THICKNESS);

        //
        epd_fill_rect(area.x, area.y, area.width, area.height, 0, display_EPD_framebuffer());
        display_draw_buffer();

        //
        renderedCursorX = cursorX;

        //
        debug_log("WP_render_cursor::pos %d line %d line pos %d\n", cursorPos, cursorLine, cursorLinePos);
    }
}

// Check if text is saved
void WP_check_saved()
{
    //
    static unsigned int last = millis();
    static int lastBufferSize = Editor::getInstance().fileBuffer.getBufferSize();
    int bufferSize = Editor::getInstance().fileBuffer.getBufferSize();

    //
    // when the file is saved then extend the autosave timer
    if (lastBufferSize != bufferSize)
    {
        last = millis();

        //
        lastBufferSize = bufferSize;
    }

    //
    // when idle for 4 seconds then auto save
    if (millis() - last > 4000)
    {
        //
        last = millis();

        if (!Editor::getInstance().saved)
        {
            Editor::getInstance().saveFile();
        }
    }
}

void WP_check_sleep()
{
    // 600 seconds - 10 minutes
    if (millis() - last_sleep > 10 * 60 * 1000)
    {
        // if no action for 10 minute go to sleep
        last_sleep = millis();

        //
        JsonDocument &app = app_status();
        app["screen"] = SLEEPSCREEN;
    }
}

// display file number
// display character total
// display keyboard layout - Rev.5 and Rev.7
// display save status
// FILE INDEX | BYTES | SAVED | LAYOUT
// 160 | 200 ---- 200 | 100
#define STATUS_REFRESH 300
void WP_render_status()
{
    // Changed - T.
    String label;

    JsonDocument &app = app_status();

    // status start Y position
    int cursorY = statusY + status_height - 8;

    
    static size_t filesize_prev = 0;
    size_t filesize = Editor::getInstance().fileBuffer.seekPos + Editor::getInstance().fileBuffer.getBufferSize();
    // status word count - T.
    int wordcount = Editor::getInstance().fileBuffer.updateWordCountTotal();

    // FILE SIZE DRAWS WHEN STOPPED EDITING FOR A WHILE
    static int last = millis();
    static bool debouncing = false;
    static bool fileSaveStatusChanged = false;
    // On edit or refresh...

    if (filesize != filesize_prev)
    {
        // debounce for status_refresh amount
        last = millis();
        filesize_prev = filesize;
        debouncing = true;

        
    }


    if ((debouncing == true && last + STATUS_REFRESH < millis()))
    {
        last = millis();
        debouncing = false;
        WP_render_dynamic_status(false);
    }

    /////////////////////////////////////
    // DISPLAY SAVED STATE
    /////////////////////////////////////
     //
   
    String savedText = "NOT SAVED";
    
    if (Editor::getInstance().saved)
    
        // file is saved
        savedText = "SAVED";
    else 
        savedText = "NOT SAVED"; 

   
    if (!fileStatus.equals(savedText))
        fileSaveStatusChanged = true;
    else
        fileSaveStatusChanged = false;
   
    if (fileSaveStatusChanged || clear_full)
    {
      WP_render_fileSavedStatus(savedText);
      fileStatus = savedText;
    }

    

    /////////////////////////////////////
}

void WP_render_static_status()
{
    ////////////////////////////////////////
    JsonDocument &app = app_status();
    // File Index
    int cursorX = 25;
    // status start Y position
    int cursorY = statusY + status_height - 8;
    String label = format("FILE %d", app["config"]["file_index"].as<int>());
    writeln((GFXfont *)&systemFont, label.c_str(), &cursorX, &cursorY, display_EPD_framebuffer());

    // Char count C:
    cursorX = 200;
    label = "C: ";
    writeln((GFXfont *)&systemFont, label.c_str(), &cursorX, &cursorY, display_EPD_framebuffer());

    // Char count W:
    cursorX = 400;
    label = "W: ";
    writeln((GFXfont *)&systemFont, label.c_str(), &cursorX, &cursorY, display_EPD_framebuffer());

    ////////////////////////////////////////
    // Keyboard Layout
    label = app["config"]["keyboard_layout"].as<String>();
    if (label == "null" || label.isEmpty())
        label = "US"; // defaults to US layout
    cursorX = 860;
    writeln((GFXfont *)&systemFont, label.c_str(), &cursorX, &cursorY, display_EPD_framebuffer());
}

void WP_render_dynamic_status(bool firstRender)
{
    
    size_t filesize = Editor::getInstance().fileBuffer.seekPos + Editor::getInstance().fileBuffer.getBufferSize();
    // status word count - T.
    int wordcount = Editor::getInstance().fileBuffer.updateWordCountTotal();
    // Char count C:
    int cursorX = 200;
    // status start Y position
    int cursorY = statusY + status_height - 8;

    // redraw the new number
    // Changed - T.
    String filesizeFormatted = formatNumber(filesize);
    String label = "C: ";
    // Adjust positon to exclude label from clearing.
    cursorX = cursorX + label.length() * fontWidth;
    int eraseWidth = fontWidth * 6;
    String info = filesizeFormatted;

    // remove previous text
    Rect_t area = display_rect(
        cursorX,
        statusY,
        eraseWidth,
        status_height);

    epd_poweron();
    epd_clear_quick(area, 4, 50);

    writeln((GFXfont *)&systemFont, info.c_str(), &cursorX, &cursorY, NULL);

    // update word count only if it changed.
    if (wordcount != lastWordCount || firstRender)
    {
        lastWordCount = wordcount;
        // Redraw the word count -T.
        cursorX = 400;
        String wordcountFormatted = formatNumber(wordcount);
        label = "W: ";
        cursorX = cursorX + label.length() * fontWidth;
        eraseWidth = fontWidth * 6;
        info = wordcountFormatted;

        // remove previous text
        area = display_rect(
            cursorX,
            statusY,
            eraseWidth,
            status_height);

        epd_poweron();
        epd_clear_quick(area, 4, 50);

        writeln((GFXfont *)&systemFont, info.c_str(), &cursorX, &cursorY, NULL);
    }
    epd_poweroff_all();

   
}

//
void WP_keyboard(char key)
{
    // Every key stroke resets sleep timer
    last_sleep = millis();

    //
    JsonDocument &app = app_status();

    // Check if menu key is pressed
    if (key == MENU)
    {
        // Save before transitioning to the menu
        Editor::getInstance().saveFile();

        app["screen"] = MENUSCREEN;

        //
        debug_log("WP_keyboard::Moving to Menu Screen\n");
    }

    // REFRESH SCREEN F5
    else if (key == 5)
    {
        clear_full = true;
        Editor::getInstance().saveFile();
    }

    // SLEEP BUTTON - PAUSE
    else if (key == 24)
    {
        // go to sleep mode
        app["screen"] = SLEEPSCREEN;
    }

    else
    {
        // convert tab key to space
        if (key == '\t')
            key = ' ';

        // send the keys to the editor
        Editor::getInstance().keyboard(key);

        if (key == '\b')
        {
            backspaced = true;
        }
    }
}

//
void convert_extended_ascii_to_utf8(const char *input, char *output, size_t output_size)
{
    size_t out_index = 0;

    for (size_t i = 0; input[i] != '\0' && out_index < output_size - 1; i++)
    {
        unsigned char ch = (unsigned char)input[i];

        if (ch < 128)
        {
            // ASCII character, copy directly.
            output[out_index++] = ch;
        }
        else
        {
            // Extended ASCII, encode in UTF-8 (2 bytes).
            if (out_index + 2 < output_size)
            {
                output[out_index++] = 0xC0 | (ch >> 6);   // First byte: 110xxxxx
                output[out_index++] = 0x80 | (ch & 0x3F); // Second byte: 10xxxxxx
            }
            else
            {
                // Not enough space for encoding.
                break;
            }
        }
    }

    // Null-terminate the output.
    output[out_index] = '\0';
}


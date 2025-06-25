#ifndef WORDPROCESSOR_h
#define WORDPROCESSOR_h

#include <Arduino.h>


//
void WP_setup();

// 
void WP_render();

// 
void WP_keyboard(char key);

//
void WP_render_cursor();

// Renders file saved or no to the status bar - T.
void WP_render_fileSavedStatus(String status); 
void WP_render_text();
void WP_render_text_line(int i, int cursorY);
void WP_clear_row(int row);
// Clears the lines below the current cursor line - T.
void WP_clear_below_line(int cursorLine);
void WP_render_status();
// Renders the static labels in the status bar
void WP_render_static_status();
// Renders the character and word count to the status bar
// Parameter bool firstRender is true when the file is being
// loaded for the first time or after a manual refresh of the screen -T.
void WP_render_dynamic_status(bool firstRender);
void WP_check_saved();
void WP_check_sleep();

//
void convert_extended_ascii_to_utf8(const char *input, char *output, size_t output_size);

#endif
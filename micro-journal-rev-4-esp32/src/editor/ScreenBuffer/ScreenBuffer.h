#include "editor/editor.h"

#ifndef ScreenBuffer_h
#define ScreenBuffer_h

#define SCREEN_BUFFER_SIZE 4000

//
class ScreenBuffer
{
public:
    //
    int rows = 10;
    int cols = 26;

    //
    char *line_position[SCREEN_BUFFER_SIZE + 2];
    int line_length[SCREEN_BUFFER_SIZE + 2];
    int total_line = 0;

    bool clear_later = false;
    unsigned int clear_last = 0;
    // store the size of the line above and below current cursor line
    int sizeNextLine = -1; 
    int sizePreviousLine  = -1;
    // if the adjacent to the current curson line changed in size 
    bool sizeNextLineChanged = false;
    bool sizePreviousLineChange = false;
    // Update the buffer
    // Parameters: ileBuffer &fileBuffer and bool edit 
    // edit is true if the Update is called after an edit operation
    // eidt is false for cursor moments
    void Update(FileBuffer &fileBuffer, bool edit);
    // Maintains info regarding lines above and below current curror line
    // Checks if an edit operation in current line has 
    // changed the lines above or below it (due to wrapping)
    // Parameters: FileBuffer &fileBuffer, bool edit
    // edit is true if the Update is called after an edit operation
    // eidt is false for cursor moments
    void UpdateLineInfo (FileBuffer &fileBuffer, bool edit);
  
};

#endif
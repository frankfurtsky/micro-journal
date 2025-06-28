#include "ScreenBuffer.h"
#include "app/app.h"
#include "display/display.h"

//
void ScreenBuffer::Update(FileBuffer &fileBuffer, bool edit)
{
    // Loop through the text buffer
    // and product the data structure that is splitted in each line
    char *buffer = fileBuffer.getBuffer();

    // Handle empty buffer
    if (buffer[0] == '\0')
    {
        total_line = 0;
        line_position[0] = buffer;
        line_length[0] = 0;
        fileBuffer.cursorLine = 0;
        fileBuffer.cursorLinePos = 0;
        return;
    }

    // first line is the first of the buffer
    line_position[0] = &buffer[0];
    line_length[0] = 0;

    //
    this->total_line = 0;
    int line_count = 0;

    // remember the last space position to use it for the word wrap
    int last_space_index = -1;
    int last_space_position = -1;

    //
    // BUFFER -> SPLIT IN LINES
    //
    for (int i = 0; i < BUFFER_SIZE; i++) // Fixed loop condition
    {
        // When reaching the end of text, break
        if (buffer[i] == '\0')
        {
            // Update the length of the last line
            line_length[total_line] = line_count;

            //
            break;
        }

        // Count total characters in a line
        line_count++;

        // Track the position of the last space
        if (buffer[i] == ' ')
        {
            last_space_index = i;
            last_space_position = line_count;
        }

        // Handle words longer than `cols`
        if (line_count == cols && last_space_index == -1)
        {
            // Register the line count
            line_length[total_line] = line_count;

            // Start a new line
            line_position[++total_line] = &buffer[i + 1];

            // Reset counters
            line_count = 0;
            last_space_index = -1;
            last_space_position = -1;

            continue;
        }
        // When receiving a newline or max characters reached, start a new line
        if (buffer[i] == '\n' || line_count == cols)
        {
            // when ENTER key is found
            if (buffer[i] == '\n')
            {
                // register the line count
                line_length[total_line] = line_count;

                // start of the new line
                line_position[++total_line] = &buffer[i + 1];

                // reset counters
                line_count = 0;
            }

            // This line requires word-wrap
            else if (last_space_index != -1 && buffer[i] != '\n')
            {
                // register the line position as the last space position
                line_length[total_line] = last_space_position;

                // new line starts from just after the space
                line_position[++total_line] = &buffer[last_space_index + 1];

                // new line count starts from the wrapped word
                line_count -= last_space_position;
            }

            // This line doesn't requrie word wrap
            else
            {
                // register the line count
                line_length[total_line] = line_count;

                //
                line_position[++total_line] = &buffer[i + 1];

                //
                line_count = 0;
            }

            // reset the word wrap flags
            last_space_index = -1;
            last_space_position = -1;
        }
    }

    // Handle cursor position beyond buffer
    if (fileBuffer.cursorPos >= BUFFER_SIZE)
    {
        fileBuffer.cursorPos = BUFFER_SIZE - 1;
    }

    //
    // CALCULATE CURSOR INFORMATION
    //
    char *pCursorPos = &fileBuffer.buffer[fileBuffer.cursorPos];

    //
    fileBuffer.cursorLine = 0;
    fileBuffer.cursorLinePos = line_length[0];

    // caculate the which line cursor is located and the line position
    for (int i = total_line; i >= 0; i--)
    {
        //
        if (pCursorPos >= line_position[i])
        {

            // found the line index
            fileBuffer.cursorLine = i;

            // calculate the cursor position within the line
            fileBuffer.cursorLinePos = pCursorPos - line_position[i];
            break;
        }
    }

    UpdateLineInfo(fileBuffer, edit);
}

/**
 * @brief   Update info about lines above and below the current cursor.
 * Author: T. 
 * This fuction maintains info about the lines above and below the
 * current cursor. In case, while editing, the lines above or
 * below have been changed (due to word wraping) the flags
 * sizeNextLineChanged & sizePreviousLineChange are set to true.
 * On cursor moment, these flags are reset.
 *
 * @param[in]  fileBuffer  Current file bufffer.
 * @param[in]  edit        should be True if called during edit operation; false otherwise
 * @param[out] sizeNextLineChanged
 * @param[out] sizePreviousLineChange
 * @return     None.
 */
void ScreenBuffer::UpdateLineInfo(FileBuffer &fileBuffer, bool edit)
{
    if (total_line >= 1)
    {

        // Called not during edit? Reset to
        // just update information
        if (!edit)
        {
            sizeNextLine = -1;
            sizePreviousLine != -1;
        }

        // Last line?
        if (fileBuffer.cursorLine <= total_line - 1)
        {
            // The text is being edited...check for changes in the line below the current cursor
            if (sizeNextLine != -1 && line_length[fileBuffer.cursorLine + 1] != sizeNextLine)
                sizeNextLineChanged = true;
            else
                sizeNextLineChanged = false;
            // Update
            sizeNextLine = line_length[fileBuffer.cursorLine + 1];
        }

        // Minimum two lines...
        if (total_line > 1)
            if (fileBuffer.cursorLine >= 1)
            {
                // The text is being edited...check for changes in the line above the current cursor
                if (sizePreviousLine != -1 && line_length[fileBuffer.cursorLine - 1] != sizePreviousLine)
                    sizePreviousLineChange = true;
                else
                    sizePreviousLineChange = false;
                // Update
                sizePreviousLine = line_length[fileBuffer.cursorLine - 1];
            }
            else
            {
                sizePreviousLine != -1;
                sizePreviousLineChange = false;
            }
    }
}

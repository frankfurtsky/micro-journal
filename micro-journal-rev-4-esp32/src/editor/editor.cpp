#include "editor.h"
#include "app/app.h"
#include "display/display.h"
#include "keyboard/keyboard.h"

//
#include <FS.h>
#include <SD.h>

// Given the fileName, go through the loading process
// initialize FileBuffer and ScreenBuffer
void Editor::loadFile(String fileName)
{
    // do not load file while there are on-going process
    if (locked)
        return;

    // lock
    locked = true;

    // app status
    JsonDocument &app = app_status();

    //
    app_log("Editor loading file %s\n", fileName.c_str());

    // if no current file is specified then skip
    if (fileName.isEmpty())
    {
        app["error"] = format("Filename is empty. Can't load.\n");
        app_log(app["error"].as<const char *>());
        app["screen"] = ERRORSCREEN;

        // unlock
        locked = false;

        return;
    }

    // Step 1. Create file if necessary
    if (!SD.exists(fileName))
    {
        app_log("Creating an empty file since it's new\n");
        File file = SD.open(fileName, FILE_WRITE);
        if (!file)
        {
            app["error"] = format("Failed to create a file. %s\n", fileName);
            app_log(app["error"].as<const char *>());
            app["screen"] = ERRORSCREEN;

            // unlock
            locked = false;

            return;
        }

        //
        file.close();
        delay(100);

        //
        app_log("File created. %s\n", fileName);
    }

    // Step 2. Initialize the FileBuffer
    fileBuffer.load(fileName);

    // File just loaded 
    // so update the screen buffer - T.

    screenBuffer.Update(fileBuffer, false);

    // unlock
    locked = false;
}

//
void Editor::saveFile()
{
    //
    JsonDocument &app = app_status();

    // if no current file is specified then skip
    if (fileBuffer.getFileName().isEmpty())
    {
        app["error"] = format("Filename is empty. Can't save.\n");
        app_log(app["error"].as<const char *>());
        app["screen"] = ERRORSCREEN;

        return;
    }

    // if already save nothing to do
    if (this->saved)
    {
        app_log("File already saved\n");
        return;
    }

    //
    fileBuffer.save();

    // flag to save
    this->saved = true;
}

// Make the current file empty
void Editor::clearFile()
{
    //
    JsonDocument &app = app_status();

    // if no current file is specified then skip
    String fileName = fileBuffer.getFileName();
    if (fileName.isEmpty())
    {
        app["error"] = format("Filename is empty. Can't clear.\n");
        app_log(app["error"].as<const char *>());
        app["screen"] = ERRORSCREEN;

        return;
    }

    // Step 1. Check if the backup file exists
    // remove it if already exists
    String backupFileName = format("/%s_backup.txt", fileBuffer.getFileName());
    if (SD.exists(backupFileName))
    {
        // remove the backup file
        SD.remove(backupFileName);
    }

    // Step 2. Rename the current file to the backup.txt
    if (SD.rename(fileBuffer.getFileName(), backupFileName))
    {
        app_log("File renamed successfully: %s.\n", backupFileName);
    }
    else
    {
        app["error"] = format("Error making a backup file. %s\n", backupFileName);
        app_log(app["error"].as<const char *>());
        app["screen"] = ERRORSCREEN;

        return;
    }

    // Step 3. Empty the current file by opening it with FILE_WRITE
    File file = SD.open(fileName, FILE_WRITE);
    if (!file)
    {
        app["error"] = format("Failed to create an empty file %s\n", fileName);
        app_log(app["error"].as<const char *>());
        app["screen"] = ERRORSCREEN;

        return;
    }

    // clean up file
    file.close();
    delay(100);

    // Step 4. Go through the loading process of the empty file
    loadFile(fileName);
}

// Handle Keyboard Input
void Editor::keyboard(char key)
{
    debug_log("Editor::keyboard:: %c\n", key);

    //////////////////////////
    // BACKWARD EDITING
    //////////////////////////
    if (key == '\b')
    {
        // buffer has more than 1 character
        if (fileBuffer.getBufferSize() > 0)
        {
            fileBuffer.removeLastChar();
            // set saved flag to false
            this->saved = false;
            // The file has been edited,
            // so update the screen buffer - .T
            screenBuffer.Update(fileBuffer, true);
        }
        // buffer emptied
        else
        {
            // Load previous contents from the file if at the beginning of the buffer
            app_log("Backspace reached the beginning of the buffer\n");

            //
            saveFile();

            //
            loadFile(fileBuffer.fileName);
        }
    }

    // DEL key deletes the word
    else if (key == DEL)
    {
        if (fileBuffer.getBufferSize() > 0)
        {
           fileBuffer.removeLastWord();
           // The file has been edited,
            // so update the screen buffer - .T
            screenBuffer.Update(fileBuffer, true);
            // set saved flag to false
            this->saved = false;
        }
        // buffer emptied
        else
        {
            // Load previous contents from the file if at the beginning of the buffer
            app_log("Delete word reached the beginning of the buffer\n");

            //
            saveFile();

            //
            loadFile(fileBuffer.fileName);
        }
    }
    else if (key == ALT_LEFT || key == ALT_RIGHT)
    {
        // if ALT + LEFT or ALT + RIGHT
        // then move the cursor to the start of the word
        // or to the end of the word
        if (key == ALT_LEFT)
        {
            fileBuffer.moveCursorOneWordLeft();
        }
        else if (key == ALT_RIGHT)
        {
            fileBuffer.moveCursorOneWordRight();
        }

        // The cursor was moved,
        // so update the screen buffer - .T
        screenBuffer.Update(fileBuffer, false);
        
    }

    //////////////////////////
    // CURSORS
    //////////////////////////
    else if (key >= 18 && key <= 23 || key == 2 || key == 3)
    {
        // arrow keys
        // 18 - Left, 19 - Right, 20 - Up, 21 - Down
        // 22 - Page Up, 23 - Page Down
        // 2 - Home 3 - End
        if (key == 18)
        {
            if (fileBuffer.cursorPos == 0)
            {
                // load previous page
            }
            else
            {
                // left
                --fileBuffer.cursorPos;
            }
        }
        else if (key == 19)
        {
            // cursor can't move outside the last text
            if (fileBuffer.cursorPos < fileBuffer.getBufferSize())
                ++fileBuffer.cursorPos;
        }

        // UP
        else if (key == 20)
        {
            // move the cursorPos to the start of the previous line
            if (fileBuffer.cursorLine > 0)
            {
                // look at the previous line and move to the start of the cursor
                int newCursorPos =
                    screenBuffer.line_position[fileBuffer.cursorLine - 1] - screenBuffer.line_position[0];

                // cusor position in the line
                int cursorLinePos = fileBuffer.cursorLinePos;

                // if previous line length is shorter than cursorLinePos
                int previousLineLength = screenBuffer.line_length[fileBuffer.cursorLine - 1];
                if (previousLineLength < cursorLinePos)
                {
                    // then move to the end of the previous line
                    newCursorPos += previousLineLength-1;
                }
                else
                {
                    // if previous line length is long enough then move as much as the currentLineCursorPos
                    newCursorPos += cursorLinePos;
                }

                // edge case
                if (newCursorPos < 0)
                    newCursorPos = 0;

                //
                fileBuffer.cursorPos = newCursorPos;
            }
        }

        // DOWN
        else if (key == 21)
        {
            // move the cursorPos to the start of the next line
            if (fileBuffer.cursorLine < screenBuffer.total_line)
            {
                //
                int newCursorPos =
                    screenBuffer.line_position[fileBuffer.cursorLine + 1] - screenBuffer.line_position[0];

                // cusor position in the line
                int cursorLinePos = fileBuffer.cursorLinePos;

                // next line length
                int nextLineLength = max(screenBuffer.line_length[fileBuffer.cursorLine + 1], 1);

                // if next line has shorter length of the current cursor pos 
                // then move to the end of the next line
                if (cursorLinePos > nextLineLength)
                {
                    newCursorPos += nextLineLength -1;
                }
                else
                {
                    // 
                    newCursorPos += cursorLinePos;
                }

                // handle edge case
                if (newCursorPos > fileBuffer.bufferSize)
                    newCursorPos = fileBuffer.bufferSize;

                //
                fileBuffer.cursorPos = newCursorPos;
                fileBuffer.cursorLine += 1;
            }

            // when trying to go down at the last line, move the cursor to the end
            else if (fileBuffer.cursorLine == screenBuffer.total_line)
            {

                // if last line then move to the end of the buffer
                fileBuffer.cursorPos = fileBuffer.getBufferSize();

                debug_log("Editor::keyboard::DOWN last line condition met cursorPos %d\n",
                          fileBuffer.cursorPos);
            }
        }

        // HOME
        else if (key == 2)
        {
            // home - move to the start of the line
            int newCursorPos =
                screenBuffer.line_position[fileBuffer.cursorLine] - screenBuffer.line_position[0];
            //
            fileBuffer.cursorPos = newCursorPos;
        }

        // END
        else if (key == 3)
        {
            // end - move to the end of the line
            int line_length = max(screenBuffer.line_length[fileBuffer.cursorLine], 1);
            int newCursorPos =
                screenBuffer.line_position[fileBuffer.cursorLine] - screenBuffer.line_position[0] + line_length - 1;

            // if last line then move to the end of the buffer
            if (fileBuffer.cursorLine == screenBuffer.total_line)
                newCursorPos = fileBuffer.getBufferSize();

            //
            fileBuffer.cursorPos = newCursorPos;
        }

        // PAGE UP
        else if (key == 22)
        {
            int newCursorLine = max(fileBuffer.cursorLine - screenBuffer.rows, 0);
            int newCursorPos =
                screenBuffer.line_position[newCursorLine] - screenBuffer.line_position[0];

            //
            fileBuffer.cursorPos = newCursorPos;
        }

        // PAGE DOWN
        else if (key == 23)
        {
            int newCursorLine = min(fileBuffer.cursorLine + screenBuffer.rows, screenBuffer.total_line);
            int line_length = max(screenBuffer.line_length[newCursorLine], 1);
            int newCursorPos =
                screenBuffer.line_position[newCursorLine] - screenBuffer.line_position[0] + line_length - 1;

            // if last line then move to the end of the buffer
            if (fileBuffer.cursorLine == screenBuffer.total_line)
                newCursorPos = fileBuffer.getBufferSize();

            //
            fileBuffer.cursorPos = newCursorPos;
        }
        // Cursor moved,
        // update the screen buffer -T.
        screenBuffer.Update(fileBuffer, false);
    }

    //////////////////////////
    // FORWARD EDITING
    //////////////////////////
    else
    {
        // add to the edit buffer new character
        if (!fileBuffer.available())
        {
            app_log("Text buffer full\n");

            //
            saveFile();

            //
            loadFile(fileBuffer.fileName);
        }

        //
        fileBuffer.addChar(key);
        // set saved flag to false
        this->saved = false;
        

     // update the word counter -  T.
    
    fileBuffer.updateWordCountTotal();
    
    // The file has been edited so
    // update the screen buffer - T.
    screenBuffer.Update(fileBuffer, true);
    }
}

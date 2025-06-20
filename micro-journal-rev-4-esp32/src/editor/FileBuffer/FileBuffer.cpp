#include "FileBuffer.h"
#include "app/app.h"
#include "display/display.h"

//
#include <FS.h>
#include <SD.h>

//
int FileBuffer::getSeekPos()
{
    return seekPos;
}

//
String FileBuffer::getFileName()
{
    return fileName;
}

// Fill the buffer from File
void FileBuffer::load(String fileName)
{
    // app status
    JsonDocument &app = app_status();

    //
    if (fileName.isEmpty())
    {
        //
        app["error"] = "Load file failed. File name is empty.";
        app["screen"] = ERRORSCREEN;

        return;
    }

    // Save filen name
    this->fileName = fileName;

    // Check if the file exists, create if not
    if (!SD.exists(fileName))
    {
        File file = SD.open(fileName, FILE_WRITE);
        if (!file)
        {
            //
            file.close();
            delay(100);

            //
            app["error"] = "Failed to create file";
            app["screen"] = ERRORSCREEN;

            return;
        }

        //
        file.close();
        delay(100);
    }

    // open the text file
    app_log("Loading file %s\n", fileName);
    File file = SD.open(fileName);
    if (!file)
    {
        //
        app["error"] = format("file open failed %s\n", fileName);
        app["screen"] = ERRORSCREEN;
        app_log(app["error"].as<const char *>());

        return;
    }

    // Determine file size and set buffer accordingly
    fileSize = file.size();

    
    app_log("File: %s of size: %d\n", fileName, fileSize);

    // calcualte the file offset
    seekPos = 0;
    int stepSize = BUFFER_SIZE/2; // use half of the buffer
    // Changed - .T
    /* if (fileSize > 0)
    {
        // this offset will offer last portion of the buffer
        seekPos = (fileSize / stepSize) * stepSize;
    }

    // when it is exactly the buffer end
    // go one buffer behind so that screen will show something
    if (fileSize == seekPos && seekPos > 0)
    {
        if (seekPos > stepSize)
            seekPos -= stepSize;
        else
            // defensive code in order for the offset not to go negative (MAX in unsigned in)
            seekPos = 0;
    } */
   
    seekPos =  fileSize > stepSize ? fileSize - stepSize : 0;

    // move the file position to offset
    if (!file.seek(seekPos))
    {
        //
        file.close();
        delay(100);

        //
        app["error"] = format("Failed to seek file pointer. fileSize: %d seekPos: %d\n", fileSize, seekPos);
        app["screen"] = ERRORSCREEN;
        app_log(app["error"].as<const char *>());

        return;
    }

    // reset the buffer
    reset();

    // Read file content into text buffer
    bufferSize = 0;
    while (file.available())
    {
        char c = file.read();
        buffer[bufferSize++] = c;
    }
    cursorPos = bufferSize;
    buffer[bufferSize] = '\0';
    
    initializeWordCount(file);   
    file.close();
    delay(100);

    

    // log
    debug_log("FileBuffer::load::Read size: %d, seek: %d, buffer: %d, cursor: %d\n", fileSize, seekPos, bufferSize, cursorPos);
}

void FileBuffer::save()
{
    // app status
    JsonDocument &app = app_status();

    //
    app_log("Saving file %s\n", fileName);
    File file = SD.open(fileName, FILE_WRITE);
    if (!file)
    {
        //
        app["error"] = "Failed to open file for writing\n";
        app["screen"] = ERRORSCREEN;
        app_log(app["error"].as<const char *>());

        return;
    }

    // Seek to the last loaded offset
    if (!file.seek(seekPos))
    {
        app_log("Failed to seek file pointer\n");
        file.close();
        delay(100);

        return;
    }
    app_log("Writing file at: %d\n", seekPos);

    // writing the file content
    size_t length = file.print(buffer);
    if (length >= 0)
    {
        app_log("File written: %d bytes\n", length);
    }
    else
    {
        app["error"] = "Save failed\n";
        app["screen"] = ERRORSCREEN;
        app_log(app["error"].as<const char *>());
    }

    //
    file.close();
    delay(100);

    // recalculate the file size
    // calculate the file size
    file = SD.open(fileName, FILE_READ);
    if (!file)
    {
        app["error"] = "Failed to open file for reading\n";
        app_log(app["error"].as<const char *>());
        app["screen"] = ERRORSCREEN;

        //
        return;
    }

    //
    fileSize = file.size();

    file.close();
    
    delay(100);
    // update word count -T.
    updateWordCountTotal();
}

//
void FileBuffer::reset()
{
    //
    memset(buffer, '\0', sizeof(buffer));

    //
    bufferSize = 0;
}

//
char *FileBuffer::getBuffer()
{
    return buffer;
}

//
int FileBuffer::getBufferSize()
{
    return bufferSize;
}
// Count words in the buffer
int FileBuffer::getWordCountBuffer()
{
    // Function to count words in a C-style char buffer

    int count = 0;
    bool inWord = false;

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        if (std::isalnum(buffer[i])) {
            if (!inWord) {
                inWord = true;
                ++count;
            }
        } else {
            inWord = false;
        }
    }

    return count;

}
// Count words in the file (excluding the buffer)
int FileBuffer::getWordCountFile(File currentFile)
{

  int wordCount = 0;
  bool inWord = false;
  
  uint32_t fileSize = currentFile.size();

  // Size of the data not going into the buffer
  int sizeOfDataNotInBuffer = fileSize - (BUFFER_SIZE/2);
  
  //Move the pointer to start of the file
  currentFile.seek(0);
  int readLimit = (fileSize > (BUFFER_SIZE/2)) ? sizeOfDataNotInBuffer : 0;
 
  app_log("read limit:  %d \n",readLimit);
  // Count the words until the part of the file that will go into the buffer
  uint32_t bytesRead = 0;
  while (currentFile.available() && bytesRead < readLimit) {
    char c = currentFile.read();
    bytesRead++;

    if (isAlphaNumeric(c)) {
      if (!inWord) {
        inWord = true;
        wordCount++;
      }
    } else {
      inWord = false;
    }
   
   }
// Handle the border case where the word in the
// file continues on to the buffer.
  if (fileSize > (BUFFER_SIZE/2))
   {
    currentFile.seek(readLimit);
    char firstCharInBuffer = currentFile.read();
    if (inWord && isAlphaNumeric(firstCharInBuffer))
     wordCount--;
   }
  //reset file postion pointer
  currentFile.seek(0);
  app_log("Words in file:  %d \n",wordCount);
  return wordCount;

}

int FileBuffer::updateWordCountTotal()
{
    // update word count -T.
    wordCountTotal =   wordCountFile + getWordCountBuffer();
    return wordCountTotal;
}

void FileBuffer::initializeWordCount(File file)
{
    // initialize word count total -T. 
    wordCountFile = getWordCountFile(file);
    wordCountBuffer = getWordCountBuffer();
    wordCountTotal = wordCountFile + wordCountBuffer;
    app_log("Initialize word count: File %d WordsInBuffer %d\n",wordCountFile, wordCountBuffer);
}

void FileBuffer::addChar(char c)
{
    if (bufferSize < BUFFER_SIZE)
    {
        // shift the trailing texts
        if (bufferSize > cursorPos)
            memmove(buffer + cursorPos + 1, buffer + cursorPos, bufferSize - cursorPos);

        //
        buffer[cursorPos++] = c;
        buffer[++bufferSize] = '\0';
        // Changed - T.
        updateWordCountTotal();
        debug_log("FileBuffer::addChar::cursorPos %d %c\n", cursorPos, c);
    }
}

void FileBuffer::removeLastChar()
{
    if (bufferSize > 0 && cursorPos > 0)
    {
        // Shift the trailing texts left by one position
        if (bufferSize > cursorPos)
        {
            memmove(buffer + cursorPos - 1, buffer + cursorPos, bufferSize - cursorPos);
        }

        // Decrease buffer size and cursor position
        --bufferSize;
        --cursorPos;
        // Changed - T.
        updateWordCountTotal();

        //
        debug_log("FileBuffer::removeLastChar %d\n", cursorPos);

        // Null terminate the buffer
        buffer[bufferSize] = '\0';
        // Changed - T.
        updateWordCountTotal();
    }
}

void FileBuffer::removeCharAtCursor()
{
    if (bufferSize > 0 && cursorPos < bufferSize)
    {
        // Shift the trailing text left by one position
        if (bufferSize > cursorPos + 1)
        {
            memmove(buffer + cursorPos, buffer + cursorPos + 1, bufferSize - cursorPos - 1);
        }

        // Decrease buffer size
        --bufferSize;

        // Null terminate the buffer
        buffer[bufferSize] = '\0';
        // Changed - T.
        updateWordCountTotal();
    }
}

void FileBuffer::removeLastWord()
{
    int length = bufferSize;
    if (length == 0)
        return;

    int end = length - 1;
    while (end >= 0 && buffer[end] == ' ')
        end--;

    if (end < 0)
        return;

    int start = end;
    while (start >= 0 && buffer[start] != ' ' && buffer[start] != '\n')
        start--;

    if (start <= 0)
    {
        start = 0;
        buffer[0] = '\0';
        bufferSize = 0;
    }
    else
    {
        buffer[start] = ' ';
        buffer[start + 1] = '\0';
        bufferSize = start + 1;
    }

    cursorPos = bufferSize;
    // Changed - T.
    updateWordCountTotal();

    //
    debug_log("FileBuffer::removeLastWord %d\n", cursorPos);
}

//
bool FileBuffer::available()
{
    // is there still buffer?
    return bufferSize < BUFFER_SIZE;
}
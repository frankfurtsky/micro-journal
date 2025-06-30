//
#ifndef FileBuffer_h
#define FileBuffer_h

//
#include <Arduino.h>
#include <cstring> // For memset

#define BUFFER_SIZE 4000
#include <FS.h>
#include <SD.h>
//
class FileBuffer
{
public:
    FileBuffer()
    {
        bufferSize = 0;
        fileSize = 0;
        seekPos = 0;
        cursorPos = 0;
        cursorLine = 0;
        cursorLinePos = 0;
    }

    //
    char buffer[BUFFER_SIZE + 2];
    int bufferSize;

    //
    String getFileName();
    //
    int getSeekPos();
    //
    void load(String fileName);
    void save();

    //
    String fileName;
    //
    size_t fileSize;
    //
    size_t seekPos;
    // holds the word count of the entire file - T.
    size_t wordCountFile;
    // holds the word count of the buffer - T.
    size_t wordCountBuffer;
    // holds the word count of the file data that is not in the current buffer - T.
    size_t wordCountTotal;
    //
    
    int cursorPos;
    int cursorLine;
    int cursorLinePos;

    //
    void reset();

    //
    char *getBuffer();

    //
    int getBufferSize();
    // calculate word count of the buffer - T.
    int getWordCountBuffer();
    // calculate word count of the file (excluding the buffer) - T.
    int getWordCountFile(File currentFile);
    // update total word count - T.
    int updateWordCountTotal();
    // count words  when the file is loaded - T.
    void initializeWordCount(File currentFile);

    void addChar(char c);
    void removeLastChar();
    void removeCharAtCursor();
    void removeLastWord();
    void moveCursorOneWordLeft();
    void moveCursorOneWordRight();
    

    //
    bool available();
};

#endif
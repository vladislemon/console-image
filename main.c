#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincon.h>

int main() {
    HANDLE consoleInput = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE screenBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    HANDLE imageFile = CreateFileA("image.wci", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    unsigned char magicAndHeader[10];
    DWORD bytesRead;
    ReadFile(imageFile, magicAndHeader, 10, &bytesRead, NULL);
    WORD width = (magicAndHeader[4] << 8) | (magicAndHeader[5]);
    WORD height = (magicAndHeader[6] << 8) | (magicAndHeader[7]);
    WORD paletteSize = (magicAndHeader[8] << 8) | (magicAndHeader[9]);
    unsigned char paletteData[paletteSize * 3];
    ReadFile(imageFile, paletteData, paletteSize * 3, &bytesRead, NULL);
    COLORREF colorTable[16];
    for(int i = 0; i < paletteSize && i < 16; i++) {
        colorTable[i] = paletteData[i*3] | (paletteData[i*3+1] << 8) | (paletteData[i*3+2] << 16);
    }
    unsigned char indices[width*height];
    ReadFile(imageFile, indices, width*height, &bytesRead, NULL);
    CloseHandle(imageFile);
    CHAR_INFO charInfo[width*height];
    for(int i = 0; i < width*height; i++) {
        charInfo[i].Char.AsciiChar = ' ';
        charInfo[i].Attributes = ((WORD)indices[i]) << 4;
    }


    CONSOLE_SCREEN_BUFFER_INFOEX consoleInfo;
    ZeroMemory(&consoleInfo, sizeof(consoleInfo));
    consoleInfo.cbSize = sizeof(consoleInfo);
    int success = GetConsoleScreenBufferInfoEx(screenBuffer, &consoleInfo);

    CONSOLE_SCREEN_BUFFER_INFOEX consoleInfoBackup;
    memcpy(&consoleInfoBackup, &consoleInfo, sizeof(consoleInfo));

    SetConsoleActiveScreenBuffer(screenBuffer);

    consoleInfo.dwSize.X = width;
    consoleInfo.dwSize.Y = height;
    consoleInfo.srWindow.Left = 0;
    consoleInfo.srWindow.Top = 0;
    consoleInfo.srWindow.Right = width;
    consoleInfo.srWindow.Bottom = height;
    consoleInfo.dwMaximumWindowSize.X = width;
    consoleInfo.dwMaximumWindowSize.Y = height;
    memcpy(consoleInfo.ColorTable, colorTable, sizeof(colorTable));
    SetConsoleScreenBufferInfoEx(screenBuffer, &consoleInfo);

    CONSOLE_FONT_INFOEX fontInfo;
    ZeroMemory(&fontInfo, sizeof(fontInfo));
    fontInfo.cbSize = sizeof(fontInfo);
    GetCurrentConsoleFontEx(screenBuffer, FALSE, &fontInfo);
    fontInfo.nFont = 7;
    fontInfo.dwFontSize.X = 8;
    fontInfo.dwFontSize.Y = 9;
    fontInfo.FontFamily = 48;
    SetCurrentConsoleFontEx(screenBuffer, FALSE, &fontInfo);

    COORD bufferSize = {width, height};
    COORD bufferCoord = {0, 0};
    SMALL_RECT writeRegion = {0, 0, width, height};
    WriteConsoleOutputA(screenBuffer, charInfo, bufferSize, bufferCoord, &writeRegion);

    char text[128];
    int size = wsprintfA(text, "Success: %d, Width: %d, Height: %d, Font: %d\n", success, consoleInfo.dwSize.X, consoleInfo.dwSize.Y, fontInfo.FontFamily);
    WriteFile(consoleOutput, text, size, NULL, NULL);

    INPUT_RECORD inputRecord;
    DWORD recordCount;
    do {
        ReadConsoleInputA(consoleInput, &inputRecord, 1, &recordCount);
    } while (inputRecord.EventType != KEY_EVENT || inputRecord.Event.KeyEvent.uChar.AsciiChar != 'q');

    SetConsoleActiveScreenBuffer(consoleOutput);
    SetConsoleScreenBufferInfoEx(consoleOutput, &consoleInfoBackup);

    return 0;
}
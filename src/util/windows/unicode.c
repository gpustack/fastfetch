#include "unicode.h"
#include <stdlib.h>
#include <windows.h>

void ffStrbufSetNWS(FFstrbuf* result, uint32_t length, const wchar_t* source)
{
    if(!length)
    {
        ffStrbufClear(result);
        return;
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, source, (int)length, NULL, 0, NULL, NULL);
    ffStrbufEnsureFree(result, (uint32_t)size_needed);
    WideCharToMultiByte(CP_UTF8, 0, source, (int)length, result->chars, size_needed, NULL, NULL);
    result->length = (uint32_t)size_needed;
    result->chars[size_needed] = '\0';
}

void ffStrbufInitNWS(FFstrbuf* result, uint32_t length, const wchar_t* source)
{
    if(!length)
    {
        ffStrbufInit(result);
        return;
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, source, (int)length, NULL, 0, NULL, NULL);
    ffStrbufInitA(result, (uint32_t)size_needed + 1);
    WideCharToMultiByte(CP_UTF8, 0, source, (int)length, result->chars, size_needed, NULL, NULL);
    result->length = (uint32_t)size_needed;
    result->chars[size_needed] = '\0';
}

wchar_t* ffStrbufToWideChar(const FFstrbuf* strbuf)
{
    if(strbuf->length == 0)
    {
        wchar_t* emptyStr = (wchar_t*)malloc(sizeof(wchar_t));
        emptyStr[0] = L'\0';
        return emptyStr;
    }

    int wcharCount = MultiByteToWideChar(CP_UTF8, 0, strbuf->chars, (int)strbuf->length, NULL, 0);
    if(wcharCount == 0)
    {
        return NULL;
    }

    wchar_t* wideStr = (wchar_t*)malloc((uint32_t)(wcharCount + 1) * sizeof(wchar_t));
    if(wideStr == NULL)
    {
        return NULL;
    }

    int32_t result = MultiByteToWideChar(CP_UTF8, 0, strbuf->chars, (int)strbuf->length, wideStr, wcharCount);
    if(result == 0)
    {
        free(wideStr);
        return NULL;
    }

    wideStr[wcharCount] = L'\0';

    return wideStr;
}
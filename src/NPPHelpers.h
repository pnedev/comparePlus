#ifndef NPPHELPERS_H
#define NPPHELPERS_H
HWND getCurrentWindow();
HWND getOtherWindow();
void markTextAsChanged(HWND window,int start,int length);
void markAsMoved(HWND window,int line);
void markAsRemoved(HWND window,int line);
void markAsChanged(HWND window,int line);
void markAsAdded(HWND window,int line);
void markAsBlank(HWND window,int line);
void setStyles(sColorSettings Settings);
void setBlank(HWND window,int color);
void ready();
void wait();
void setCursor(int type);
void setTextStyles();
void setTextStyle(HWND window);
void setChangedStyle(HWND window);
void defineSymbol(int type,int symbol);
void defineColor(int type,int color);
void clearWindow(HWND window,bool clearUndo);
void clearUndoBuffer(HWND window);
blankLineList *removeEmptyLines(HWND window,bool saveList);
int deleteLine(HWND window,int line);
char **getAllLines(HWND window,int *length, int **lineNum);
void addBlankLines(HWND window,blankLineList *list);
void addEmptyLines(HWND hSci, int offset, int length);
#endif
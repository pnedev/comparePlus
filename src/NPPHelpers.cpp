#include "Compare.h"
#include "NPPHelpers.h"
extern NppData nppData;
extern UINT	EOLtype;

int MarkerStart   = 0;
int removed       = MarkerStart + 7; // 24;
int removedSymbol = MarkerStart + 6; // 23;
int added         = MarkerStart + 5; // 22;
int addedSymbol   = MarkerStart + 4; // 21;
int changed       = MarkerStart + 3; // 20;
int changedSymbol = MarkerStart + 2; // 19;
int moved         = MarkerStart + 1; // 18;
int blank         = MarkerStart;     // 17;

HWND getCurrentWindow(){
	int win=3;
	SendMessage(nppData._nppHandle,NPPM_GETCURRENTSCINTILLA,0,(LPARAM)&win);
	HWND window=win==0?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
	return window;
}

HWND getOtherWindow(){
	int win=3;
	SendMessage(nppData._nppHandle,NPPM_GETCURRENTSCINTILLA,0,(LPARAM)&win);
	HWND window=win==1?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
	return window;
}


void defineColor(int type,int color){
	::SendMessageA(nppData._scintillaMainHandle, SCI_MARKERDEFINE,type, (LPARAM)SC_MARK_BACKGROUND);	
	::SendMessageA(nppData._scintillaMainHandle, SCI_MARKERSETBACK,type, (LPARAM)color);
	::SendMessageA(nppData._scintillaMainHandle, SCI_MARKERSETFORE,type, (LPARAM)0);
	::SendMessageA(nppData._scintillaSecondHandle, SCI_MARKERDEFINE,type, (LPARAM)SC_MARK_BACKGROUND);	
	::SendMessageA(nppData._scintillaSecondHandle, SCI_MARKERSETBACK,type, (LPARAM)color);
	::SendMessageA(nppData._scintillaSecondHandle, SCI_MARKERSETFORE,type, (LPARAM)0);
}
void defineSymbol(int type,int symbol){
	::SendMessageA(nppData._scintillaMainHandle, SCI_MARKERDEFINE,type, (LPARAM)symbol);
	::SendMessageA(nppData._scintillaSecondHandle, SCI_MARKERDEFINE,type, (LPARAM)symbol);
}


void setChangedStyle(HWND window, sColorSettings Settings){
	::SendMessageA(window, SCI_INDICSETSTYLE, 1, (LPARAM)7);
    ::SendMessageA(window, SCI_INDICSETFORE, 1, (LPARAM)Settings.highlight);
    ::SendMessageA(window, SCI_INDICSETALPHA, 1, (LPARAM)Settings.alpha);
}


void setTextStyle(HWND window, sColorSettings Settings){
	setChangedStyle(window, Settings);
}

void setTextStyles(sColorSettings Settings){

	setTextStyle(nppData._scintillaMainHandle, Settings);
	setTextStyle(nppData._scintillaSecondHandle, Settings);

}

void setCursor(int type){
	::SendMessageA(nppData._scintillaMainHandle, SCI_SETCURSOR,type, (LPARAM)type);
	::SendMessageA(nppData._scintillaSecondHandle, SCI_SETCURSOR,type, (LPARAM)type);
}
void wait(){
	setCursor(SC_CURSORWAIT);
}
void ready(){
	setCursor(SC_CURSORNORMAL);
}

void setBlank(HWND window,int color){
	::SendMessageA(window, SCI_MARKERDEFINE, blank, (LPARAM)SC_MARK_BACKGROUND);	
	::SendMessageA(window, SCI_MARKERSETBACK, blank, (LPARAM)color);
	::SendMessageA(window, SCI_MARKERSETFORE, blank, (LPARAM)color);
	//::SendMessage(window, SCI_STYLESETVISIBLE,blank, (LPARAM)0);
	//::SendMessage(window, SCI_STYLESETCHANGEABLE,blank, (LPARAM)0);
	//int i=::SendMessage(window, SCI_STYLEGETCHANGEABLE,blank, (LPARAM)0);
}


void setStyles(sColorSettings Settings)
{
    int MarginMask = (1 << changedSymbol) | (1 << addedSymbol) | (1 << removedSymbol);

    ::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINMASKN, (WPARAM)4, (LPARAM)MarginMask);
    ::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINMASKN, (WPARAM)4, (LPARAM)MarginMask);

    ::SendMessage(nppData._scintillaMainHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, (LPARAM)14);
    ::SendMessage(nppData._scintillaSecondHandle, SCI_SETMARGINWIDTHN, (WPARAM)4, (LPARAM)14);
        
    setBlank(nppData._scintillaMainHandle, Settings.blank);
	setBlank(nppData._scintillaSecondHandle, Settings.blank);

    defineColor(added, Settings.added);
	defineColor(changed, Settings.changed);
	defineColor(moved, Settings.moved);
	defineColor(removed, Settings.deleted);

	defineSymbol(addedSymbol, SC_MARK_PLUS); //SC_MARK_PLUS);
	defineSymbol(removedSymbol, SC_MARK_MINUS);
	defineSymbol(changedSymbol, SC_MARK_ARROWS); //SC_MARK_ARROWS);

	setTextStyles(Settings);
}

void markAsBlank(HWND window,int line)
{
	::SendMessageA(window, SCI_MARKERADD, line, blank);
}

void markAsAdded(HWND window,int line)
{
	//::SendMessageA(window, SCI_MARKERADDSET, line, (LPARAM)(pow(2.0,added)+pow(2.0,addedSymbol)));
    ::SendMessageA(window, SCI_MARKERADD, line, (LPARAM)addedSymbol);
    ::SendMessageA(window, SCI_MARKERADD, line, (LPARAM)added);
}
void markAsChanged(HWND window,int line)
{
	//::SendMessageA(window, SCI_MARKERADDSET, line, (LPARAM)(pow(2.0,changed)+pow(2.0,changedSymbol)));
    ::SendMessageA(window, SCI_MARKERADD, line, (LPARAM)changedSymbol);
    ::SendMessageA(window, SCI_MARKERADD, line, (LPARAM)changed);
}
void markAsRemoved(HWND window,int line)
{
	//::SendMessageA(window, SCI_MARKERADDSET, line, (LPARAM)(pow(2.0,removed)+pow(2.0,removedSymbol)));
    ::SendMessageA(window, SCI_MARKERADD, line, (LPARAM)removedSymbol);
    ::SendMessageA(window, SCI_MARKERADD, line, (LPARAM)removed);
}

void markAsMoved(HWND window,int line)
{
	::SendMessageA(window, SCI_MARKERADD, line, (LPARAM)moved);
}

void markTextAsChanged(HWND window,int start,int length)
{
	if(length!=0)
    {
		::SendMessageA(window, SCI_STARTSTYLING, start, (LPARAM)INDICS_MASK);
		::SendMessageA(window, SCI_SETSTYLING, length , (LPARAM)INDIC1_MASK);
	}
}

char **getAllLines(HWND window,int *length, int **lineNum){
	int docLines=SendMessageA(window, SCI_GETLINECOUNT, 0, (LPARAM)0);
	char **lines=new char*[docLines];
	*lineNum=new int[docLines];
	int textCount=0;
	for(int line=0;line<docLines;line++)
	{
		
		//clearLine(window,line);
		int lineLength=SendMessageA(window, SCI_LINELENGTH,line,  (LPARAM)0);
		(*lineNum)[line]=textCount;
		textCount+=lineLength;

		if(lineLength>0){
			lines[line] = new char[lineLength+1];
			::SendMessageA(window, SCI_GETLINE, line, (LPARAM)lines[line]);				
			int i=0;
			for(i=0;i<lineLength&& lines[line][i]!='\n' && lines[line][i]!='\r';i++);

			
			lines[line][i]=0;
			
					
		}else{
			lines[line]="";
		}

	}
	*length=docLines;
	return lines;
}
int deleteLine(HWND window,int line)
{
	int posAdd = ::SendMessageA(window, SCI_POSITIONFROMLINE, line, 0);
	::SendMessageA(window, SCI_SETTARGETSTART, posAdd, 0);
	int docLength=::SendMessageA(window, SCI_GETLINECOUNT, 0, 0)-1;
	int length=0;//::SendMessageA(window, SCI_LINELENGTH, line, 0);
	UINT EOLtype = ::SendMessageA(window,SCI_GETEOLMODE,0,0);

	//::SendMessageA(window,SCI_TARGETFROMSELECTION,0,0);
	int start=line;


	int lines=0;
	int marker=SendMessageA(window, SCI_MARKERGET, line, 0);
	//int blankMask=pow(2.0,blank);
    int blankMask = 1 << blank;
	while((marker&blankMask)!=0){
		int lineLength=::SendMessageA(window, SCI_LINELENGTH, line, 0);		
		
		//don't delete lines that actually have text in them
		if(line<docLength && lineLength>lenEOL[EOLtype] ){
			//lineLength-=lenEOL[EOLtype];
			break;
		}else if(line== docLength&& lineLength>0){			
			break;
		}
		lines++;
		length+=lineLength;
		line++;

		marker=SendMessageA(window, SCI_MARKERGET, line, 0);
	}
	

	//select the end of the lines, and unmark them so they aren't called for delete again
	::SendMessageA(window, SCI_SETTARGETEND, posAdd+length, 0);
	for(int i=start;i<line;i++){
		::SendMessageA(window, SCI_MARKERDELETE, i, blank);
	}

	//if we're at the end of the document, we can't delete that line, because it doesn't have any EOL characters to delete
	//, so we have to delete the EOL on the previous line
	if(line>docLength){
		::SendMessageA(window, SCI_SETTARGETSTART, posAdd-(lenEOL[EOLtype]), 0);
		length+=lenEOL[EOLtype];
	}
	if(length>0){
		::SendMessageA(window, SCI_MARKERDELETE, line, blank);	
		::SendMessageA(window, SCI_REPLACETARGET, 0, (LPARAM)"");
		return lines;
	}else{
		::SendMessageA(window, SCI_MARKERDELETE, line, blank);	
		return 0;
	}

	//::SendMessage(window,SCI_TARGETFROMSELECTION,0,0);
	//::SendMessage(window, SCI_REPLACETARGET, 0, (LPARAM)"");	
}

blankLineList *removeEmptyLines(HWND window,bool saveList)
{
	::SendMessageA(window, SCI_SETUNDOCOLLECTION, FALSE, 0);
	blankLineList *list=NULL;	
	//int curPosBeg = ::SendMessage(window, SCI_GETSELECTIONSTART, 0, 0);
	//int curPosEnd = ::SendMessage(window, SCI_GETSELECTIONEND, 0, 0);
	double marker=pow(2.0,blank);
	//int line=SendMessageA(window, SCI_MARKERNEXT, 0, (LPARAM)marker);	
    int line=SendMessageA(window, SCI_MARKERNEXT, 0, marker);	
	while(line!=-1){
		int lines=deleteLine(window,line);
		if(lines>0&&saveList){
			//add to list
			blankLineList *newLine=new blankLineList;
			newLine->next=list;
			newLine->line=line;
			newLine->length=lines;
			list=newLine;
		}
		//line=SendMessageA(window, SCI_MARKERNEXT, 0, marker);	
        line=SendMessageA(window, SCI_MARKERNEXT, 0, (LPARAM)marker);	
	}

	//::SendMessage(window, SCI_SETSEL, curPosBeg, curPosEnd);
	::SendMessageA(window, SCI_SETUNDOCOLLECTION, TRUE, 0);
	return list;
}

void clearUndoBuffer(HWND window){
	int modified=::SendMessageA(window, SCI_GETMODIFY, 0, (LPARAM)0);	
		::SendMessageA(window, SCI_EMPTYUNDOBUFFER, 0, (LPARAM)0);
		if(modified){
			::SendMessageA(window, SCI_BEGINUNDOACTION, 0, (LPARAM)0);
			char fake[2];
			fake[1]=0;
			fake[0]=::SendMessageA(window, SCI_GETCHARAT, 0, (LPARAM)0);
			::SendMessageA(window, SCI_SETTARGETSTART, 0, 0);	
			::SendMessageA(window, SCI_SETTARGETEND, 1, 0);
			::SendMessageA(window, SCI_REPLACETARGET, 1, (LPARAM)fake);

			::SendMessageA(window, SCI_ENDUNDOACTION, 0, (LPARAM)0);
		}
}

void clearWindow(HWND window,bool clearUndo)
    {
	//int pos=SendMessageA(window, SCI_MARKERDELETEALL, changed, (LPARAM)changed);	
	
	clearUndo=(removeEmptyLines(window,false)!=NULL);

    ::SendMessageA(window, SCI_MARKERDELETEALL, changed, (LPARAM)changed);	
	::SendMessageA(window, SCI_MARKERDELETEALL, added, (LPARAM)added);
	::SendMessageA(window, SCI_MARKERDELETEALL, removed, (LPARAM)removed);
	::SendMessageA(window, SCI_MARKERDELETEALL, moved, (LPARAM)moved);
	::SendMessageA(window, SCI_MARKERDELETEALL, blank, (LPARAM)blank);
	::SendMessageA(window, SCI_MARKERDELETEALL, changedSymbol, (LPARAM)changedSymbol);		
	::SendMessageA(window, SCI_MARKERDELETEALL, addedSymbol, (LPARAM)addedSymbol);
	::SendMessageA(window, SCI_MARKERDELETEALL, removedSymbol, (LPARAM)removedSymbol);	

	//very aggressive approach to removing the indicators
	//clear style, than tell Notepad++ to unfold all lines, which forces it to redo the page style
	::SendMessageA(window, SCI_CLEARDOCUMENTSTYLE, 0, (LPARAM)0);	
	::SendMessageA(window, SCI_GRABFOCUS, 0, (LPARAM)0);
	::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_VIEW_TOGGLE_UNFOLDALL,0); 

	//int topLine=SendMessageA(window,SCI_GETFIRSTVISIBLELINE,0,0);
	//int linesOnScreen=SendMessageA(window,SCI_LINESONSCREEN,0,0);
	
	//int curPosBeg = ::SendMessageA(window, SCI_GETSELECTIONSTART, 0, 0);
	//int curPosEnd = ::SendMessageA(window, SCI_GETSELECTIONEND, 0, 0);
	::SendMessageA(window, SCN_UPDATEUI, 0, (LPARAM)0);
	//::SendMessageA(window, SCI_SHOWLINES, 0, (LPARAM)1);

	if(clearUndo)
    {
		clearUndoBuffer(window);
	}
	
	/*if(returnToPos){
		SendMessageA(window,SCI_GOTOLINE,topLine,0);
		SendMessageA(window,SCI_GOTOLINE,topLine+linesOnScreen-1,0);
		
		::SendMessageA(window, SCI_SETSEL, curPosBeg, curPosEnd);
	}*/
	
}

void addBlankLines(HWND window,blankLineList *list){
	::SendMessageA(window, SCI_SETUNDOCOLLECTION, FALSE, 0);
	while(list!=NULL){
		addEmptyLines(window,list->line,list->length);
		list=list->next;
	}
	::SendMessageA(window, SCI_SETUNDOCOLLECTION, TRUE, 0);
}

char *getAllText(HWND window,int *length){
	int docLength=SendMessageA(window, SCI_GETLENGTH, 0, (LPARAM)0);
	char *text = new char[docLength+1];
	SendMessageA(window, SCI_GETTEXT, docLength, (LPARAM)text);
	text[docLength]=0;
	*length=docLength;
	return text;

}

void addEmptyLines(HWND hSci, int offset, int length){
	if(length<=0){return;}
	::SendMessageA(hSci, SCI_SETUNDOCOLLECTION, FALSE, 0);
	int posAdd=0;
	UINT EOLtype = ::SendMessageA(hSci,SCI_GETEOLMODE,0,0);


	if(offset!=0){
		int docLines=SendMessageA(hSci, SCI_GETLINECOUNT, 0, (LPARAM)0);
		posAdd= ::SendMessageA(hSci, SCI_POSITIONFROMLINE, offset-1, 0);
		
		
		posAdd+=::SendMessageA(hSci, SCI_LINELENGTH,offset-1,  (LPARAM)0)-lenEOL[EOLtype];
		if(offset==docLines){
			posAdd=SendMessageA(hSci, SCI_GETLENGTH, 0, (LPARAM)0);
		}
		if(posAdd!=0){
			posAdd--;
		}else{
			posAdd=lenEOL[EOLtype]-1;
		}
	}
	::SendMessageA(hSci, SCI_SETTARGETSTART, posAdd, 0);	
	::SendMessageA(hSci, SCI_SETTARGETEND, posAdd+1, 0);
	int blankLinesLength=lenEOL[EOLtype]*length+1;
	int off=0;
	char *buff=new char[blankLinesLength];


	int marker;
	if(offset==0){
		marker=::SendMessageA(hSci, SCI_MARKERGET, 0, 0);
		::SendMessageA(hSci, SCI_MARKERDELETE, 0, (LPARAM)-1);
		buff[blankLinesLength-1]=SendMessageA(hSci, SCI_GETCHARAT, posAdd, (LPARAM)0);
		off=0;
	}else{
		buff[0]=SendMessageA(hSci, SCI_GETCHARAT, posAdd, (LPARAM)0);
		off=1;
		
	}
	for(int j=0;j<length;j++){
		for(int i=0;i<lenEOL[EOLtype];i++){
			buff[j*lenEOL[EOLtype]+i+off]=strEOL[EOLtype][i];
		}
	}

	::SendMessageA(hSci, SCI_REPLACETARGET, blankLinesLength, (LPARAM)buff);

	for (int i = 0; i < length; i++){
			markAsBlank(hSci,offset+i);
	}
	if(offset==0){
		SendMessageA(hSci, SCI_MARKERADDSET, length, marker);
		
	}
#if CLEANUP
	delete[] buff;
#endif
	::SendMessageA(hSci, SCI_SETUNDOCOLLECTION, TRUE, 0);
}
//
//
//
//
//void addEmptyLines(HWND hSci, int offset, int length){
//	if(length<=0){return;}
//	::SendMessage(hSci, SCI_SETUNDOCOLLECTION, FALSE, 0);
//	int curPosBeg = ::SendMessageA(hSci, SCI_GETSELECTIONSTART, 0, 0);
//	int curPosEnd = ::SendMessageA(hSci, SCI_GETSELECTIONEND, 0, 0);
//	int posAdd=0;
//
//
//	if(offset!=0){
//		int docLines=SendMessage(hSci, SCI_GETLINECOUNT, 0, (LPARAM)0);
//		posAdd= ::SendMessage(hSci, SCI_POSITIONFROMLINE, offset-1, 0);
//		posAdd+=::SendMessage(hSci, SCI_LINELENGTH,offset-1,  (LPARAM)0)-lenEOL[EOLtype];
//		if(offset==docLines){
//			posAdd=SendMessage(hSci, SCI_GETLENGTH, 0, (LPARAM)0);
//		}
//	}
//	//::SendMessage(hSci, SCI_SETTARGETSTART, posAdd, 0);	
//	//::SendMessage(hSci, SCI_SETTARGETEND, posAdd, 0);
//	::SendMessage(hSci, SCI_SETSEL, posAdd, posAdd);
//
//	int marker;
//	if(offset==0){
//		marker=::SendMessage(hSci, SCI_MARKERGET, 0, 0);
//		::SendMessage(hSci, SCI_MARKERDELETE, 0, (LPARAM)-1);
//	}
//
//
//	for (int i = 0; i < length; i++){
//		::SendMessage(hSci, SCI_ADDTEXT, lenEOL[EOLtype], (LPARAM)strEOL[EOLtype]);
//		
//		if(curPosBeg>posAdd){
//			curPosBeg+=lenEOL[EOLtype];
//		}
//		if(curPosEnd>posAdd){
//			curPosEnd+=lenEOL[EOLtype];
//		}
//
//		markAsBlank(hSci,offset+i);
//
//	}
//	if(offset==0){
//		SendMessage(hSci, SCI_MARKERADDSET, length, marker);
//		
//	}
//	//	::SendMessage(hSci, SCI_SETTARGETSTART, posAdd, 0);
//	//int docLength=::SendMessage(window, SCI_GETLINECOUNT, 0, 0);
//	//int length=::SendMessage(window, SCI_LINELENGTH, line, 0);
//	//::SendMessage(hSci, SCI_SETTARGETEND, posAdd+lenEOL[EOLtype]*length+lenEOL[EOLtype], 0);
//	//::SendMessage(hSci, SCI_LINESSPLIT, 1, 0);
//	//::SendMessage(hSci, SCI_STARTSTYLING, posAdd, 255);
//	//::SendMessage(hSci, SCI_SETSTYLING,lenEOL[EOLtype]*length+lenEOL[EOLtype] , blank);
//	
//	::SendMessage(hSci, SCI_SETSEL, curPosBeg, curPosEnd);
//	::SendMessage(hSci, SCI_SETUNDOCOLLECTION, TRUE, 0);
//}

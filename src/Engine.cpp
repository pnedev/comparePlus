#include "Engine.h"

unsigned int getLineFromIndex(unsigned int *arr, int index, void * /*context*/)
{
    return arr[index];
}

int compareLines(unsigned int line1, unsigned int line2, void * /*context*/)
{
    if(line1 == line2) return 0;
    return -1;
}

int checkWords(diff_edit* e,chunk_info* chunk,chunk_info* otherChunk)
{
    Word *word = (Word*)varray_get(chunk->words,e->off);
    int start = word->line;
    word = (Word*)varray_get(chunk->words,e->off+e->len-1);
    int end = word->line;
    assert(start <= end);
    int line2 = chunk->lineMappings[start];
    int len = e->len;
    int off = e->off;

    //if the beginning is not at the start of the line, than its definetely a change;
    if(line2 == -1)
    {
        //if this line is not matched to another line, don't bother checking it
        if(start == end)
        {
            return chunk->changeCount;
        }
        else
        {
            e->len -= chunk->lineEndPos[start] - e->off;
            e->off = chunk->lineEndPos[start];
            start++;
        }
    }
    else if(e->off!=chunk->linePos[start])
    {
        struct diff_change *change = (diff_change*) varray_get(chunk->changes, chunk->changeCount++);
        struct diff_change *change2 = (diff_change*) varray_get(otherChunk->changes, otherChunk->changeCount++);

        change2->line = line2;
        change2->len = 0;
        change2->off = 0;
        change2->matchedLine = chunk->lineStart+start;

        word = (Word*)varray_get(chunk->words,e->off);
        assert(word->line == start);
        change->off = word->pos;
        change->line = start;
        change->matchedLine = otherChunk->lineStart+line2;

        //multiline change or single line change
        if(start != end)
        {
            len = chunk->lineEndPos[start] - e->off;
            assert(len > 0);

            word = (Word*)varray_get(chunk->words,e->off+len-1);
            e->off = chunk->lineEndPos[start];
            e->len -= len;
            assert(word->length > 0);
            assert(word->line == start);
            change->len = (word->pos + word->length) - change->off;
            assert(change->len >= 0);

            start++;
        }
        else
        {
            len = e->len;
            word = (Word*)varray_get(chunk->words,e->off+len-1);
            assert(word->length > 0);
            assert(word->line == change->line);
            change->len = (word->pos + word->length) - change->off;
            assert(change->len >= 0);
            return chunk->changeCount;
        }				
    }

    //if a change spans more than one line, all the middle lines are just inserts or deletes
    while(start != end)
    {
        //potentially a inserted line					
        e->off = chunk->lineEndPos[start];
        e->len -= (chunk->lineEndPos[start]-chunk->linePos[start]);
        start++;
    }

    line2 = chunk->lineMappings[start];

    //if the change does not go to the end of the line than its definetely a change
    if(line2!=-1 && (e->off+e->len)<chunk->lineEndPos[start])
    {
        //todo recheck change because some of the diffs will be previous lines
        struct diff_change *change = (diff_change*) varray_get(chunk->changes, chunk->changeCount++);
        struct diff_change *change2 = (diff_change*) varray_get(otherChunk->changes, otherChunk->changeCount++);
        //offset+=(direction*e->len);

        change2->line = line2;//getLineFromPos(chunk->mappings[e->off],otherChunk->lineEndPos,otherChunk->lineCount,false);				
        change2->matchedLine = chunk->lineStart+start;
        change2->len = 0;
        change2->off = 0;

        word = (Word*)varray_get(chunk->words,e->off);
        assert(word->line == start);
        change->off = word->pos;
        len = e->len;
        word = (Word*)varray_get(chunk->words,e->off+len-1);
        assert(word->length > 0);
        change->len = (word->pos + word->length) - change->off;
        change->line = start;	
        assert(word->line == change->line);
        assert(change->len >= 0);
        change->matchedLine = otherChunk->lineStart+line2;
    }
    e->off = off;
    e->len = len;
    return chunk->changeCount;
}


Word *getWord(varray *words, int index, void * /*context*/)
{
    Word *word = (Word*)varray_get(words, index);
    return word;
}

int compareWord(Word *word1, Word *word2, void * /*context*/)
{
    if(word1->hash == word2->hash) return 0;
    return 1;
}

bool compareWords(diff_edit* e1,diff_edit *e2,char** doc1,char** doc2, bool IncludeSpace)
{
	int i, j;

    chunk_info chunk1;
    chunk1.lineCount = e1->len;
    chunk1.words = varray_new(sizeof(struct Word), NULL);
    chunk1.lineStart = e1->off;
    chunk1.count = getWords(e1, doc1, &chunk1, IncludeSpace);
    chunk1.lineMappings = new int[e1->len];

    for(i = 0; i < e1->len; i++)
    {
        chunk1.lineMappings[i] = -1;
    }

    chunk_info chunk2;
    chunk2.lineCount = e2->len;
    chunk2.words = varray_new(sizeof(struct Word), NULL);
    chunk2.count = getWords(e2, doc2, &chunk2, IncludeSpace);
    chunk2.lineStart = e2->off;
    chunk2.lineMappings = new int[e2->len];

    for(i = 0; i < e2->len; i++)
    {
        chunk2.lineMappings[i] = -1;
    }

    //Compare the two chunks
    int sn;
    struct varray *ses = varray_new(sizeof(struct diff_edit), NULL);

    diff(chunk1.words, 0, chunk1.count, chunk2.words, 0, chunk2.count, (idx_fn)(getWord), (cmp_fn)(compareWord), NULL, 0, ses, &sn, NULL);

    chunk1.changes = varray_new(sizeof(struct diff_change), NULL);
    chunk2.changes = varray_new(sizeof(struct diff_change), NULL);

    int offset = 0;
    int **lineMappings1 = new int*[chunk1.lineCount];

    for(i = 0; i < chunk1.lineCount; i++)
    {
        lineMappings1[i] = new int[chunk2.lineCount];

        for(j = 0; j < chunk2.lineCount; j++)
        {
            lineMappings1[i][j] = 0;
        }
    }

    /// Use the MATCH results to syncronise line numbers
    /// count how many are on each line, than select the line with the most matches
    for (i = 0; i < sn; i++) 
    {
        struct diff_edit *e =(diff_edit*) varray_get(ses, i);

        if(e->op == DIFF_DELETE)
        {			
            offset -= e->len;	
        }
        else if(e->op == DIFF_INSERT)
        {			
            offset += e->len;	
        }
        else
        {
            for(int index = e->off; index < (e->off+e->len); index++)
            {
                Word *word1 = (Word*)varray_get(chunk1.words, index);
                Word *word2 = (Word*)varray_get(chunk2.words, index+offset);
                
                if(word1->type != SPACECHAR)
                {
                    int line1a = word1->line;
                    int line2a = word2->line;
                    lineMappings1[line1a][line2a] += word1->length;
                }
            }
        }
    }

    // go through each line, and select the line with the highest strength
    for(i = 0; i < chunk1.lineCount; i++)
    {
        int line = -1;
        int max = 0;

        for(j = 0; j <chunk2.lineCount; j++)
        {
            if(lineMappings1[i][j] > max && (e2->moves == NULL || e2->moves[j] == -1))
            {
                line = j;
                max = lineMappings1[i][j];
            }			
        }

        //make sure that the line isnt already matched to another line, and that enough of the line is matched to be significant
        int size = strlen(doc1[e1->off + i]);

        if(line != -1 && chunk2.lineMappings[line] == -1 && max > (size/3) && (e1->moves == NULL || e1->moves[i] == -1))
        {
            chunk1.lineMappings[i] = line;
            chunk2.lineMappings[line] = i;
        }
    }

    //find all the differences between the lines
    chunk1.changeCount = 0;
    chunk2.changeCount = 0;

    for (i = 0; i < sn; i++) 
    {
        struct diff_edit *e =(diff_edit*) varray_get(ses, i);
        if(e->op == DIFF_DELETE)
        {
            //Differences for Doc 1
            checkWords(e, &chunk1, &chunk2);
        }
        else if(e->op==DIFF_INSERT)
        {			
            //Differences for Doc2
            checkWords(e, &chunk2, &chunk1);
        }
    }

    e1->changeCount = chunk1.changeCount;
    e1->changes = chunk1.changes;
    e2->changeCount = chunk2.changeCount;
    e2->changes = chunk2.changes;

#if CLEANUP

    for(i = 0; i < chunk1.lineCount; i++)
    {
        delete[] lineMappings1[i];
    }

    delete[] lineMappings1;
    delete[] chunk1.lineMappings;
    delete[] chunk1.lineEndPos;
    delete[] chunk1.linePos;
    delete[] chunk2.lineMappings;
    delete[] chunk2.lineEndPos;
    delete[] chunk2.linePos;

    for(i = chunk1.count; i >= 0; i--)
    {
        //Word *w=(Word*)varray_get(chunk1.words,i);
        //w->text.clear();		
        //w->text="";
        //delete w->text;
        varray_release(chunk1.words, i);
    }

    delete[] chunk1.words;

    for(i = chunk2.count; i >= 0; i--)
    {
        //Word *w=(Word*)varray_get(chunk2.words,i);
        //w->text.clear();
        //w->text="";
        //delete w->text;
        varray_release(chunk2.words, i);
    }

    //varray_deinit(chunk2.words);
    delete[] chunk2.words;

    for(i = sn; i >= 0; i--)
    {
        varray_release(ses,i);
    }

    delete ses;

#endif

    return chunk1.changeCount + chunk2.changeCount > 0;
}

int getWords(diff_edit* e, char** doc, chunk_info *chunk, bool IncludeSpace)
{
    varray *words = chunk->words;
    int wordIndex = 0;
    chunk->lineEndPos = new int[chunk->lineCount];
    chunk->linePos = new int[chunk->lineCount];

    for(int line = 0; line < (e->len); line++)
    {
        string text = string("");
        wordType type = SPACECHAR;
        int len = 0;
        chunk->linePos[line] = wordIndex;
        int i = 0;
        unsigned int hash = 0;

        for(i = 0; doc[line+e->off][i] != 0; i++)
        {
            char l = doc[line+e->off][i];
            wordType newType = getWordType(l);

            if(newType == type)
            {
                text += l;
                len++;
                hash = HASH(hash, l);
            }
            else
            {
                if(len > 0)
                {
                    if(!IncludeSpace || type!=SPACECHAR)
                    {
                        Word *word = (Word*)varray_get(words,wordIndex++);
                        word->length = len;
                        word->line = line;
                        word->pos = i-len;
                        word->type = type;
                        word->hash = hash;
                    }
                }
                type = newType;
                text = l;
                len = 1;
                hash = HASH(0,l);
            }
        }

        if(len > 0)
        {
            if(!IncludeSpace || type != SPACECHAR)
            {
                Word *word = (Word*)varray_get(words, wordIndex++);
                word->length = len;
                word->line = line;
                word->pos = i-len;
                word->type = type;
                word->hash = hash;
            }
        }
        chunk->lineEndPos[line] = wordIndex;
    }
    return wordIndex;
}

wordType getWordType(char letter)
{
    switch(letter)
    {
        case ' ':
        case '\t':
            return SPACECHAR;
        default:
            if((letter >= 'a' && letter <= 'z') || 
               (letter >= 'A' && letter <= 'Z') || 
               (letter >= '0' && letter <= '9'))
            {
                return ALPHANUMCHAR;
            }
            return OTHERCHAR;
            break;
    }
}

// change the blocks of diffs to one diff per line. 
// revert a "Changed" line to a insert or delete line if there are no changes
int setDiffLines(diff_edit *e, diff_edit changes[], int *i, short op, int altLocation)
{
    int index = *i;
    int addedLines = 0;
    
    for(int j = 0; j < (e->len); j++)
    {
        changes[index].set = e->set;
        changes[index].len = 1;
        changes[index].op = e->op;
        changes[index].off = e->off+j;
        changes[index].changes = NULL;
        changes[index].changeCount = 0;
        changes[index].moves = NULL;

        //see if line is already marked as move
        if(e->moves != NULL && e->moves[j] != -1)
        {
            changes[index].op = DIFF_MOVE;
            changes[index].matchedLine = e->moves[j];
            changes[index].altLocation = altLocation;
            addedLines++;
        }
        else
        {
            for(int k = 0; k < e->changeCount; k++)
            {
                struct diff_change *change =(diff_change*) varray_get(e->changes, k);
                
                if(change->line == j)
                {
                    changes[index].matchedLine = change->matchedLine;
                    changes[index].altLocation = altLocation;

                    if(altLocation != change->matchedLine)
                    {
                        int diff = altLocation-change->matchedLine;
                        altLocation = change->matchedLine;

                        for(int i = 1; i <= j; i++)
                        {
                            if(changes[index-i].changes != NULL)
                            {
                                break;
                            }
                            if(op == DIFF_DELETE)
                            {
                                changes[index-i].altLocation = change->matchedLine+diff;
                            }
                            else
                            {
                                changes[index-i].altLocation = change->matchedLine;
                            }
                        }
                        if(op == DIFF_INSERT)
                        {
                            altLocation += diff;
                        }
                    }

                    if(changes[index].changes == NULL)
                    {
                        changes[index].changes = varray_new(sizeof(struct diff_change), NULL);
                    }
                    
                    struct diff_change *newChange =(diff_change*) varray_get(changes[index].changes, changes[index].changeCount++);

                    newChange->len = change->len;
                    newChange->off = change->off;							
                }
            }

            if(changes[index].changes == NULL)
            {
                changes[index].op = op;
                changes[index].altLocation = altLocation;
                addedLines++;
            }
            else
            {
                if(op == DIFF_DELETE)
                {
                    altLocation++;
                }
                else
                {
                    altLocation++;
                }
            }
        }

        index++;
    }
    *i = index;
    return addedLines;
}

//Move algorithm:
//scan for lines that are only in the other document once
//use one-to-one match as an anchor
//scan to see if the lines above and below anchor also match
diff_edit *find_anchor(int line, varray *ses, int sn, unsigned int *doc1, unsigned int *doc2, int *line2)
{
    diff_edit *insert = NULL;
    int matches = 0;

    for(int i = 0; i < sn; i++)
    {
        diff_edit *e = (diff_edit*)varray_get(ses,i);
        
        if(e->op == DIFF_INSERT)
        {
            for(int j = 0; j < e->len; j++)
            {
                if(compareLines(doc1[line], doc2[e->off+j], NULL) == 0)
                {
                    *line2 = j;
                    insert = e;
                    matches++;
                }
            }
        }
    }

    if(matches != 1 || insert->moves[*line2] != -1)
    {
        return NULL;
    }

    matches = 0;

    for(int i = 0; i < sn; i++)
    {
        diff_edit *e = (diff_edit*)varray_get(ses, i);

        if(e->op == DIFF_DELETE)
        {
            for(int j = 0; j < e->len; j++)
            {
                if(compareLines(doc1[line], doc1[e->off+j], NULL) == 0)
                {
                    matches++;
                }
            }
        }
    }

    if(matches != 1)
    {
        return NULL;
    }
    return insert;
}


void find_moves(varray *ses,int sn,unsigned int *doc1, unsigned int *doc2, bool DetectMove)
{
    // Exit immediately if user don't want to find moves
    if(DetectMove == false) return;

    //init moves arrays
    for(int i = 0; i < sn; i++)
    {
        diff_edit *e = (diff_edit*)varray_get(ses, i);
        
        if(e->op != DIFF_MATCH)
        {
            e->moves = new int[e->len];

            for(int j = 0; j < e->len; j++)
            {
                e->moves[j] = -1;
            }
        }
        else
        {
            e->moves = NULL;
        }
    }

    for(int i = 0; i < sn; i++)
    {
        diff_edit *e = (diff_edit*)varray_get(ses, i);

        if(e->op == DIFF_DELETE)
        {
            for(int j = 0; j < e->len; j++)
            {
                if(e->moves[j] == -1)
                {
                    int line2;
                    diff_edit *match = find_anchor(e->off+j, ses, sn, doc1, doc2, &line2);
                    
                    if(match != NULL)
                    {
                        e->moves[j] = match->off+line2;
                        match->moves[line2] = e->off+j;
                        int d1 = j-1;
                        int d2 = line2-1;

                        while(d1 >= 0 && d2 >= 0 && e->moves[d1] == -1 && match->moves[d2] == -1 && 
                            compareLines(doc1[e->off+d1], doc2[match->off+d2], NULL) == 0)
                        {
                            e->moves[d1] = match->off + d2;
                            match->moves[d2] = e->off + d1;
                            d1--;
                            d2--;
                        }

                        d1 = j + 1;
                        d2 = line2 + 1;
                        
                        while(d1 < e->len && d2 < match->len && e->moves[d1] == -1 && match->moves[d2] == -1 && 
                            compareLines(doc1[e->off+d1], doc2[match->off+d2], NULL) == 0)
                        {
                            e->moves[d1] = match->off + d2;
                            match->moves[d2] = e->off + d1;
                            d1++;
                            d2++;
                        }
                    }
                }
            }
        }
    }
}

//algorithm borrowed from WinMerge
//if the line after the delete is the same as the first line of the delete, shift down
//basically cabbat -abb is the same as -bba
//since most languages start with unique lines and end with repetative lines(end,</node>, }, etc)
//we shift the differences down where its possible so the results will be cleaner
void shift_boundries(varray *ses, int sn, unsigned int *doc1, unsigned int *doc2, int doc1Length, int doc2Length)
{
    for (int i = 0; i < sn; i++) 
    {
        struct diff_edit *e =(diff_edit*) varray_get(ses, i);
        struct diff_edit *e2 = NULL;
        struct diff_edit *e3 = NULL;
        int max1 = doc1Length;
        int max2 = doc2Length;
        int end2;
        
        if(e->op != 1)
        {
            for(int j = i+1; j < sn; j++)
            {
                e2 = (diff_edit*) varray_get(ses, j);
                if(e2->op == e->op)
                {
                    max1 = e2->off;
                    max2 = e2->off;
                    break;
                }
            }
        }

        if(e->op == DIFF_DELETE)
        {
            e2 = (diff_edit*) varray_get(ses, i+1);

            //if theres an insert after a delete, theres a potential match, so both blocks
            //need to be moved at the same time
            if(e2->op == DIFF_INSERT)
            {
                max2 = doc2Length;
                for(int j = i+2; j < sn; j++)
                {
                    e3 = (diff_edit*) varray_get(ses, j);

                    if(e2->op == e3->op)
                    {
                        max2 = e3->off;
                        break;
                    }
                }

                end2 = e2->off + e2->len;
                i++;
                int end = e->off + e->len;

                while(end < max1 && end2 < max2 && 
                    compareLines(doc1[e->off], doc1[end], NULL) == 0 && 
                    compareLines(doc2[e2->off], doc2[end2], NULL) == 0)
                {
                    end++;
                    end2++;
                    e->off++;
                    e2->off++;
                }
            }
            else
            {
                int end = e->off+e->len;
                while(end < max1 && compareLines(doc1[e->off], doc1[end], NULL) == 0)
                {
                    end++;
                    e->off++;
                }
            }
        }
        else if(e->op == DIFF_INSERT)
        {
            int end = e->off+e->len;

            while(end < max2 && compareLines(doc2[e->off], doc2[end], NULL) == 0)
            {
                end++;
                e->off++;
            }
        }
    }
}

unsigned int *computeHashes(char** doc, int docLength, bool IncludeSpace)
{
    unsigned int *hashes = new unsigned int[docLength];

    for(int i = 0; i < docLength; i++)
    {
        unsigned int hash = 0;

        for(int j = 0; doc[i][j] != 0; j++)
        {
            if(doc[i][j] == ' ' || doc[i][j] == '\t')
            {
                if(!IncludeSpace)
                {
                    hash = HASH(hash, doc[i][j]);
                }
            }
            else
            {
                hash = HASH(hash, doc[i][j]);
            }
        }
        hashes[i] = hash;
    }
    return hashes;
}

void clearEdits(varray *ses,int sn)
{
    for(int i = sn; i >= 0; i--)
    {
        struct diff_edit *e = (diff_edit*) varray_get(ses, i);
        clearEdit(e);
        varray_release(ses,i);
    }
    delete ses;
}

void clearEdit(diff_edit *e)
{
    if(e->moves != NULL)
    {
        delete[] e->moves;
    }
    if(e->changes != NULL)
    {
        for(int j = e->changeCount; j >= 0; j--)
        {
            varray_release(e->changes, j);
        }
        delete e->changes;
    }
}

void cleanEmptyLines(blankLineList *line)
{
    if(line->next != NULL)
    {
        cleanEmptyLines(line->next);
        delete line->next;
    }
}
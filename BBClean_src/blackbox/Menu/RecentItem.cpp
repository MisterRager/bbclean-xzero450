/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "RecentItem.h"
//===========================================================================
int CreateRecentItemMenu(char *pszFileName, char *pszCommand, char *pszTitle, char *pszIcon, int nKeep, int nSort, bool bBeginEnd){
	static ItemList *KeepItemList;
	static ItemList *SortItemList;
	char szFilePath[MAX_PATH];
	char buf[MAX_PATH];
	ItemList *iln = (ItemList*)m_alloc(sizeof(ItemList));

	// make path (compliant with relative-path)
	make_bb_path(szFilePath, pszFileName);

	// Newer Item
	if (pszIcon && *pszIcon)
		_sprintf(buf, "\t[exec] (%s) {%s} <%s>", pszTitle, pszCommand, pszIcon);
	else
		_sprintf(buf, "\t[exec] (%s) {%s}", pszTitle, pszCommand);
	_strcpy(iln->szItem, buf);
	iln->nFrequency = 1;
	append_node(&KeepItemList, iln);

	int cnt_k = 0; // keep items counter
	int cnt_s = 0; // sort items counter
	if (FILE *fp = fopen(szFilePath, "r")){ // read Recent-menu
		while(fgets(buf, MAX_PATH, fp)){
			if (strstr(buf, "[end]"))
				break;
			else if (strstr(buf, "[exec]")){
				if (buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';
				char *p0 = strchr(buf, '{'); // command
				char *p1 = strchr(buf, '}'); // end of command
				if (p0 && p1){
					UINT nFrequency = 0;
					if (char *p = strrchr(buf, '#')){
						nFrequency = atoi(p+1);
						while(*(p--) == ' '); // delete space of eol
						*p = '\0';
					}
					if (0 != strnicmp(pszCommand, p0+1 , p1 - p0 - 1)){ // ignore duplicated item
						ItemList *il = (ItemList*)m_alloc(sizeof(ItemList));
						_strcpy(il->szItem, buf);
						il->nFrequency = imin(nFrequency, UINT_MAX);
						if (cnt_k++ < nKeep - 1)
							append_node(&KeepItemList, il);
						else if (cnt_s++ < nSort)
							append_node(&SortItemList, il);
						else
							m_free(il);
					}
					else{ // if duplicate, increment freq
						iln->nFrequency += imin(nFrequency, UINT_MAX - 1);
					}
				}
			}
		}
		fclose(fp);
	}

	bool isSortList = listlen(SortItemList) != 0;
	bool isKeepList = listlen(KeepItemList) != 0;

	if (isSortList)
		SortItemList = sortlist(SortItemList);

	if (FILE *fp = fopen(szFilePath, "w")){ // write Recent-menu
		if (bBeginEnd)
			fprintf(fp, "[begin] (Recent Item)\n");

		// most recent item
		ItemList *p;
		dolist (p, KeepItemList){
			fprintf(fp, "%s #%u\n", p->szItem, p->nFrequency);
		}
		if (isKeepList && isSortList)
			fprintf(fp, "\t[separator]\n");
		dolist (p, SortItemList){
			fprintf(fp, "%s #%u\n", p->szItem, p->nFrequency);
		}

		if (bBeginEnd)
			fprintf(fp, "[end]\n");

		fclose(fp);
	}

	freeall(&KeepItemList);
	freeall(&SortItemList);
	return 0;
}
//===========================================================================
ItemList *sortlist(ItemList *il){
	ItemList dmy;
	ItemList *p0,*p1;
	ItemList *p;

	dmy.nFrequency = UINT_MAX;
	dmy.next = il;
	p = il->next;
	il->next = NULL;

	p1 = p;
	while(p1 != NULL){
		p0 = &dmy;
		while(p0->next != NULL){
			if(p1->nFrequency > p0->next->nFrequency){
				p = p1->next;
				p1->next = p0->next;
				p0->next = p1;
				break;
			}
			p0 = p0->next;
		}
		if(p0->next == NULL){
			p = p1->next;
			p0->next = p1;
			p1->next = NULL;
		}
		p1 = p;
	}
	return dmy.next;
}
//===========================================================================

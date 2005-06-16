/*
 *  exPIMP
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa³ Lindemann, Stamina
 *
 *  $Id: import.cpp 98 2005-06-16 16:18:58Z konnekt $
 */

#include "stdafx.h"

#include "../include/dtablebin.h"
#include "../include/time64.h"
#include "../include/func.h"
#include "../include/simxml.h"

#include "base64.h"

#include "konnekt/plug_export.h"
#include "konnekt/ui.h"
#include "konnekt/plug_func.h"
#include "expimp.h"
#include "konnekt/expimp.h"

cColumn * CheckColumn(int tableID , tColumns & Columns , CStdString id , CStdString name) {
	// Okreœlamy ID
	int colID = atoi(id);
	if (id.empty() || colID & DT_COLID_UNIQUE) colID = DT_COLID_UNIQUE;
	// porownujemy z tablicami...
	if (!name.empty()) {
		int _id = Ctrl->DTgetNameID(tableID , name);
		if (_id==-1 
			//|| ((colID & DT_COLID_UNIQUE)!=(_id & DT_COLID_UNIQUE))
			|| (colID!=DT_COLID_UNIQUE && colID != _id)
			) {
			return 0;
		}
		colID = _id;
	} else if (colID == DT_COLID_UNIQUE)
		return 0; // Skoro nie ma ID, musi byæ chocia¿ nazwa...

	if (colID != DT_COLID_UNIQUE && colID != -1 && Ctrl->DTgetPos(tableID , colID)!=-1) {
		cColumn & c = Columns[colID];
		c.id = colID;
		c.name = name;
		return &c;
	} else return 0;

}
struct cReplace {
	CStdString pattern;
	int limit;
};

void DTBImport(CStdString XMLfile) {
	//
	cXML XML;
	if (!XML.loadFile(XMLfile))
		throw "Nie mog³em wczytaæ pliku!";
	IMLOG("- Importujê");
	// Wczytujemy informacje
	// W zasadzie jedynym istotnym parametrem jest rodzaj tabeli.
	CStdString tmp = XML.getText("data/info/table");
	if (tmp.empty())
		throw "Importowane mog¹ byæ tylko tablice ustawieñ i kontaktów!";
	int tableID = atoi(tmp);
	if (tableID != DTCNT && tableID != DTCFG)
		throw "Importowane mog¹ byæ tylko tablice ustawieñ i kontaktów!";
	// wczytujemy deskryptor
	CStdString info = "Import ";
	info+= tableID==DTCNT ? "kontaktów" : "konfiguracji";
	info+="\r\n  Wygenerowany przez: " + XML.getText("data/info/owner");
	info+="\r\n  Dnia: " + XML.getText("data/info/date");
	info+="\r\n  " + XML.getText("data/info/comment");
	info+="\r\nZaimportowaæ?";
	if (!ICMessage(IMI_CONFIRM , (int)info.c_str())) return;
	IMLOG("- Wczytujê deskryptor");
	tColumns Columns;
	{
		cXML XML2;
		XML2.loadSource(XML.getContent("data/descriptor").c_str());
		CStdString rejected;
		while (XML2.prepareNode("col" , true)) {
			cColumn * col = CheckColumn(tableID , Columns , XML2.getAttrib("id") , XML2.getAttrib("name"));
			if (!col || !col->use) goto col_rejected;
			col->type = atoi(XML2.getAttrib("type").c_str());
			col->key = atoi(XML2.getAttrib("key").c_str());
			col->descriptive = atoi(XML2.getAttrib("desc").c_str());
			col->required = atoi(XML2.getAttrib("required").c_str());
			col->encoded = atoi(XML2.getAttrib("encoded").c_str());

			if ((Ctrl->DTgetType(tableID , col->id) & DT_CT_TYPEMASK) != (col->type & DT_CT_TYPEMASK))
				goto col_rejected; // Musz¹ zgadzaæ siê typy!
			goto col_ok;
			col_rejected:
			if (col) {
				if (!rejected.empty()) rejected+=", ";
				if (col->name.empty()) {
					char val [32];
					itoa(col->id , val , 10);
					rejected+=val;
				} else
					rejected+=col->name;
				col->use = false;
			}
			col_ok:
			XML2.next();
		}
		if (!rejected.empty()) {
			rejected = "Poni¿sze kolumny nie zostan¹ wczytane: \r\n" 
				+ rejected
				+ "\r\n - zmieni³eœ coœ w xml'u,\r\n - masz w³¹czone nie wszystkie wtyczki\r\n - wtyczki s¹ w innych wersjach.\r\n\r\nKontynuowaæ?"
				;
			if (!ICMessage(IMI_CONFIRM , (int) rejected.c_str())) return;
		}
	}// deskryptor
	// Skoro deskryptor jest za³atwiony mo¿emy zabraæ siê za wczytywanie poszczególnych linijek...
	IMLOG("- Wczyta³em %d kolumn" , Columns.size());
	IMLOG("- Wczytujê dane");
	XML.loadSource(XML.getContent("data/rows").c_str()); // nic wiêcej nie bêdzie ju¿ potrzebne...
	cXML XMLr;
	cPreg replacer (true);

	while (XML.prepareNode("row" , true)) {
		int rowID = atoi(XML.getAttrib("id").c_str());
		XMLr.loadSource(XML.getContent().c_str());
		// Wczytujemy wszystkie komórki...
		map <int , CStdString> cells;
		map <int , cReplace> regexp;
		while (XMLr.prepareNode("cell" , true)) {
			cColumn * col = CheckColumn(tableID , Columns , XMLr.getAttrib("id") , XMLr.getAttrib("name"));
			if (col && col->use) {
				cells[col->id]=XMLr.getText();
				if (!XMLr.getAttrib("encoded").empty()) {
					CStdString dec;
					unsigned int length = cells[col->id].size();
					decode64(cells[col->id], cells[col->id].size() , dec.GetBuffer(length) , &length);
					dec.ReleaseBuffer(length);
					cells[col->id] = dec;
				}
				if (!XMLr.getAttrib("replace").empty()) {
					cReplace re;
					re.pattern = XMLr.getAttrib("replace");
					re.limit = atoi(XMLr.getAttrib("limit").c_str());
					regexp[col->id] = re;
				}
			}
			XMLr.next();
		}
		if (tableID == DTCFG)
			rowID = 0;
		else { // sprawdzamy któr¹ linijkê w³aœnie czytamy...
			// S¹ max. trzy poziomy kluczy...
			// -1 oznacza, ¿e któryœ nie trafi³, a 0 ¿e ¿aden nie zosta³ sprawdzony
			rowID = -1;
			int key [3] = {0,0,0};
			bool keyValid [3];
/*			if (tableID == DTCNT 
				&& cells.find(CNT_NET)!=cells.end()
				&& cells[CNT_NET]=="-1") {
					rowID = 0;
					goto store_data;
				}*/
			// Przeszukujemy ca³¹ tablicê...
			for (int i=0; i<Ctrl->DTgetCount(tableID); i++) {
				keyValid[0]=keyValid[1]=keyValid[2]=true;

				// Szukamy kluczy
				int id = Ctrl->DTgetID(tableID , i);
				for (tColumns::iterator ci = Columns.begin(); ci != Columns.end(); ci++) {
					cColumn & col = ci->second;
					if (col.key>0 && col.key<=3 && (key[col.key-1]==id || key[col.key-1]==0)) {
						CStdString value = Ctrl->DTgetStr(tableID , id , col.id);
						if (cells.find(col.id) == cells.end() || cells[col.id]=="" || cells[col.id]=="0" || value=="" || value=="0")
							keyValid[col.key-1] = false;
						key[col.key-1] = (( keyValid[col.key-1] && cells[col.id] == value)?id:-1);
					}
				}
				// zerujemy stany kluczy...
				for (int j=0;j<sizeof(key)/4;j++) if (key[j]==-1 || (j>0 && keyValid[j-1])) key[j]=0;
			}
			for (int j=0;j<sizeof(key)/4;j++) 
				if (key[j]) {rowID = key[j];break;} // Uda³o siê!
			if (rowID == -1) { // Musimy dodaæ
				if (tableID == DTCNT) {
					rowID = ICMessage(IMC_CNT_ADD , 0 , 0);
				}
			}
		} // != DTCFG
store_data:
		if (rowID != -1 && Ctrl->DTgetPos(tableID , rowID)!=-1) {
			for (map <int , CStdString>::iterator cell=cells.begin(); cell!=cells.end(); cell++) {
				// wykonujemy zleconego replace'a
				if (regexp.find(cell->first) != regexp.end()) {
					cReplace & re = regexp[cell->first];
					cell->second = replacer.replace(re.pattern , cell->second , Ctrl->DTgetStr(tableID , rowID , cell->first) , re.limit);
				}
				Ctrl->DTsetStr(tableID , rowID , cell->first , (char*)cell->second.c_str());
			}
		}
		if (tableID == DTCNT) ICMessage(IMC_CNT_CHANGED , rowID);
		XML.next();
	}
	if (tableID == DTCFG) ICMessage(IMC_CFG_CHANGED);

	if (tableID == DTCFG)
		ICMessage(IMC_SAVE_CFG);
	else if (tableID == DTCFG)
		ICMessage(IMC_SAVE_CNT);

	IMLOG("- Gotowe");

}

void DoImport() {
	CStdString fileName = "";
	try {
		OPENFILENAME of;
		memset(&of , 0 , sizeof(of));
		CStdString title = "Importowanie danych";
		of.lpstrTitle = title;
		of.hwndOwner = (HWND)ICMessage(IMI_GROUP_GETHANDLE , (int)&sUIAction(0 , IMIG_MAINWND));
		of.lStructSize=sizeof(of);
		of.lpstrFilter="Plik ExPImP\0*.expimp\0Plik XML\0*.xml\0";
		of.lpstrFile=fileName.GetBuffer(MAX_PATH);
		of.nMaxFile=MAX_PATH;
	//                          of.nFilterIndex=1;
		of.Flags=OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
		of.lpstrDefExt="";

		sMRU mru;
		CStdString mru_result;
		mru.name = "exPiMP_dir";
		mru.flags = MRU_GET_USETEMP;
		mru.buffer = 0;//mru_result.GetBuffer(255);
		mru.buffSize = 255;
		mru.count = 1;
		Ctrl->IMessage(&sIMessage_MRU(IMC_MRU_GET , &mru));
		//mru_result.ReleaseBuffer();
		mru_result = mru.values[0];
		of.lpstrInitialDir = mru_result;

		if (!GetOpenFileName(&of)) {
			/*IMLOG("GetOpenFileName error Code = %x" , CommDlgExtendedError());
			throw "GetOpenFileName zwraca 0";*/
		} else {
			CStdString dir = of.lpstrFile;
			if (dir.find('\\')!=-1) dir.erase(dir.find_last_of('\\'));
			mru.current = dir;
			Ctrl->IMessage(&sIMessage_MRU(IMC_MRU_SET , &mru));

			ICMessage(IMC_RESTORECURDIR);
			DTBImport(fileName);
		}
	} catch (char*e) {
		CStdString msg = "Wyst¹pi³ b³¹d podczas importowania:\r\n\r\n";
		msg+=e;
		ICMessage(IMI_ERROR , (int)msg.c_str());
	}
}

void DoImport(const CStdString fileName) {
	ICMessage(IMC_RESTORECURDIR);
	try {
		DTBImport(fileName);
	} catch (char*e) {
		CStdString msg = "Wyst¹pi³ b³¹d podczas importowania ";
		msg+=fileName;
		msg+=":\r\n\r\n";
		msg+=e;
		ICMessage(IMI_ERROR , (int)msg.c_str());
	}

}

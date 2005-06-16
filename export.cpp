/*
 *  exPIMP
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa³ Lindemann, Stamina
 *
 *  $Id: export.cpp 98 2005-06-16 16:18:58Z konnekt $
 */

#include "stdafx.h"

#include "../include/dtablebin.h"
#include "../include/time64.h"
#include "../include/simxml.h"

#include "base64.h"

#include "konnekt/plug_export.h"
#include "konnekt/ui.h"
#include "konnekt/plug_func.h"
#include "expimp.h"
#include "konnekt/expimp.h"




void DTBExport(int tableID , CStdString DTBfile , CStdString XMLfile , tAllowIDs & allow) {
//	if (!tableID) throw "";
	ofstream xml(XMLfile);
	IMLOG("- Export");
	if (!xml.is_open()) throw "Plik nie móg³ zostaæ utworzony!";
	CdTable table;
	CdtFileBin fb;
	if (tableID != DTNONE) {
		table.cxor_key = XOR_KEY;
		Ctrl->DTlock(tableID , -1);
	}
	if (tableID == DTCFG)
		ICMessage(IMC_SAVE_CFG);
	else if (tableID == DTCFG)
		ICMessage(IMC_SAVE_CNT);
	// Wczytujemy deskryptor
	IMLOG("- Wczytujê deskryptor z dtb");
	fb.assign(&table);
//	fb.open(XMLfile , DT_READ);
//	fb.freaddesc();
	fb.loadAll(DTBfile);
	fb.close();
	// Tworzymy listê kolumn do wyexportowania...
	tColumns Columns;
	for (CdtColDesc::cols_it_t i=table.cols.cols.begin(); i!=table.cols.cols.end();i++) {
		CdtColDescItem & c = (*i);
		if (c.type & DT_CF_NOSAVE) continue;
		cColumn & Col = Columns[c.id];
		Col.type = c.type;
		Col.name = c.name;
		Col.name.Replace("\"" , "&quot;");
		if (c.type & DT_CF_SECRET)
			Col.encoded = 1;
	}
	// Zmieniamy kolumny dla wybranych tabel
	if (tableID == DTCNT) {
		Columns[CNT_NET].key = 1;
		Columns[CNT_UID].key = 1;
		Columns[CNT_DISPLAY].key = 2;
		Columns[CNT_DISPLAY].descriptive = 1;
		Columns[CNT_UID].descriptive = 1;
	}
	// Zapisujemy dane..
	IMLOG("- Zapisujê deskryptor");
	xml << "<?xml version=\"1.0\" encoding=\"Windows-1250\"?>" << endl;
	xml << "<!-- Plik wygenerowany automagicznie przez exPiMP. Najlepiej za du¿o w nim nie mieszaæ... -->" << endl;
	xml << "<data>" << endl;
	xml << "	<info>" << endl;
	xml << "		<date>" << cTime64(true).strftime("%d-%m-%y %H:%M:%S") << "</date>" << endl;
	xml << "		<file>" << DTBfile << "</file>" << endl;
	if (tableID != DTNONE) xml << "		<table>" << tableID << "</table>" << endl;
	if (allow.size())
		xml << "		<chosenData>1</chosenData>" << endl;
	xml << "		<owner>" << (char*)ICMessage(IMC_GETPROFILE) << "</owner>" << endl;
	{
		char v [60];
		ICMessage(IMC_PLUG_VERSION , ICMessage(IMC_PLUGID_POS , Ctrl->ID()) , (int)v);
		xml << "		<version exPiMP=\"" << v;
		ICMessage(IMC_PLUG_VERSION , -1 , (int)v);
		xml <<"\" konnekt=\""<< v << "\" />" << endl;
	}
	xml << "	</info>" << endl;
	// Zapisujemy "deskryptor", ¿eby póŸniej sprawdziæ, czy typy siê zgadzaj¹...
	xml << "	<descriptor count=\""<< Columns.size() <<"\">" << endl;
	for (tColumns::iterator c = Columns.begin(); c!=Columns.end(); c++) {
		xml << "		<col";
		cColumn & col = c->second;
		if (!(c->first & DT_COLID_UNIQUE)) xml << " id=\"" << c->first << "\"";
		if (!col.name.empty()) xml << " name=\"" << col.name << "\"";
		xml << " type=\"" << (col.type & (~DT_CF_LOADED)) << "\"";
		if (col.descriptive) xml << " desc=\"" << (int)col.descriptive << "\"";
		if (col.encoded) xml << " encoded=\"" << (int)col.encoded << "\"";
		if (col.key) xml << " key=\"" << (int)col.key << "\"";
		if (col.required) xml << " required=\"" << (int)col.required << "\"";
		xml << "/>" << endl;
	}
	xml << "	</descriptor>" << endl;
	// A teraz ju¿ linijka po linijce...
	IMLOG("- Zapisujê dane");
	xml << "	<rows count=\""<< table.getrowcount() <<"\">" << endl;
	for (unsigned int i=0; i<table.getrowcount(); i++) {
		if (table.rows[i]->flag & DT_RF_DONTSAVE) continue;
		if (allow.size() && find(allow.begin() , allow.end() , table.getrowid(i)) == allow.end()) continue;
		xml << "		<row id=\""<< table.getrowid(i) << "\">" << endl;
		for (tColumns::iterator c = Columns.begin(); c!=Columns.end(); c++) {
			cColumn & col = c->second;
			xml << "			<cell ";
			if (col.name.empty())
				xml << "id=\"" << c->first << "\"";
			else
				xml << "name=\"" << col.name << "\"";
			if (col.encoded)
				xml << " encoded=\"" << (int)col.encoded << "\"";
			xml << ">";
			Tables::Value value;
			value.type = DT_CT_PCHAR;
			char buff[32];
			value.vChar = buff;
			value.buffSize = 0;
			table.getValue(i , c->first , &value);
			CStdString v = value.vChar;
			if (col.encoded) {
				CStdString enc;
				size_t enc_size = v.size()+64;
				encode64(v , v.size() , enc.GetBuffer(enc_size) , enc_size , &enc_size);
				enc.ReleaseBuffer(enc_size);
				v = enc;
			}
			xml << EncodeEntities(v);
			xml << "</cell>" << endl;
		}	
		xml << "		</row>" << endl;
	}
	xml << "	</rows>" << endl;
	
	xml << "</data>" << endl;
	xml.close();
	if (tableID != DTNONE)
		Ctrl->DTunlock(tableID , -1);
	IMLOG("- Gotowe");
	
}

void DoExport(int tableID , tAllowIDs & allow , CStdString file) {
	CStdString fileName = "konnekt";
	CStdString profile = (char*)ICMessage(IMC_GETPROFILE);
	try {
		if (file.empty()) {
			file = (char*)ICMessage(IMC_PROFILEDIR);
			if (tableID == DTCNT) {file+="cnt.dtb"; fileName="konnekt_"+profile+"_kontakty";}
			else if (tableID == DTCFG) {file+="cfg.dtb";fileName="konnekt_"+profile+"_ustawienia";}
			else throw "Nieznany typ danych.";
		}
		OPENFILENAME of;
		memset(&of , 0 , sizeof(of));
		CStdString title = "Exportowanie "+file;
		of.lpstrTitle = title;
//		of.lpstrInitialDir = (char*)ICMessage(IMC_RESTORECURDIR);
		of.hwndOwner = (HWND)ICMessage(IMI_GROUP_GETHANDLE , (int)&sUIAction(0 , IMIG_MAINWND));
		of.lStructSize=sizeof(of);
		of.lpstrFilter="Plik ExPImP\0*.expimp\0*.xml\0*.xml\0";
		of.lpstrFile=fileName.GetBuffer(MAX_PATH);
		of.nMaxFile=MAX_PATH;
	//                          of.nFilterIndex=1;
		of.Flags=OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
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

		if (!GetSaveFileName(&of)) {
			/*IMLOG("GetSaveFileName error Code = %x" , CommDlgExtendedError());
			throw "GetSaveFileName zwraca 0";*/}
		else {
			CStdString dir = of.lpstrFile;
			if (dir.find('\\')!=-1) dir.erase(dir.find_last_of('\\'));
			mru.current = dir;
			Ctrl->IMessage(&sIMessage_MRU(IMC_MRU_SET , &mru));

			ICMessage(IMC_RESTORECURDIR);
			DTBExport(tableID , file , fileName , allow);
		}
	} catch (char*e) {
		CStdString msg = "Wyst¹pi³ b³¹d podczas exportowania:\r\n\r\n";
		msg+=e;
		ICMessage(IMI_ERROR , (int)msg.c_str());
	}
}

void DoExport(int tableID , CStdString file) {
	tAllowIDs ids;
	DoExport(tableID , ids , file);
}


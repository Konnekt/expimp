/*
 *  exPIMP
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa³ Lindemann, Stamina
 *
 *  $Id: expimp.h 98 2005-06-16 16:18:58Z konnekt $
 */

#define XOR_KEY "\x16\x48\xf0\x85\xa9\x12\x03\x98\xbe\xcf\x42\x08\x76\xa5\x22\x84"  // NIE ZMIENIAÆ!!!!!!!

struct cColumn {
	CStdString name;
	int type;
	bool descriptive;
	char key;
	char required;
	char encoded;
	bool use;
	int id;
	cColumn() {
		descriptive = false;
		key = required = encoded = 0;
		use = true;
		id = 0;
	}
};

typedef map<int , cColumn> tColumns;
typedef list<int> tAllowIDs;


void DoExport(int tableID , CStdString file="");
void DoExport(int tableID , tAllowIDs & allow , CStdString file="");
void DoImport();
void DoImport(const CStdString fileName);

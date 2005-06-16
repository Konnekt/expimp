/*
 *  exPIMP
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa³ Lindemann, Stamina
 *
 *  $Id: exPIMP.cpp 98 2005-06-16 16:18:58Z konnekt $
 */

#include "stdafx.h"


#include "konnekt/plug_export.h"
#include "konnekt/ui.h"
#include "konnekt/plug_func.h"
#include "konnekt/obsolete.h"
#include "plug_defs/lib.h"

#include "expimp.h"
#include "konnekt/expimp.h"
#include "resource.h"

int __stdcall DllMain(void * hinstDLL, unsigned long fdwReason, void * lpvReserved)
{
        return true;
}

int Init() {
  return 1;
}

int DeInit() {
  return 1;
}

int IStart() {
  return 1;
}
int IEnd() {
  return 1;
}

#define CFGSETCOL(i,t,d) {sSETCOL sc;sc.id=(i);sc.type=(t);sc.def=(int)(d);ICMessage(IMC_CFG_SETCOL,(int)&sc);}
int ISetCols() {
  return 1;
}

int IPrepare() {
	IconRegister(IML_16 , IDI_EXPIMP_LOGO , Ctrl->hDll() , IDI_EXPIMP_LOGO);
	IconRegister(IML_16 , IDI_EXPIMP_EXPORT , Ctrl->hDll() , IDI_EXPIMP_EXPORT);
	IconRegister(IML_16 , IDI_EXPIMP_IMPORT , Ctrl->hDll() , IDI_EXPIMP_IMPORT);
	UIGroupAdd(IMIG_MAIN_OPTIONS , IMIG_MAIN_CFG_EXPIMP , 0 , "exPiMP" , IDI_EXPIMP_LOGO);
	UIActionAdd(IMIG_MAIN_CFG_EXPIMP , IMIA_MAIN_CFG_EXPIMP_IMPORT , 0 , "Importuj" , IDI_EXPIMP_IMPORT);
	UIActionAdd(IMIG_MAIN_CFG_EXPIMP , IMIA_MAIN_CFG_EXPIMP_ECFG , 0 , "Exportuj ustawienia" , IDI_EXPIMP_EXPORT);
	UIActionAdd(IMIG_MAIN_CFG_EXPIMP , IMIA_MAIN_CFG_EXPIMP_ECNT , 0 , "Exportuj kontakty" , IDI_EXPIMP_EXPORT);
	UIActionAdd(IMIG_MAIN_CFG_EXPIMP , IMIA_MAIN_CFG_EXPIMP_ECNTSEL , 0 , "Exportuj kontakty zaznaczone" , IDI_EXPIMP_EXPORT);
	return 1;
}

int ActionCfgProc(sUIActionNotify_base * anBase) {
  sUIActionNotify_2params * an = (anBase->s_size>=sizeof(sUIActionNotify_2params))?static_cast<sUIActionNotify_2params*>(anBase):0;
  switch (anBase->act.id & ~IMIB_CFG) {
    
  }
  return 0;
}

ActionProc(sUIActionNotify_base * anBase) {
  sUIActionNotify_2params * an = (anBase->s_size>=sizeof(sUIActionNotify_2params))?static_cast<sUIActionNotify_2params*>(anBase):0;

  if ((anBase->act.id & IMIB_) == IMIB_CFG) return ActionCfgProc(anBase); 
  switch (anBase->act.id) {
	  case IMIA_MAIN_CFG_EXPIMP_ECFG: 
		  ACTIONONLY(anBase);
		  DoExport(DTCFG);
		  break;
	  case IMIA_MAIN_CFG_EXPIMP_ECNT: 
		  ACTIONONLY(anBase);
		  DoExport(DTCNT);
		  break;
	  case IMIA_MAIN_CFG_EXPIMP_ECNTSEL: {
		  ACTIONONLY(anBase);
		  int c = ICMessage(IMI_LST_SELCOUNT);
		  tAllowIDs allow;
		  for (int i=0; i<c; i++) {
			  int id = ICMessage(IMI_LST_GETSELPOS , i);
			  allow.push_back(id);
		  }
		  DoExport(DTCNT , allow);
		  break;}
	  case IMIA_MAIN_CFG_EXPIMP_IMPORT: 
		  ACTIONONLY(anBase);
		  DoImport();
		  break;
    
  }
  return 0;
}



int __stdcall IMessageProc(sIMessage_base * msgBase) {
    sIMessage_2params * msg = (msgBase->s_size>=sizeof(sIMessage_2params))?static_cast<sIMessage_2params*>(msgBase):0;
    switch (msgBase->id) {
    /* Wiadomoœci na które TRZEBA odpowiedzieæ */
    case IM_PLUG_NET:        return NET_EXPIMP; // Zwracamy wartoœæ NET, która MUSI byæ ró¿na od 0 i UNIKATOWA!
	case IM_PLUG_TYPE:       return IMT_UI|IMT_CONFIG; // Zwracamy jakiego typu jest nasza wtyczka (które wiadomoœci bêdziemy obs³ugiwaæ)
    case IM_PLUG_VERSION:    return 0; // Wersja wtyczki tekstowo major.minor.release.build ...
    case IM_PLUG_SDKVERSION: return KONNEKT_SDK_V;  // Ta linijka jest wymagana!
    case IM_PLUG_SIG:        return (int)"EXPIMP"; // Sygnaturka wtyczki (krótka, kilkuliterowa nazwa)
    case IM_PLUG_CORE_V:     return (int)"W98"; // Wymagana wersja rdzenia
    case IM_PLUG_UI_V:       return 0; // Wymagana wersja UI
    case IM_PLUG_NAME:       return (int)"exPiMP"; // Pe³na nazwa wtyczki
    case IM_PLUG_NETNAME:    return 0; // Nazwa obs³ugiwanej sieci (o ile jak¹œ sieæ obs³uguje)
    case IM_PLUG_INIT:       Plug_Init(msg->p1,msg->p2);return Init();
    case IM_PLUG_DEINIT:     Plug_Deinit(msg->p1,msg->p2);return DeInit();

    case IM_SETCOLS:     return ISetCols();

    case IM_UI_PREPARE:      return IPrepare();
    case IM_START:           return IStart();
    case IM_END:             return IEnd();

    case IM_UIACTION:        return ActionProc((sUIActionNotify_base*)msg->p1);

	case IM_PLUG_ARGS: {
		sIMessage_plugArgs * pa = static_cast<sIMessage_plugArgs*> (msgBase);
		if (pa->argc < 2) return 0;
		const char * import_ = getArgV(pa->argv+1 , pa->argc-1 , "-import" , true);
		if (import_ && *import_) {
			CStdString import = import_;
			if (import.find("expimp:")==0) {
				import = import.substr(import.find(':') + 1);
			}
			DoImport(import);		
		}
		return 0;}

    /* Tutaj obs³ugujemy wszystkie pozosta³e wiadomoœci */

    default:
        if (Ctrl) Ctrl->setError(IMERROR_NORESULT);
        return 0;

 }
 return 0;
}


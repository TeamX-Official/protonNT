
/************** F R O M  C P C D O S   W R A P P E R **************/

// ---- Buffer / context from Cpcdos ---- //
extern long					cpc_Creer_Contexte			(long TAILLEX, long TAILLEYn);
extern void*				cpc_Obtenir_Zone_Contexte	(long ID);
extern long					cpc_Blitter					(long ID);

// ---- User input from cpcdos ---- //
extern long					cpc_mouse_state_lck			(long param, long id_context); /* 0:Clic state (1:left 2:right 3:middle) | 1:Position X | 2:Position Y | 3:Scrool wheel */
extern long					cpc_set_mouse_lck			(long px, long py, bool visible, long id_context);
extern long					cpc_get_key_lck				(long id_context);




/************** F R O M   C P I N T I   C O R E   W R A P P E R **************/

// ---- Create thread ---- //
extern unsigned long 		xe_cpinti_creer_thread		(void* (* Fonction) (void * arg), unsigned long stackSize, void* threadParam, bool ExecuteThreadNow);

// ---- Doevent (Calling CPinti Core scheduler) ---- //
extern void 				xe_cpinti_doevents			(unsigned long time_ms);

// ---- Getting threads info ---- //
extern unsigned long 		xe_cpinti_get_ThreadEnCours	();

// ---- Getting process infos ---- //
extern unsigned long 		xe_cpinti_get_pid_from_tid	();
// extern unsigned long 		xe_cpinti_get_pid_from_tid	(unsigned long time_ms);


// ---- Critical section  ---- //
extern void 				xe_cpinti_ENTRER_SectionCritique();
extern void 				xe_cpinti_SORTIR_SectionCritique();


/**********************************************************************/



int UserInputUpdate(uint32_t h, long id_context);

uint32_t Create_context(ContextInf _ctx)
{
	aContext_count++; //Important: Skip the zero index (NULL)
	int idx = aContext_count;
	
	aContext[idx] = _ctx;
	
	
	if(aContext[idx].width > 10)
	{
		// Get ID context from cpcdos
		aContext[idx].id_context = cpc_Creer_Contexte((long) aContext[idx].width, (long)aContext[idx].height);
		showinf("Create_Context()= idx: %d, id.context: %zu, height: %d, width: %d", idx,  aContext[idx].id_context, aContext[idx].height,  aContext[idx].width);
	}
	else
	{
		aContext[idx].id_context = cpc_Creer_Contexte((long) 1, (long)1);
		showinf("Create_Context()= (FIXED) idx: %d, id.context: %zu, height: %d, width: %d", idx,  aContext[idx].id_context, 1,  1);
	}

	showinf("PixView= idx: %d,  height: %d, width: %d", idx, aContext[idx].height,  aContext[idx].width);
	showinf("create hwnd_View( hwnd_View: %d, idx: %d, height: %d, width: %d )", aContext[idx].hwnd_View,  idx,  aContext[idx].height,  aContext[idx].width );
	return (uint32_t)idx;
}

ContextInf* Blit_context(uint32_t _idx, const void* _pixels, int _SrcWidth)
{
	// Updating mouse state. id_context for identifing window handle for SCI cpcdos service
	UserInputUpdate(_idx, (long) aContext[1].id_context);

	// If witdh > 10 px
	if(aContext[_idx].width > 10)
	{

		// Getting pixels context
		aContext[_idx].pixels = cpc_Obtenir_Zone_Contexte((long) aContext[1].id_context);

		// Getting ptr source & destination
		uint32_t* pix_src = (uint32_t*)_pixels;
		uint32_t* pix_dest = (uint32_t*)aContext[_idx].pixels;
		
		// Write to ptr video buffer
		for(int y = 0; y <  aContext[_idx].height; y++){
			memcpy(pix_dest + (y * aContext[_idx].width), pix_src + (y * _SrcWidth), aContext[_idx].width *4);
		}

		_printl("hndl:%zu - Blit_context(%l) [%dx%d]", aContext[1].id_context, aContext[_idx].height, aContext[_idx].width);

		// Blit fram buffer to cpcdos SCI
		cpc_Blitter((long) aContext[_idx].id_context);

	}	

	return &aContext[_idx]; //number of scan lines copied
}

long previous_keydown[12] = {0};
int UserInputUpdate(uint32_t h, long id_context)
{
	// cpc_mouse_state_lck() --> 0:Clic state (1:left 2:right 3:middle) | 1:Position X | 2:Position Y | 3:Scrool wheel

	// Getting clic state
	long clic = (long) cpc_mouse_state_lck(0, id_context);

	// Getting mouse position (relative to window with id)
	long posx = (long) cpc_mouse_state_lck(1, id_context);
	long posy = (long) cpc_mouse_state_lck(2, id_context);


	long get_key = (long) cpc_get_key_lck(id_context);

	_printl(" *** get_key : %ld\n", get_key);

	// ---- EVENTS ----

	// If user press clic
	if(clic > 0)
		aSysMsg(add,(XEGI_SysMsg){.handle=h, .type=XEGI_Msg_(LBUTTONDOWN)});
	
	// If user left clic
	if(clic == 0)
		aSysMsg(add,(XEGI_SysMsg){.handle=h, .type=XEGI_Msg_(LBUTTONUP)});
	
	// When user mouse move
	aSysMsg(add,(XEGI_SysMsg){.handle=h, .type=XEGI_Msg_(MOUSEMOVE), .x=posx, .y=posy });

	_printl("hndl:%zu - UserInputUpdate(%ld) - Mouse state [clic:%ld] [%ldx%ld] ", h, id_context, clic, posx, posy);


	if (get_key > 0)
	{
		_printl(" ****** KEYDOWN\n");
		aSysMsg(add,(XEGI_SysMsg){.handle=h, .type=XEGI_Msg_(KEYDOWN), .key=get_key});
		for(int index = 0; index < 12; index++)
		{
			if(previous_keydown[index] == get_key) 
				break;

			if(previous_keydown[index] == 0)
				previous_keydown[index] = get_key;
		}
	}

	// get_key = 0;

	if (get_key == 0)
	{
		_printl(" ****** KEYUP\n");
		for(int index = 12; index >= 0; index--)
		{
			if(previous_keydown[index] > 0)
			{
				aSysMsg(add,(XEGI_SysMsg){.handle=h, .type=XEGI_Msg_(KEYUP), .key=previous_keydown[index]});
				previous_keydown[index] = 0;
				break;
			}
		}
	}

	if (get_key < 0)
	{
		get_key = 0-get_key;

		_printl(" ****** KEYUP -- \n");
		for(int index = 12; index >= 0; index--)
		{
			if(previous_keydown[index] == get_key)
			{
				aSysMsg(add,(XEGI_SysMsg){.handle=h, .type=XEGI_Msg_(KEYUP), .key=get_key});
				previous_keydown[index] = 0;
				break;
			}
		}
	}

	_printl("hndl:%zu - UserInputUpdate(%ld) - Keyboard state [key:0x%x ASCII:%ld]", h, id_context, get_key, get_key);
	
}


uint32_t Sys_ProcessMsg(XEGI_SysMsg _msg)
{
	return 0;
}


uint32_t Sys_CreateThread(void* (* threadMain) (void * arg), unsigned long stackSize, void* threadParam)
{
	xe_cpinti_ENTRER_SectionCritique();
	_printl("[CPCDOS] Using CPinti Core threads");
	unsigned long result = xe_cpinti_creer_thread((void* (*) (void *)) threadMain, stackSize, (void*) threadParam, false);
	_printl("[CPCDOS] Results TID CPinti Core thread : %lu", result);
	xe_cpinti_SORTIR_SectionCritique();

	return result;
}


void Sys_EnterCriticalSection()
{
	// Entrer dans une section critique
	xe_cpinti_ENTRER_SectionCritique();
}

void Sys_TryEnterCriticalSection()
{
	// Entrer dans une section critique
	xe_cpinti_ENTRER_SectionCritique();
}

void Sys_LeaveCriticalSection()
{
	// Sortir d'une section critique
	xe_cpinti_SORTIR_SectionCritique();
}

void* Sys_GetCurrentThread()
{
	// Obtenir le thread courant
	return (void*) xe_cpinti_get_ThreadEnCours();
}

double Sys_GetCurrentThreadId()
{
	// Obtenir le thread courant
	return (double)  xe_cpinti_get_ThreadEnCours();
}

double Sys_GetCurrentProcessId()
{
	// Obtenir le thread courant
	return (double)  xe_cpinti_get_pid_from_tid();
}



//////////////// MESSAGES ////////////////

uint32_t XeGI_Get_ContextMsg(hdl_t _context){

    //aSysMsg(add,(XEGI_SysMsg){.handle=_context, .type=XEGI_Msg_(LBUTTONDOWN)});
    //aSysMsg(add,(XEGI_SysMsg){.handle=_context, .type=XEGI_Msg_(LBUTTONUP)});
    //aSysMsg(add,(XEGI_SysMsg){.handle=_context, .type=XEGI_Msg_(MOUSEMOVE), .x=MouseX, .y=MouseY });
    return 1;
}

//////////////////// THREAD ////////////////////

uintptr_t XeGI_CreateThread(XEGI_THREAD_START_ROUTINE threadMain, uint32_t stackSize, void* threadParam)
{
    uintptr_t thdl =0;

	if ((thdl = (uintptr_t)Sys_CreateThread((void* (*) (void *))threadMain, stackSize,(LPVOID)threadParam))){
		return thdl;
	}
	_printl("Error, no thread created");
	return 1;
}

void* XeGI_GetCurrentThread()
{
	// Obtenir le thread courant
	return Sys_GetCurrentThread();
	
}
void XeGI_EnterCriticalSection(void* _handle)
{

}
void XeGI_LeaveCriticalSection(void* _handle)
{

}

////////////////////// FILE //////////////
//typedef struct {
//    hdl_t handle;
//    uint32_t  pos;
//    uint32_t  size;
//    char*  path;
//} XEGI_FileOp;
size_t 
    XeGI_GetFileSize(XEGI_FileOp* _file)
{
    return 0;
}
bool 
    XeGI_ReadFile(XEGI_FileOp* _file)
{
    // _file->size = XeGI_GetFileSize(_file->path)
    // _file->data = malloc _file->size
    return false;
}
bool 
    XeGI_WriteFile(XEGI_FileOp* _file)
{
    return false;
}
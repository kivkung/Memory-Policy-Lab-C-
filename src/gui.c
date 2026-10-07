#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>
#include "simulator.h"
#include "input.h"
#include "search.h"

#define BLUE RGB(36,99,220)
#define INK RGB(24,42,68)
#define MUTED RGB(91,108,133)
#define LINE RGB(226,232,241)
#define SOFT RGB(239,245,255)
#define GREEN RGB(21,117,74)
#define RED RGB(178,53,47)
#define DONE (WM_APP+1)
#define STARTTEST (WM_APP+2)
#define COUNT(a) (sizeof(a)/sizeof((a)[0]))
enum { OPEN=100,SAVE,REFS,DEMO,LOADDEMO,FILELABEL,REFPREVIEW,STATUS,
 NAV0=120,NAV1,NAV2,NAV3,FRAMES=130,ALGO,FLABEL,ALABEL,
 RUN=150,STEP,NEXT,REST,STOP,TRACE,STATS,REASON,
 COMPARE=170,CSTEP,CNEXT,CREST,CSTOP,SUMMARY,SIDE,DIFF,CSUB,CSTATS,
 SWEEP=190,SWEEPLIST,ANOMALY,
 CONDITION=210,K,N,F,LIMIT,SEARCH,SEARCHRESULT,SEARCHPROGRESS,TOCOMPARE,SEARCHSAVE,
 CONDITIONLABEL,KLABEL,NLABEL,SLABEL,LLABEL,SEARCHHELP,PAGETITLE,PAGESUB,EXITAPP };
typedef struct { HWND h; int id,page; } Control;
static Control controls[100]; static int control_count,page=0,frames=3,algo=0,shown=0,cshown=0;
static int stepping=0,cstepping=0,dirty=0,busy=0,smoke=0,test_ok=1;
static HWND win,refwin; static HFONT font,titlefont,smallfont; static HBRUSH white,soft;
static HINSTANCE instance; static float scale=1;
static ReferenceString references; static Step trace[MAX_REFERENCES],lefttrace[MAX_REFERENCES],righttrace[MAX_REFERENCES];
static wchar_t rootdir[MAX_PATH],resultsdir[MAX_PATH],filename[MAX_PATH],smokedir[MAX_PATH];
static SearchConfig searchconfig; static SearchResult searchresult; static HANDLE worker;

static LRESULT CALLBACK ComboPrintProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
static int px(int v){return (int)(v*scale+.5f);}
static HWND ctl(int id){for(int i=0;i<control_count;i++)if(controls[i].id==id)return controls[i].h;return NULL;}
static void text(int id,const wchar_t*s){SetWindowTextW(ctl(id),s);}
static void status(const wchar_t*s){text(STATUS,s);}
static void wide(const char*s,wchar_t*out,int cap){MultiByteToWideChar(CP_UTF8,0,s,-1,out,cap);out[cap-1]=0;}
static void errorbox(const char*s){wchar_t b[700];wide(s,b,700);if(smoke){test_ok=0;return;}MessageBoxW(win,b,L"Memory Policy Lab",MB_OK|MB_ICONWARNING);}
static HWND add(int id,int pg,const wchar_t*cl,const wchar_t*txt,DWORD style,DWORD ex){
 HWND h=CreateWindowExW(ex,cl,txt,WS_CHILD|WS_VISIBLE|style,0,0,1,1,win,(HMENU)(INT_PTR)id,instance,NULL);
 controls[control_count++]=(Control){h,id,pg};SendMessageW(h,WM_SETFONT,(WPARAM)font,TRUE);return h;
}
static void label(int id,int pg,const wchar_t*s){add(id,pg,L"STATIC",s,SS_LEFT,0);}
static void button(int id,int pg,const wchar_t*s){add(id,pg,L"BUTTON",s,BS_OWNERDRAW|WS_TABSTOP,0);}
static void combo(int id,int pg){HWND h=add(id,pg,L"COMBOBOX",L"",CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL|WS_TABSTOP,0);SetWindowTheme(h,L"Explorer",NULL);SetWindowSubclass(h,ComboPrintProc,1,0);}
static void option(int id,const wchar_t*s){SendMessageW(ctl(id),CB_ADDSTRING,0,(LPARAM)s);}
static int selected(int id){return (int)SendMessageW(ctl(id),CB_GETCURSEL,0,0);}
static void edit(int id,int pg,const wchar_t*s){HWND h=add(id,pg,L"EDIT",s,ES_AUTOHSCROLL|WS_TABSTOP|WS_BORDER,0);SendMessageW(h,EM_SETLIMITTEXT,10,0);}
static void list(int id,int pg){HWND h=add(id,pg,WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_TABSTOP,0);ListView_SetExtendedListViewStyle(h,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);ListView_SetBkColor(h,RGB(255,255,255));ListView_SetTextBkColor(h,RGB(255,255,255));ListView_SetTextColor(h,INK);SetWindowTheme(h,L"Explorer",NULL);}
static void column(int id,int n,const wchar_t*s,int width){LVCOLUMNW c={0};c.mask=LVCF_TEXT|LVCF_WIDTH;c.pszText=(wchar_t*)s;c.cx=px(width);ListView_InsertColumn(ctl(id),n,&c);}
static void cell(int id,int row,int col,const wchar_t*s){LVITEMW item={0};item.iItem=row;item.iSubItem=col;item.pszText=(wchar_t*)s;item.mask=LVIF_TEXT;if(col==0)ListView_InsertItem(ctl(id),&item);else ListView_SetItemText(ctl(id),row,col,(wchar_t*)s);}
static void number(int id,int row,int col,unsigned long n){wchar_t b[40];swprintf(b,40,L"%lu",n);cell(id,row,col,b);}
static void move(int id,int x,int y,int w,int h){MoveWindow(ctl(id),px(x),px(y),px(w),px(h),TRUE);}
static void rows_begin(int id){SendMessageW(ctl(id),WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(ctl(id));}
static void rows_end(int id){SendMessageW(ctl(id),WM_SETREDRAW,TRUE,0);InvalidateRect(ctl(id),NULL,TRUE);}
static int capture(const Step*s,void*p){((Step*)p)[s->index]=*s;return 1;}
static Stats partial(const Step*t,int count){Stats s={0};for(int i=0;i<count;i++){if(t[i].fault)s.faults++;else s.hits++;if(t[i].evicted_page!=EMPTY_PAGE)s.replacements++;}return s;}
static void frame_text(const Step*s,wchar_t*b,size_t capacity){size_t used=0;b[0]=0;for(int i=0;i<s->frame_count;i++){int n;if(s->frames[i]==EMPTY_PAGE)n=swprintf(b+used,capacity-used,L"%ls[ - ]",i?L"  ":L"");else n=swprintf(b+used,capacity-used,L"%ls[ %d ]",i?L"  ":L"",s->frames[i]);if(n<0)break;used+=(size_t)n;}}
static void reference_text(wchar_t*b,size_t capacity){size_t used=0;b[0]=0;for(size_t i=0;i<references.count;i++){wchar_t token[32];int n=swprintf(token,32,L"%ls%d",i?L"  ":L"",references.pages[i]);if(n<0||used+(size_t)n+5>=capacity){if(used+5<capacity)wcscat(b,L" ...");break;}wcscpy(b+used,token);used+=(size_t)n;}}
static void enable_controls(void){
 int has=references.count>0;
 int needs[]={SAVE,REFS,RUN,STEP,COMPARE,CSTEP,SWEEP,SEARCHSAVE,TOCOMPARE};
 for(size_t i=0;i<COUNT(needs);i++)EnableWindow(ctl(needs[i]),has&&!busy);
 EnableWindow(ctl(NEXT),has&&stepping&&shown<(int)references.count&&!busy);EnableWindow(ctl(REST),has&&stepping&&shown<(int)references.count&&!busy);EnableWindow(ctl(STOP),stepping&&!busy);
 EnableWindow(ctl(CNEXT),has&&cstepping&&cshown<(int)references.count&&!busy);EnableWindow(ctl(CREST),has&&cstepping&&cshown<(int)references.count&&!busy);EnableWindow(ctl(CSTOP),cstepping&&!busy);
 int inputs[]={OPEN,LOADDEMO,DEMO,FRAMES,ALGO,CONDITION,K,N,F,LIMIT,SEARCH};for(size_t i=0;i<COUNT(inputs);i++)EnableWindow(ctl(inputs[i]),!busy);
}
static void update_refs(void){if(refwin){wchar_t all[16000];reference_text(all,16000);SetWindowTextW(GetDlgItem(refwin,1),all);}wchar_t b[1200],seq[900];reference_text(seq,900);swprintf(b,1200,L"%u references    %ls",(unsigned)references.count,seq);text(REFPREVIEW,b);text(FILELABEL,filename[0]?filename:L"ยังไม่มีข้อมูล — เปิดไฟล์ หรือเลือกตัวอย่างเพื่อเริ่มทดลอง");enable_controls();}
static void trace_stats(void){wchar_t b[500];Stats s=partial(trace,shown);swprintf(b,500,L"HITS  %u        FAULTS  %u        REPLACEMENTS  %u        FAULT RATE  %.2f%%        •        %d / %u steps",(unsigned)s.hits,(unsigned)s.faults,(unsigned)s.replacements,shown?100.*s.faults/shown:0,shown,(unsigned)references.count);text(STATS,b);if(shown){wide(trace[shown-1].reason,b,500);text(REASON,b);}else text(REASON,L"เลือก ‘รันทั้งหมด’ หรือ ‘เริ่มทีละขั้น’ เพื่อเริ่มด้วย frames ว่าง");}
static void show_trace(void){rows_begin(TRACE);for(int i=0;i<shown;i++){wchar_t b[400];number(TRACE,i,0,i+1);number(TRACE,i,1,trace[i].page);frame_text(&trace[i],b,400);cell(TRACE,i,2,b);cell(TRACE,i,3,trace[i].fault?L"Fault":L"Hit");if(trace[i].evicted_page<0)wcscpy(b,L"—");else swprintf(b,400,L"%d → %d",trace[i].evicted_page,trace[i].page);cell(TRACE,i,4,b);}rows_end(TRACE);if(shown)ListView_EnsureVisible(ctl(TRACE),shown-1,FALSE);trace_stats();enable_controls();}
static void simulate_start(int all){Stats s;if(!simulate(&references,frames,(Algorithm)algo,capture,trace,&s))return;shown=all?(int)references.count:1;stepping=!all;show_trace();status(all?L"จำลองครบแล้ว • ทุกการรันเริ่มด้วย frames ว่าง":L"โหมดทีละขั้น • กดขั้นถัดไปเพื่อดำเนินต่อ");}
static void summary(void){rows_begin(SUMMARY);for(int a=0;a<3;a++){Stats s;wchar_t b[80];if(!simulate(&references,frames,(Algorithm)a,NULL,NULL,&s))continue;wide(algorithm_name((Algorithm)a),b,80);cell(SUMMARY,a,0,b);number(SUMMARY,a,1,s.hits);number(SUMMARY,a,2,s.faults);number(SUMMARY,a,3,s.replacements);swprintf(b,80,L"%.2f%%",100.*s.faults/references.count);cell(SUMMARY,a,4,b);}rows_end(SUMMARY);}
static void show_side(void){int decision=-1,outcome=-1;rows_begin(SIDE);for(int i=0;i<cshown;i++){wchar_t b[500],reason[200];Step*l=&lefttrace[i],*r=&righttrace[i];number(SIDE,i,0,i+1);number(SIDE,i,1,l->page);frame_text(l,b,500);cell(SIDE,i,2,b);cell(SIDE,i,3,l->fault?L"Fault":L"Hit");frame_text(r,b,500);cell(SIDE,i,4,b);cell(SIDE,i,5,r->fault?L"Fault":L"Hit");wide(l->reason,reason,200);cell(SIDE,i,6,reason);wide(r->reason,reason,200);cell(SIDE,i,7,reason);if(decision<0&&l->evicted_page!=r->evicted_page)decision=i+1;if(outcome<0&&l->fault!=r->fault)outcome=i+1;}rows_end(SIDE);wchar_t b[500],d[30],o[30];if(decision<0)wcscpy(d,L"ยังไม่พบ");else swprintf(d,30,L"ขั้น %d",decision);if(outcome<0)wcscpy(o,L"ยังไม่พบ");else swprintf(o,30,L"ขั้น %d",outcome);swprintf(b,500,L"เฉพาะขั้นที่แสดง: เลือกหน้าออกต่างกัน %ls    •    Hit/Fault ต่างกัน %ls",d,o);text(DIFF,b);Stats l=partial(lefttrace,cshown),r=partial(righttrace,cshown);swprintf(b,500,L"%d / %u steps     FIFO: %u hits / %u faults / %u replacements     LRU: %u hits / %u faults / %u replacements",cshown,(unsigned)references.count,(unsigned)l.hits,(unsigned)l.faults,(unsigned)l.replacements,(unsigned)r.hits,(unsigned)r.faults,(unsigned)r.replacements);text(CSTATS,b);if(cshown)ListView_EnsureVisible(ctl(SIDE),cshown-1,FALSE);enable_controls();}
static void compare_start(int all){Stats s;if(!references.count)return;summary();simulate(&references,frames,FIFO,capture,lefttrace,&s);simulate(&references,frames,LRU,capture,righttrace,&s);cshown=all?(int)references.count:1;cstepping=!all;show_side();}
static void sweep(void){rows_begin(SWEEPLIST);int found=0;size_t prev=0;for(int f=1;f<=10;f++){Stats s[3];for(int a=0;a<3;a++)simulate(&references,f,(Algorithm)a,NULL,NULL,&s[a]);number(SWEEPLIST,f-1,0,f);for(int a=0;a<3;a++)number(SWEEPLIST,f-1,a+1,s[a].faults);wchar_t b[120]=L"—";if(f>1&&s[0].faults>prev){swprintf(b,120,L"%d → %d frames: %u → %u faults",f-1,f,(unsigned)prev,(unsigned)s[0].faults);found=1;}cell(SWEEPLIST,f-1,4,b);prev=s[0].faults;}rows_end(SWEEPLIST);text(ANOMALY,found?L"พบ Belady’s anomaly • FIFO เกิด faults มากขึ้นเมื่อเพิ่ม frames (ดูแถวที่ระบุ)":L"ไม่พบ Belady’s anomaly ในช่วง 1–10 frames ของชุดข้อมูลนี้");}
static void clear_results(void){shown=cshown=0;stepping=cstepping=0;rows_begin(TRACE);rows_end(TRACE);rows_begin(SUMMARY);rows_end(SUMMARY);rows_begin(SIDE);rows_end(SIDE);rows_begin(SWEEPLIST);rows_end(SWEEPLIST);text(DIFF,L"");text(CSTATS,L"");text(ANOMALY,L"กดเปรียบเทียบเพื่อดู faults ที่ 1–10 frames");trace_stats();update_refs();}
static int discard_ok(void){return !dirty||smoke||MessageBoxW(win,L"ชุดข้อมูลจากการค้นหายังไม่ได้บันทึก ต้องการเปลี่ยนข้อมูลหรือไม่?",L"เปลี่ยนชุดข้อมูล",MB_YESNO|MB_ICONQUESTION)==IDYES;}
static int load_path(const wchar_t*path){ReferenceString candidate;char err[500];if(!load_references_stream(_wfopen(path,L"rb"),&candidate,err,sizeof(err))){errorbox(err);return 0;}references=candidate;dirty=0;const wchar_t*base=wcsrchr(path,L'\\');wcsncpy(filename,base?base+1:path,MAX_PATH-1);clear_results();text(SEARCHRESULT,L"พร้อมค้นหา • ผลการค้นหาจะเปิดใช้ชุดข้อมูลและ frames โดยอัตโนมัติ");status(L"โหลดไฟล์สำเร็จ • ตั้งค่าแล้วเริ่มจำลองได้เลย");return 1;}
static void open_file(void){if(!discard_ok())return;wchar_t path[MAX_PATH]=L"";OPENFILENAMEW o={0};o.lStructSize=sizeof(o);o.hwndOwner=win;o.lpstrFilter=L"Reference text (*.txt)\0*.txt\0All files\0*.*\0";o.lpstrFile=path;o.nMaxFile=MAX_PATH;o.lpstrInitialDir=resultsdir;o.lpstrTitle=L"เปิดชุดข้อมูล • ตัวเลขไม่ติดลบ คั่นด้วยช่องว่าง";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(GetOpenFileNameW(&o))load_path(path);}
static int save_path(const wchar_t*path){FILE*f=_wfopen(path,L"wb");if(!f){errorbox("Cannot save this file. Choose a writable folder.");return 0;}int ok=1;for(size_t i=0;i<references.count;i++)if(fprintf(f,"%d%c",references.pages[i],i+1==references.count?'\n':' ')<0){ok=0;break;}if(fclose(f)!=0)ok=0;if(!ok){errorbox("The file could not be fully written.");return 0;}dirty=0;const wchar_t*b=wcsrchr(path,L'\\');wcsncpy(filename,b?b+1:path,MAX_PATH-1);update_refs();status(L"บันทึกสำเร็จ • ไฟล์เก็บเฉพาะ reference string ไม่รวม frames และสถิติ");return 1;}
static void save_file(void){if(!references.count)return;wchar_t path[MAX_PATH],check[MAX_PATH];int n=1;do{swprintf(path,MAX_PATH,L"result_%03d.txt",n++);swprintf(check,MAX_PATH,L"%ls\\%ls",resultsdir,path);}while(GetFileAttributesW(check)!=INVALID_FILE_ATTRIBUTES&&n<1000000);OPENFILENAMEW o={0};o.lStructSize=sizeof(o);o.hwndOwner=win;o.lpstrFilter=L"Reference text (*.txt)\0*.txt\0";o.lpstrFile=path;o.nMaxFile=MAX_PATH;o.lpstrInitialDir=resultsdir;o.lpstrDefExt=L"txt";o.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;o.lpstrTitle=L"บันทึกชุดข้อมูล";if(GetSaveFileNameW(&o))save_path(path);}
static LRESULT CALLBACK RefProc(HWND h,UINT m,WPARAM w,LPARAM l){if(m==WM_SIZE){MoveWindow(GetDlgItem(h,1),16,16,LOWORD(l)-32,HIWORD(l)-32,TRUE);return 0;}if(m==WM_CLOSE){DestroyWindow(h);return 0;}if(m==WM_DESTROY){refwin=NULL;return 0;}return DefWindowProcW(h,m,w,l);}
static void show_refs(void){if(refwin){SetForegroundWindow(refwin);return;}refwin=CreateWindowExW(WS_EX_TOOLWINDOW,L"MPLReferences",L"Reference string • เลือกข้อความและกด Ctrl+C เพื่อคัดลอก",WS_OVERLAPPEDWINDOW|(smoke?0:WS_VISIBLE),CW_USEDEFAULT,CW_USEDEFAULT,px(700),px(330),win,NULL,instance,NULL);HWND e=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,16,16,px(650),px(250),refwin,(HMENU)1,instance,NULL);SendMessageW(e,WM_SETFONT,(WPARAM)font,TRUE);wchar_t*b=calloc(16000,sizeof(wchar_t));if(b){reference_text(b,16000);SetWindowTextW(e,b);free(b);}RECT r;GetClientRect(refwin,&r);SendMessageW(refwin,WM_SIZE,0,MAKELPARAM(r.right,r.bottom));}
static DWORD WINAPI search_thread(void*p){(void)p;search_references(&searchconfig,&searchresult);PostMessageW(win,DONE,0,0);return 0;}
static int field(int id,int lo,int hi,int*value){char b[100];GetWindowTextA(ctl(id),b,100);if(!parse_integer(b,lo,hi,value)){wchar_t err[160];swprintf(err,160,L"กรุณากรอกจำนวนเต็มระหว่าง %d ถึง %d",lo,hi);if(!smoke)MessageBoxW(win,err,L"ตรวจสอบข้อมูล",MB_OK|MB_ICONWARNING);SetFocus(ctl(id));SendMessageW(ctl(id),EM_SETSEL,0,-1);return 0;}return 1;}
static void start_search(void){int limit;searchconfig.condition=(SearchCondition)(selected(CONDITION)+1);if(!field(K,1,6,&searchconfig.page_count)||!field(N,1,12,&searchconfig.length)||!field(F,1,searchconfig.condition==FIFO_BELADY?9:10,&searchconfig.frame_count)||!field(LIMIT,1,100000,&limit))return;if(!discard_ok())return;searchconfig.limit=(unsigned long)limit;busy=1;enable_controls();SendMessageW(ctl(SEARCHPROGRESS),PBM_SETMARQUEE,TRUE,30);text(SEARCHRESULT,L"กำลังค้นหาอย่างเป็นระบบ … หน้าต่างยังใช้งานได้");status(L"กำลังค้นหาชุดข้อมูล");worker=CreateThread(NULL,0,search_thread,NULL,0,NULL);if(!worker){busy=0;SendMessageW(ctl(SEARCHPROGRESS),PBM_SETMARQUEE,FALSE,0);enable_controls();errorbox("Could not start the search worker. Please try again.");}}
static void finish_search(void){if(worker){WaitForSingleObject(worker,INFINITE);CloseHandle(worker);worker=NULL;}busy=0;SendMessageW(ctl(SEARCHPROGRESS),PBM_SETMARQUEE,FALSE,0);wchar_t b[1600],r[500];if(searchresult.status==SEARCH_FOUND){references=searchresult.references;frames=searchconfig.frame_count;SendMessageW(ctl(FRAMES),CB_SETCURSEL,frames-1,0);dirty=1;wcscpy(filename,L"ข้อมูลจากการค้นหา • ยังไม่บันทึก");clear_results();reference_text(r,500);swprintf(b,1600,L"MATCH FOUND\r\n\r\nตรวจ %lu / %lu ชุด\r\nชุดข้อมูล: %ls\r\n\r\nFIFO (%d frames): %u faults\r\n%ls (%d frames): %u faults\r\n\r\nเปิดใช้ข้อมูลและ frames แล้ว • เป็นคำตอบแรกตามลำดับค้นหา ไม่ได้ยืนยันว่าสั้นที่สุด",searchresult.tested,searchresult.total,r,frames,(unsigned)searchresult.left.faults,searchconfig.condition==FIFO_BELADY?L"FIFO":L"LRU",frames+(searchconfig.condition==FIFO_BELADY),(unsigned)searchresult.right.faults);status(L"ค้นพบแล้ว • ไปวิเคราะห์หรือบันทึกข้อมูลได้");}else if(searchresult.status==SEARCH_EXHAUSTED){swprintf(b,1600,L"NO MATCH IN THIS SEARCH SPACE\r\n\r\nตรวจครบ %lu / %lu ชุดแล้วไม่พบ\r\n\r\nไม่ได้แปลว่าไม่มีคำตอบเมื่อใช้ค่าอื่น\r\nข้อมูลและ frames เดิมไม่เปลี่ยน",searchresult.tested,searchresult.total);status(L"ตรวจครบขอบเขตแล้วไม่พบ");}else if(searchresult.status==SEARCH_LIMIT_REACHED){swprintf(b,1600,L"SEARCH LIMIT REACHED\r\n\r\nตรวจ %lu / %lu ชุด\r\n\r\nยังตรวจไม่ครบ ชุดที่เหลืออาจมีคำตอบ\r\nข้อมูลและ frames เดิมไม่เปลี่ยน",searchresult.tested,searchresult.total);status(L"ถึงขีดจำกัดก่อนตรวจครบ");}else wcscpy(b,L"การตั้งค่าค้นหาไม่ถูกต้อง");text(SEARCHRESULT,b);enable_controls();}
static void layout(void){RECT r;GetClientRect(win,&r);int w=(int)(r.right/scale),h=(int)(r.bottom/scale),right=w-32;
 move(OPEN,32,137,115,36);move(SAVE,157,137,130,36);move(REFS,297,137,125,36);move(DEMO,448,137,180,220);move(LOADDEMO,638,137,125,36);move(EXITAPP,right-85,137,85,36);move(FILELABEL,32,185,w-64,26);move(REFPREVIEW,32,216,w-64,28);move(STATUS,32,h-35,w-64,27);
 const int navwidths[]={135,160,170,180};int x=32;for(int i=0;i<4;i++){move(NAV0+i,x,78,navwidths[i],39);x+=navwidths[i]+10;}
 move(PAGETITLE,32,269,w-64,30);move(PAGESUB,32,303,w-64,25);
 move(FLABEL,32,343,150,25);move(FRAMES,32,371,150,260);move(ALABEL,202,343,200,25);move(ALGO,202,371,200,220);
 move(RUN,430,371,133,36);move(STEP,573,371,130,36);move(NEXT,713,371,110,36);move(REST,833,371,115,36);move(STOP,958,371,75,36);
 move(STATS,32,424,w-64,29);move(TRACE,32,464,w-64,h-583);move(REASON,32,h-101,w-64,56);
 move(COMPARE,32,343,200,36);move(SUMMARY,32,391,w-64,117);move(CSUB,32,524,280,28);move(CSTEP,325,519,130,36);move(CNEXT,465,519,110,36);move(CREST,585,519,125,36);move(CSTOP,720,519,90,36);move(SIDE,32,568,w-64,h-725);move(CSTATS,32,h-145,w-64,27);move(DIFF,32,h-108,w-64,48);
 move(SWEEP,32,346,240,36);move(SWEEPLIST,32,401,w-64,h-523);move(ANOMALY,32,h-101,w-64,50);
 int colw=(w-100)/2;move(CONDITIONLABEL,32,343,colw,26);move(CONDITION,32,373,colw,220);move(KLABEL,32,424,210,26);move(K,32,453,180,33);move(NLABEL,242,424,240,26);move(N,242,453,180,33);move(SLABEL,32,508,210,26);move(F,32,537,180,33);move(LLABEL,242,508,240,26);move(LIMIT,242,537,180,33);move(SEARCHHELP,32,587,colw,57);move(SEARCH,32,658,160,38);move(SEARCHPROGRESS,32,714,colw,7);
 int rx=colw+68;move(SEARCHRESULT,rx,343,w-rx-32,h-470);move(TOCOMPARE,rx,h-105,160,38);move(SEARCHSAVE,rx+173,h-105,150,38);
 for(int i=0;i<control_count;i++){int pg=controls[i].page;BOOL visible=pg==-1||pg==page||(pg==-2&&page<2);ShowWindow(controls[i].h,visible?SW_SHOW:SW_HIDE);}InvalidateRect(win,NULL,FALSE);
}
static void set_page(int p){page=p;const wchar_t*t[]={L"Simulation",L"Compare algorithms",L"Frame count experiment",L"Find an interesting input"};const wchar_t*s[]={L"จำลองทีละขั้นหรือทั้งหมด • เริ่มใหม่ด้วย frames ว่างทุกครั้ง",L"สรุปทั้งสามวิธี และวิเคราะห์ FIFO / LRU ข้างกัน",L"เปรียบเทียบ faults ที่ 1–10 frames • ตรวจ Belady’s anomaly ของ FIFO",L"กำหนดเงื่อนไขและขอบเขต เพื่อค้นหาชุดข้อมูลที่ให้ผลต่างกัน"};text(PAGETITLE,t[p]);text(PAGESUB,s[p]);if(p==1){ShowWindow(ctl(ALABEL),SW_HIDE);ShowWindow(ctl(ALGO),SW_HIDE);}layout();/* comparison frames are shown in subtitle to keep its toolbar compact */if(p==1){ShowWindow(ctl(FLABEL),SW_HIDE);ShowWindow(ctl(FRAMES),SW_HIDE);ShowWindow(ctl(ALABEL),SW_HIDE);ShowWindow(ctl(ALGO),SW_HIDE);wchar_t b[250];swprintf(b,250,L"ใช้ %d frames • เปลี่ยนจำนวนได้ในหน้า Simulation • ทุกวิธีเริ่มด้วย frames ว่าง",frames);text(PAGESUB,b);} }
static void create_controls(void){
 button(OPEN,-1,L"เปิดไฟล์");button(SAVE,-1,L"บันทึกข้อมูล");button(REFS,-1,L"ดูชุดข้อมูล");combo(DEMO,-1);option(DEMO,L"ตัวอย่าง: LRU wins");option(DEMO,L"ตัวอย่าง: FIFO wins");option(DEMO,L"ตัวอย่าง: Belady");SendMessageW(ctl(DEMO),CB_SETCURSEL,0,0);button(LOADDEMO,-1,L"โหลดตัวอย่าง");button(EXITAPP,-1,L"ออก");label(FILELABEL,-1,L"");label(REFPREVIEW,-1,L"");label(STATUS,-1,L"พร้อมเริ่มทดลอง");
 button(NAV0,-1,L"จำลอง");button(NAV1,-1,L"เปรียบเทียบ");button(NAV2,-1,L"จำนวน Frames");button(NAV3,-1,L"ค้นหาชุดข้อมูล");label(PAGETITLE,-1,L"");SendMessageW(ctl(PAGETITLE),WM_SETFONT,(WPARAM)titlefont,TRUE);label(PAGESUB,-1,L"");
 label(FLABEL,0,L"จำนวน Frames (1–10)");combo(FRAMES,0);for(int i=1;i<=10;i++){wchar_t b[8];swprintf(b,8,L"%d",i);option(FRAMES,b);}SendMessageW(ctl(FRAMES),CB_SETCURSEL,2,0);label(ALABEL,0,L"Algorithm");combo(ALGO,0);option(ALGO,L"FIFO");option(ALGO,L"LRU");option(ALGO,L"Optimal");SendMessageW(ctl(ALGO),CB_SETCURSEL,0,0);
 button(RUN,0,L"รันทั้งหมด");button(STEP,0,L"เริ่มทีละขั้น");button(NEXT,0,L"ขั้นถัดไป");button(REST,0,L"รันที่เหลือ");button(STOP,0,L"หยุด");label(STATS,0,L"");label(REASON,0,L"");list(TRACE,0);column(TRACE,0,L"STEP",60);column(TRACE,1,L"PAGE",70);column(TRACE,2,L"FRAMES AFTER ACCESS",500);column(TRACE,3,L"RESULT",100);column(TRACE,4,L"REPLACED",170);
 button(COMPARE,1,L"เปรียบเทียบ / แสดงทั้งหมด");list(SUMMARY,1);const wchar_t*headers[]={L"ALGORITHM",L"HITS",L"FAULTS",L"REPLACEMENTS",L"FAULT RATE"};for(int i=0;i<5;i++)column(SUMMARY,i,headers[i],i?170:220);label(CSUB,1,L"FIFO / LRU • ทีละขั้น");button(CSTEP,1,L"เริ่มทีละขั้น");button(CNEXT,1,L"ขั้นถัดไป");button(CREST,1,L"รันที่เหลือ");button(CSTOP,1,L"หยุด");list(SIDE,1);const wchar_t*sh[]={L"STEP",L"PAGE",L"FIFO FRAMES",L"RESULT",L"LRU FRAMES",L"RESULT",L"FIFO REASON",L"LRU REASON"};int widths[]={55,60,260,85,260,85,390,390};for(int i=0;i<8;i++)column(SIDE,i,sh[i],widths[i]);label(DIFF,1,L"");label(CSTATS,1,L"");
 button(SWEEP,2,L"เปรียบเทียบ 1–10 Frames");list(SWEEPLIST,2);const wchar_t*fh[]={L"FRAMES",L"FIFO",L"LRU",L"OPTIMAL",L"FIFO ANOMALY"};int fw[]={100,145,145,160,440};for(int i=0;i<5;i++)column(SWEEPLIST,i,fh[i],fw[i]);label(ANOMALY,2,L"");
 label(CONDITIONLABEL,3,L"Condition • เงื่อนไข (1–3)");combo(CONDITION,3);option(CONDITION,L"1. FIFO faults < LRU faults");option(CONDITION,L"2. LRU faults < FIFO faults");option(CONDITION,L"3. FIFO faults increase at F+1 frames");SendMessageW(ctl(CONDITION),CB_SETCURSEL,1,0);
 label(KLABEL,3,L"Page IDs (1–6)");edit(K,3,L"3");label(NLABEL,3,L"Exact length (1–12)");edit(N,3,L"5");label(SLABEL,3,L"Frames (1–10)");edit(F,3,L"2");label(LLABEL,3,L"Search limit (1–100000)");edit(LIMIT,3,L"100000");label(SEARCHHELP,3,L"Page IDs เริ่มที่ 0 • เช่น 3 = {0, 1, 2}\r\nค้นพบแล้วจะใช้ข้อมูลและ frames นี้ทันที");button(SEARCH,3,L"ค้นหาชุดข้อมูล");add(SEARCHPROGRESS,3,PROGRESS_CLASSW,L"",PBS_MARQUEE,0);SendMessageW(ctl(SEARCHPROGRESS),PBM_SETBARCOLOR,0,BLUE);label(SEARCHRESULT,3,L"พร้อมค้นหา\r\n\r\nค่าเริ่มต้นเป็นตัวอย่างที่ LRU ชนะ FIFO\r\n\r\nกดค้นหาเพื่อเริ่มทดลอง");button(TOCOMPARE,3,L"ไปวิเคราะห์");button(SEARCHSAVE,3,L"บันทึกข้อมูล");
}
static void action(int id,int code){
 if(id>=NAV0&&id<=NAV3){set_page(id-NAV0);return;}if(id==EXITAPP){SendMessageW(win,WM_CLOSE,0,0);return;}if(busy)return;
 switch(id){case OPEN:open_file();break;case SAVE:case SEARCHSAVE:save_file();break;case REFS:show_refs();break;
 case LOADDEMO:if(discard_ok()){const wchar_t*names[]={L"lru_wins.txt",L"fifo_wins.txt",L"belady.txt"};wchar_t path[MAX_PATH];int s=selected(DEMO);swprintf(path,MAX_PATH,L"%ls\\examples\\%ls",rootdir,names[s]);if(load_path(path)){frames=s==0?2:3;SendMessageW(ctl(FRAMES),CB_SETCURSEL,frames-1,0);clear_results();}}break;
 case FRAMES:if(code==CBN_SELCHANGE){frames=selected(FRAMES)+1;clear_results();status(L"เปลี่ยน frames แล้ว • เริ่มรันใหม่เพื่อดูผล");}break;
 case ALGO:if(code==CBN_SELCHANGE){algo=selected(ALGO);shown=0;stepping=0;show_trace();}break;
 case CONDITION:if(code==CBN_SELCHANGE)text(SLABEL,selected(CONDITION)==2?L"Base frames (1–9)":L"Frames (1–10)");break;
 case RUN:simulate_start(1);break;case STEP:simulate_start(0);break;case NEXT:if(stepping&&shown<(int)references.count){shown++;if(shown==(int)references.count)stepping=0;show_trace();}break;case REST:if(stepping){shown=(int)references.count;stepping=0;show_trace();}break;case STOP:stepping=0;enable_controls();status(L"หยุดแล้ว • สถิติเฉพาะขั้นที่แสดง • เริ่มทีละขั้นเพื่อเริ่มใหม่");break;
 case COMPARE:compare_start(1);break;case CSTEP:compare_start(0);break;case CNEXT:if(cstepping&&cshown<(int)references.count){cshown++;if(cshown==(int)references.count)cstepping=0;show_side();}break;case CREST:if(cstepping){cshown=(int)references.count;cstepping=0;show_side();}break;case CSTOP:cstepping=0;enable_controls();status(L"หยุดวิเคราะห์ • สรุปเฉพาะขั้นที่แสดง");break;
 case SWEEP:sweep();break;case SEARCH:start_search();break;case TOCOMPARE:set_page(1);compare_start(1);break;default:break;}
}
static void draw_button(DRAWITEMSTRUCT*d){int id=(int)d->CtlID;BOOL disabled=(d->itemState&ODS_DISABLED)!=0,active=(id==NAV0+page||id==RUN||id==SEARCH),down=(d->itemState&ODS_SELECTED)!=0;COLORREF bg=disabled?RGB(245,247,250):active?(down?RGB(24,76,175):BLUE):(down?RGB(216,231,255):SOFT);HBRUSH b=CreateSolidBrush(bg);HPEN p=CreatePen(PS_SOLID,1,bg);HGDIOBJ ob=SelectObject(d->hDC,b),op=SelectObject(d->hDC,p);RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,px(10),px(10));SelectObject(d->hDC,ob);SelectObject(d->hDC,op);DeleteObject(b);DeleteObject(p);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,disabled?RGB(145,156,173):active?RGB(255,255,255):BLUE);SelectObject(d->hDC,font);wchar_t s[150];GetWindowTextW(d->hwndItem,s,150);DrawTextW(d->hDC,s,-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(d->itemState&ODS_FOCUS){RECT r=d->rcItem;InflateRect(&r,-4,-4);DrawFocusRect(d->hDC,&r);}}
static void draw_combo(DRAWITEMSTRUCT*d){
 BOOL focus=(d->itemState&ODS_SELECTED)&&!(d->itemState&ODS_COMBOBOXEDIT);HBRUSH b=CreateSolidBrush(focus?BLUE:RGB(255,255,255));FillRect(d->hDC,&d->rcItem,b);DeleteObject(b);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,focus?RGB(255,255,255):INK);SelectObject(d->hDC,font);wchar_t s[300]=L"";if(d->itemID!=(UINT)-1)SendMessageW(d->hwndItem,CB_GETLBTEXT,d->itemID,(LPARAM)s);RECT r=d->rcItem;r.left+=px(7);DrawTextW(d->hDC,s,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&d->rcItem);
}
/* Native combo boxes omit the selection during hidden-window WM_PRINT.
   Print the same owner-drawn selection when exporting a QA capture. */
static LRESULT CALLBACK ComboPrintProc(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR data){
 (void)id;(void)data;
 if(m==WM_PRINT){int save=SaveDC((HDC)w);LRESULT r=DefSubclassProc(h,m,w,l);RestoreDC((HDC)w,save);COMBOBOXINFO info={0};info.cbSize=sizeof(info);if(GetComboBoxInfo(h,&info)){DRAWITEMSTRUCT d={0};d.CtlType=ODT_COMBOBOX;d.hwndItem=h;d.hDC=(HDC)w;d.itemID=(UINT)SendMessageW(h,CB_GETCURSEL,0,0);d.itemState=ODS_COMBOBOXEDIT;d.rcItem=info.rcItem;draw_combo(&d);}return r;}
 if(m==WM_NCDESTROY)RemoveWindowSubclass(h,ComboPrintProc,1);
 return DefSubclassProc(h,m,w,l);
}
static void snapshot_children(HDC dc){
 SendMessageW(win,WM_PRINTCLIENT,(WPARAM)dc,0);
 POINT origin={0,0};ClientToScreen(win,&origin);
 for(int i=0;i<control_count;i++){
  HWND child=controls[i].h;if(!(GetWindowLongPtrW(child,GWL_STYLE)&WS_VISIBLE))continue;
  RECT r;GetWindowRect(child,&r);int save=SaveDC(dc);SetViewportOrgEx(dc,r.left-origin.x,r.top-origin.y,NULL);
  IntersectClipRect(dc,0,0,r.right-r.left,r.bottom-r.top);
  SendMessageW(child,WM_PRINT,(WPARAM)dc,PRF_CLIENT|PRF_NONCLIENT|PRF_CHILDREN|PRF_ERASEBKGND);
  RestoreDC(dc,save);
 }
}
static void snapshot(const wchar_t*name){wchar_t path[MAX_PATH];swprintf(path,MAX_PATH,L"%ls\\%ls.bmp",smokedir,name);RECT r;GetClientRect(win,&r);HDC dc=GetDC(win),mem=CreateCompatibleDC(dc);BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=r.right;bi.bmiHeader.biHeight=-r.bottom;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;void*bits=NULL;HBITMAP bm=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,NULL,0);HGDIOBJ old=SelectObject(mem,bm);snapshot_children(mem);DWORD bytes=(DWORD)(r.right*r.bottom*4);BITMAPFILEHEADER fh={0};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+bytes;FILE*f=_wfopen(path,L"wb");if(f){fwrite(&fh,sizeof(fh),1,f);fwrite(&bi.bmiHeader,sizeof(BITMAPINFOHEADER),1,f);fwrite(bits,bytes,1,f);fclose(f);}SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);ReleaseDC(win,dc);}
#define CHECK(x) do{if(!(x)){test_ok=0;fwprintf(log,L"FAIL line %d: %hs\n",__LINE__,#x);}}while(0)
static void smoke_test(void){wchar_t path[MAX_PATH];swprintf(path,MAX_PATH,L"%ls\\gui-test.log",smokedir);FILE*log=_wfopen(path,L"w");if(!log){DestroyWindow(win);return;}CHECK(IsWindow(ctl(SEARCH)));CHECK(!IsWindowEnabled(ctl(RUN)));
 SendMessageW(ctl(DEMO),CB_SETCURSEL,1,0);action(LOADDEMO,BN_CLICKED);CHECK(references.count==7&&frames==3);action(RUN,BN_CLICKED);CHECK(shown==7&&partial(trace,shown).faults==4);snapshot(L"01-simulation");action(STEP,BN_CLICKED);CHECK(shown==1&&stepping);action(NEXT,BN_CLICKED);CHECK(shown==2);action(STOP,BN_CLICKED);CHECK(!IsWindowEnabled(ctl(NEXT)));action(STEP,BN_CLICKED);CHECK(shown==1);action(REST,BN_CLICKED);CHECK(shown==7);
 set_page(1);action(COMPARE,BN_CLICKED);CHECK(partial(lefttrace,cshown).faults==4&&partial(righttrace,cshown).faults==5);CHECK(ListView_GetItemCount(ctl(SUMMARY))==3);snapshot(L"02-compare");action(CSTEP,BN_CLICKED);CHECK(cshown==1);action(CNEXT,BN_CLICKED);CHECK(cshown==2);action(CSTOP,BN_CLICKED);CHECK(!IsWindowEnabled(ctl(CNEXT)));
 SendMessageW(ctl(DEMO),CB_SETCURSEL,2,0);action(LOADDEMO,BN_CLICKED);set_page(2);action(SWEEP,BN_CLICKED);Stats a,b;simulate(&references,3,FIFO,NULL,NULL,&a);simulate(&references,4,FIFO,NULL,NULL,&b);CHECK(a.faults==9&&b.faults==10);snapshot(L"03-frames");
 set_page(3);text(K,L"4");text(N,L"7");text(F,L"3");text(LIMIT,L"100000");SendMessageW(ctl(CONDITION),CB_SETCURSEL,0,0);action(SEARCH,BN_CLICKED);CHECK(busy&&worker!=NULL);if(worker)WaitForSingleObject(worker,10000);MSG msg;while(PeekMessageW(&msg,win,DONE,DONE,PM_REMOVE))DispatchMessageW(&msg);CHECK(!busy&&searchresult.status==SEARCH_FOUND&&searchresult.tested==302&&frames==3);snapshot(L"04-search");
 swprintf(path,MAX_PATH,L"%ls\\ทดสอบข้อมูล.txt",smokedir);CHECK(save_path(path));references.count=0;CHECK(load_path(path)&&references.count==7);ReferenceString before=references;text(K,L"1");text(N,L"1");text(F,L"1");text(LIMIT,L"1");action(SEARCH,BN_CLICKED);if(worker)WaitForSingleObject(worker,10000);while(PeekMessageW(&msg,win,DONE,DONE,PM_REMOVE))DispatchMessageW(&msg);CHECK(searchresult.status==SEARCH_EXHAUSTED&&references.count==before.count&&frames==3);
 SendMessageW(ctl(CONDITION),CB_SETCURSEL,1,0);text(K,L"3");text(N,L"5");text(F,L"2");action(SEARCH,BN_CLICKED);if(worker)WaitForSingleObject(worker,10000);while(PeekMessageW(&msg,win,DONE,DONE,PM_REMOVE))DispatchMessageW(&msg);CHECK(searchresult.status==SEARCH_LIMIT_REACHED&&references.count==before.count);text(K,L"1'");action(SEARCH,BN_CLICKED);CHECK(!busy);
 swprintf(path,MAX_PATH,L"%ls\\invalid.txt",smokedir);FILE*f=_wfopen(path,L"wb");if(f){fputs("1,2,3",f);fclose(f);}char err[256];ReferenceString r=references;CHECK(!load_references_stream(_wfopen(path,L"rb"),&r,err,sizeof(err))&&r.count==references.count);
  references.count=MAX_REFERENCES;for(size_t i=0;i<references.count;i++)references.pages[i]=2147483647-(int)(i%11);frames=10;algo=OPTIMAL;SendMessageW(ctl(FRAMES),CB_SETCURSEL,9,0);SendMessageW(ctl(ALGO),CB_SETCURSEL,OPTIMAL,0);clear_results();set_page(0);action(RUN,BN_CLICKED);CHECK(shown==MAX_REFERENCES&&ListView_GetItemCount(ctl(TRACE))==MAX_REFERENCES);wchar_t allrefs[16000];reference_text(allrefs,16000);CHECK(wcslen(allrefs)>10000&&wcsstr(allrefs,L"...")==NULL);show_refs();CHECK(refwin!=NULL);if(refwin)DestroyWindow(refwin);
 SetWindowPos(win,NULL,0,0,px(1100),px(850),SWP_NOMOVE|SWP_NOZORDER);set_page(3);RECT client,control;POINT pt;GetClientRect(win,&client);int ids[]={CONDITION,K,N,F,LIMIT,SEARCH,SEARCHSAVE,TOCOMPARE};for(size_t i=0;i<COUNT(ids);i++){GetWindowRect(ctl(ids[i]),&control);pt.x=control.right;pt.y=control.bottom;ScreenToClient(win,&pt);CHECK(pt.x<=client.right&&pt.y<=client.bottom);}
fwprintf(log,L"%ls\n",test_ok?L"PASS: GUI workflow, step/restart/stop, comparisons, Belady, worker search found/exhausted/limit, validation, Unicode save/reload.":L"FAILED");fclose(log);DestroyWindow(win);}
static LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){
 case WM_CREATE:win=h;create_controls();clear_results();set_page(0);return 0;
 case WM_GETMINMAXINFO:((MINMAXINFO*)l)->ptMinTrackSize.x=px(1100);((MINMAXINFO*)l)->ptMinTrackSize.y=px(850);return 0;
 case WM_SIZE:layout();return 0;
 case WM_COMMAND:action(LOWORD(w),HIWORD(w));return 0;
 case WM_DRAWITEM:if(((DRAWITEMSTRUCT*)l)->CtlType==ODT_COMBOBOX)draw_combo((DRAWITEMSTRUCT*)l);else draw_button((DRAWITEMSTRUCT*)l);return TRUE;
 case WM_MEASUREITEM:if(((MEASUREITEMSTRUCT*)l)->CtlType==ODT_COMBOBOX){((MEASUREITEMSTRUCT*)l)->itemHeight=px(25);return TRUE;}return 0;
 case WM_CTLCOLORSTATIC:{HDC dc=(HDC)w;SetBkColor(dc,RGB(255,255,255));SetTextColor(dc,(HWND)l==ctl(PAGETITLE)?INK:MUTED);return (LRESULT)white;}
 case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:SetBkColor((HDC)w,RGB(255,255,255));SetTextColor((HDC)w,INK);return (LRESULT)white;
 case WM_NOTIFY:{NMHDR*n=(NMHDR*)l;if(n->code==NM_CUSTOMDRAW&&(n->idFrom==TRACE||n->idFrom==SIDE)){NMLVCUSTOMDRAW*d=(NMLVCUSTOMDRAW*)l;if(d->nmcd.dwDrawStage==CDDS_PREPAINT)return CDRF_NOTIFYITEMDRAW;if(d->nmcd.dwDrawStage==CDDS_ITEMPREPAINT)return CDRF_NOTIFYSUBITEMDRAW;if(d->nmcd.dwDrawStage==(CDDS_ITEMPREPAINT|CDDS_SUBITEM)){int row=(int)d->nmcd.dwItemSpec,col=d->iSubItem;d->clrText=INK;if(n->idFrom==TRACE&&row<shown&&col==3)d->clrText=trace[row].fault?RED:GREEN;if(n->idFrom==SIDE&&row<cshown&&(col==3||col==5))d->clrText=(col==3?lefttrace[row].fault:righttrace[row].fault)?RED:GREEN;return CDRF_NEWFONT;}}if(n->idFrom==TRACE&&n->code==LVN_ITEMCHANGED){NMLISTVIEW*v=(NMLISTVIEW*)l;if(v->iItem>=0&&v->iItem<shown&&(v->uNewState&LVIS_SELECTED)){wchar_t b[500];wide(trace[v->iItem].reason,b,500);text(REASON,b);}}return 0;}
 case WM_PAINT:case WM_PRINTCLIENT:{PAINTSTRUCT ps;HDC dc=m==WM_PAINT?BeginPaint(h,&ps):(HDC)w;RECT r;GetClientRect(h,&r);FillRect(dc,&r,white);SetBkMode(dc,TRANSPARENT);SelectObject(dc,titlefont);SetTextColor(dc,INK);RECT title={px(32),px(20),r.right-px(32),px(59)};DrawTextW(dc,L"Memory Policy Lab",-1,&title,DT_LEFT|DT_SINGLELINE|DT_VCENTER);SelectObject(dc,smallfont);SetTextColor(dc,MUTED);RECT sub={r.right-px(310),px(26),r.right-px(32),px(56)};DrawTextW(dc,L"FIFO  /  LRU  /  OPTIMAL",-1,&sub,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);HPEN pen=CreatePen(PS_SOLID,1,LINE);HGDIOBJ old=SelectObject(dc,pen);MoveToEx(dc,px(32),px(255),NULL);LineTo(dc,r.right-px(32),px(255));MoveToEx(dc,px(32),r.bottom-px(45),NULL);LineTo(dc,r.right-px(32),r.bottom-px(45));SelectObject(dc,old);DeleteObject(pen);if(m==WM_PAINT)EndPaint(h,&ps);return 0;}
 case DONE:finish_search();return 0;
 case STARTTEST:smoke_test();return 0;
 case WM_CLOSE:if(dirty&&!smoke){int answer=MessageBoxW(h,L"บันทึกชุดข้อมูลจากการค้นหาก่อนออกหรือไม่?",L"ออกจาก Memory Policy Lab",MB_YESNOCANCEL|MB_ICONQUESTION);if(answer==IDCANCEL)return 0;if(answer==IDYES){save_file();if(dirty)return 0;}}DestroyWindow(h);return 0;
 case WM_DESTROY:if(worker){WaitForSingleObject(worker,INFINITE);CloseHandle(worker);worker=NULL;}if(refwin)DestroyWindow(refwin);PostQuitMessage(test_ok?0:1);return 0;
 default:return DefWindowProcW(h,m,w,l);}}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE prev,PWSTR cmd,int show){(void)prev;instance=hi;SetProcessDPIAware();HDC dc=GetDC(NULL);scale=GetDeviceCaps(dc,LOGPIXELSX)/96.f;ReleaseDC(NULL,dc);INITCOMMONCONTROLSEX ic={sizeof(ic),ICC_LISTVIEW_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&ic);white=CreateSolidBrush(RGB(255,255,255));soft=CreateSolidBrush(SOFT);font=CreateFontW(-px(16),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");titlefont=CreateFontW(-px(25),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");smallfont=CreateFontW(-px(13),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 GetModuleFileNameW(NULL,rootdir,MAX_PATH);wchar_t*slash=wcsrchr(rootdir,L'\\');if(slash)*slash=0;swprintf(resultsdir,MAX_PATH,L"%ls\\results",rootdir);CreateDirectoryW(resultsdir,NULL);
 if(wcsncmp(cmd,L"--smoke-test ",13)==0){smoke=1;wcsncpy(smokedir,cmd+13,MAX_PATH-1);size_t n=wcslen(smokedir);if(n>=2&&smokedir[0]=='"'&&smokedir[n-1]=='"'){memmove(smokedir,smokedir+1,(n-2)*sizeof(wchar_t));smokedir[n-2]=0;}}
 WNDCLASSW wc={0};wc.lpfnWndProc=WndProc;wc.hInstance=hi;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=white;wc.lpszClassName=L"MemoryPolicyLabUI";wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);RegisterClassW(&wc);wc.lpfnWndProc=RefProc;wc.lpszClassName=L"MPLReferences";RegisterClassW(&wc);
 HWND h=CreateWindowExW(WS_EX_CONTROLPARENT,L"MemoryPolicyLabUI",L"Memory Policy Lab",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,px(1140),px(920),NULL,NULL,hi,NULL);if(!h)return 1;ShowWindow(h,show);UpdateWindow(h);if(smoke)PostMessageW(h,STARTTEST,0,0);MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){if(!IsDialogMessageW(h,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}DeleteObject(font);DeleteObject(titlefont);DeleteObject(smallfont);DeleteObject(white);DeleteObject(soft);return (int)msg.wParam;
}

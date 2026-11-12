/* I10: operator-owned WAV intake; Sound Framework owns clips and voices. */
#include "base.h"
#include "../sound-framework/sdk/client.h"
#include "input.h"
#include "../../runtime/native/local_actor.h"
#define HORN_EMITTER 0x726f766572686f72ULL
static struct {
 BOOL (*read)(PTR,void *,DWORD,DWORD *,PTR),(*close)(PTR),(*size)(PTR,U64 *),(*seek)(PTR,long long,long long *,DWORD);
 DWORD (*profile_string)(const char *,const char *,const char *,char *,DWORD,const char *);unsigned (*profile_int)(const char *,const char *,int,const char *);
 PTR driver,driven,transform,position,component,notify;
 DRoot world,vehicle;PTR file;DWORD rate,frames,loaded;unsigned asset;
 int configured,vk,ready,failed,retiring,failure_notified;float gain,next_press,last_time;
 char wav[1024],ini[1024],loading_status[160];const char *status;short pcm[96000*3];
 const MoonSounds *sounds;const MoonSoundAssets *assets;
} horn;
#include "wav.h"
static void notice(PTR walker,const char *message){int ok=1;void *ca[]={dtype("MoonCleaners","MoonPrototypeEquipment")};PTR equipment=call(horn.component,walker,ca,&ok);if(equipment){void *a[]={da.string(message)};call(horn.notify,equipment,a,&ok);}event(message);}
static void configure(void){
 if(horn.configured)return;horn.configured=1;
 DWORD n=GetEnvironmentVariableA("MOONLAB_HORN_SETTINGS",horn.ini,sizeof(horn.ini));if(!n||n>=sizeof(horn.ini))cat(horn.ini,".\\MoonLab\\mods\\rover-horns\\settings.ini");
 n=GetEnvironmentVariableA("MOONLAB_HORN_WAV",horn.wav,sizeof(horn.wav));if(!n||n>=sizeof(horn.wav))horn.profile_string("Horn","Wav",".\\MoonLab\\mods\\rover-horns\\horn.wav",horn.wav,sizeof(horn.wav),horn.ini);
 unsigned vk=horn.profile_int("Horn","Key",72,horn.ini),volume=horn.profile_int("Horn","VolumePercent",65,horn.ini);horn.vk=vk>0&&vk<=255?(int)vk:72;horn.gain=(float)((volume&0x80000000u)?0:volume>100?100:volume)/100;
}
static const char *loading_notice(void){
 char label[64];if(!rh_controls_label((unsigned)horn.vk,label,sizeof(label)))cat(label,"horn");
 char *p=cat(horn.loading_status,"Rover Horns: loading operator audio; press ");p=cat(p,label);cat(p," again when ready.");return horn.loading_status;
}
#include "lifecycle.h"
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
 if(!base_bind(domain,game,"rover-horns.jsonl")||!moon_local_actor_bind())return 0;
 horn.sounds=ma_connect(domain,game);horn.assets=ma_connect_assets(domain,game);if(!horn.sounds||!horn.assets)return 0;
 PTR kernel=GetModuleHandleA("kernel32.dll");
#define W(F,N,T) horn.F=(T)GetProcAddress(kernel,N);if(!horn.F)return 0
 W(read,"ReadFile",BOOL(*)(PTR,void*,DWORD,DWORD*,PTR));W(close,"CloseHandle",BOOL(*)(PTR));W(size,"GetFileSizeEx",BOOL(*)(PTR,U64*));W(seek,"SetFilePointerEx",BOOL(*)(PTR,long long,long long*,DWORD));W(profile_string,"GetPrivateProfileStringA",DWORD(*)(const char*,const char*,const char*,char*,DWORD,const char*));W(profile_int,"GetPrivateProfileIntA",unsigned(*)(const char*,const char*,int,const char*));
#undef W
#define B(F,NS,T,N,S) horn.F=dmethod(NS,T,N,S);if(!horn.F)return 0
 B(driver,"MoonCleaners","MoonPrototypeVehicle","get_Owner","");B(driven,"MoonCleaners","MoonPrototypeVehicle","get_IsDriven","");B(transform,"UnityEngine","Component","get_transform","");B(position,"UnityEngine","Transform","get_position","");B(component,"UnityEngine","Component","GetComponent","System.Type");B(notify,"MoonCleaners","MoonPrototypeEquipment","Notify","System.String");
#undef B
 if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;before_hook=before;event("ready-awaiting-operator-wav");return install_hooks();
}

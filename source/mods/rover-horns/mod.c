/* I10: operator-owned WAV intake; Sound Framework owns clips and voices. */
#include "base.h"
#include "../sound-framework/sdk/client.h"
#include "input.h"
#define HORN_EMITTER 0x726f766572686f72ULL
static struct {
 BOOL (*read)(PTR,void *,DWORD,DWORD *,PTR),(*close)(PTR),(*size)(PTR,U64 *),(*seek)(PTR,long long,long long *,DWORD);
 DWORD (*profile_string)(const char *,const char *,const char *,char *,DWORD,const char *);unsigned (*profile_int)(const char *,const char *,int,const char *);
 PTR driver,driven,transform,position,component,notify;
 DRoot vehicle;PTR file;DWORD rate,frames,loaded;unsigned asset;
 int configured,vk,was_down,ready,failed;float gain,next_press;
 char wav[1024],ini[1024],loading_status[160];const char *status;short pcm[96000*3];
 const MoonSounds *sounds;const MoonSoundAssets *assets;
} horn;
#include "wav.h"
static void notice(PTR walker,const char *message){int ok=1;void *ca[]={dtype("MoonCleaners","MoonPrototypeEquipment")};PTR equipment=call(horn.component,walker,ca,&ok);if(equipment){void *a[]={da.string(message)};call(horn.notify,equipment,a,&ok);}event(message);}
static int drop(void){
 if(horn.file)horn.close(horn.file);horn.file=0;
 if(horn.asset){if(!horn.assets->release(horn.asset))return 0;}
 else horn.sounds->stop(HORN_EMITTER);
 horn.asset=0;ddrop(&horn.vehicle);horn.ready=horn.loaded=horn.frames=horn.failed=0;return 1;
}
static void configure(void){
 if(horn.configured)return;horn.configured=1;
 DWORD n=GetEnvironmentVariableA("MOONLAB_HORN_SETTINGS",horn.ini,sizeof(horn.ini));if(!n||n>=sizeof(horn.ini))cat(horn.ini,".\\MoonLab\\mods\\rover-horns\\settings.ini");
 n=GetEnvironmentVariableA("MOONLAB_HORN_WAV",horn.wav,sizeof(horn.wav));if(!n||n>=sizeof(horn.wav))horn.profile_string("Horn","Wav",".\\MoonLab\\mods\\rover-horns\\horn.wav",horn.wav,sizeof(horn.wav),horn.ini);
 unsigned vk=horn.profile_int("Horn","Key",72,horn.ini),volume=horn.profile_int("Horn","VolumePercent",65,horn.ini);horn.vk=vk>0&&vk<=255?(int)vk:72;horn.gain=(float)(volume>100?100:volume)/100;
}
static const char *loading_notice(void){
 char label[64];if(!rh_controls_label((unsigned)horn.vk,label,sizeof(label)))cat(label,"horn");
 char *p=cat(horn.loading_status,"Rover Horns: loading operator audio; press ");p=cat(p,label);cat(p," again when ready.");return horn.loading_status;
}
static int begin(PTR vehicle){
 horn.vehicle=dhold(vehicle);if(!horn.vehicle||!wav_open())return 0;
 horn.status=loading_notice();return 1;
}
static void load_tick(void){
 if(!horn.file||horn.ready)return;DWORD count=horn.frames-horn.loaded;if(count>8192)count=8192;
 if(!read_exact(horn.pcm+horn.loaded,count*2)){horn.failed=1;horn.status="Rover Horns: WAV read failed.";return;}
 horn.loaded+=count;
 if(horn.loaded==horn.frames){
  horn.close(horn.file);horn.file=0;
  MoonSoundCue cue={MA_ABI,sizeof(cue),"rover-horns.operator",horn.pcm,horn.frames,horn.rate,horn.gain,3,45,.5f};
  horn.asset=horn.assets->register_pcm(&cue,1);horn.ready=horn.asset!=0;horn.failed=!horn.ready;
  horn.status=horn.ready?"Rover Horns: operator sound ready.":"Rover Horns: sound registration failed.";event(horn.status);
 }
}
static int before(int id,Context *c){
 (void)id;if(!on_thread())return 0;configure();PTR walker=(PTR)c->rcx,world=0,vehicle=0;int ok=1,active=input_ready(walker,&world,&vehicle,0),pressed_now=0;float now=dnumber(time_m,0,&ok);int honk=rh_controls_honk((unsigned)horn.vk);
 if(active){PTR driver=call(horn.driver,vehicle,0,&ok);active=ok&&driver==walker&&boolean(horn.driven,vehicle,&ok)&&ok;pressed_now=active&&honk;}
 if(!active||vehicle!=dget(horn.vehicle)){if(horn.vehicle&&!drop()){horn.was_down=pressed_now;return 0;}}
 if(active&&horn.vehicle){
  PTR tr=call(horn.transform,vehicle,0,&ok);DVec p=dvector(horn.position,tr,&ok);float position[3]={p.x,p.y,p.z};
  if(ok)horn.sounds->move(HORN_EMITTER,position);
 }
 if(active&&pressed_now&&!horn.was_down&&now>=horn.next_press){horn.next_press=now+.5f;
  if(!horn.vehicle){if(!begin(vehicle)){const char *reason=horn.status;drop();notice(walker,reason?reason:"Rover Horns: audio initialization failed.");}}
  else if(horn.ready){
   if(!horn.assets->playing(HORN_EMITTER)){
    PTR tr=call(horn.transform,vehicle,0,&ok);DVec p=dvector(horn.position,tr,&ok);float position[3]={p.x,p.y,p.z};
    int played=ok&&horn.sounds->play(horn.asset,HORN_EMITTER,position,1,1);event(played?"horn-play":"horn-play-failed");
    if(!played)notice(walker,"Rover Horns: sound playback unavailable; try again.");
   }
  }else notice(walker,horn.status==horn.loading_status?loading_notice():horn.status?horn.status:"Rover Horns: operator audio is not ready.");
 }
 if(active&&horn.vehicle&&!horn.failed)load_tick();if(horn.failed){const char *reason=horn.status;drop();notice(walker,reason?reason:"Rover Horns: loading failed.");}
 horn.was_down=pressed_now;return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
 if(!base_bind(domain,game,"rover-horns.jsonl"))return 0;
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

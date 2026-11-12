#ifndef MOON_ROVER_HORN_WAV_H
#define MOON_ROVER_HORN_WAV_H
/* Operator-owned input only; bounded exact RIFF/mono PCM16 parser. */
static unsigned word(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static DWORD little(const unsigned char *p){return (DWORD)p[0]|(DWORD)p[1]<<8|(DWORD)p[2]<<16|(DWORD)p[3]<<24;}
static int tag(const unsigned char *p,const char *s){return p[0]==s[0]&&p[1]==s[1]&&p[2]==s[2]&&p[3]==s[3];}
static int read_exact(void *p,DWORD n){DWORD got=0;return horn.read(horn.file,p,n,&got,0)&&got==n;}
static int wav_open(void){
 horn.rate=horn.frames=0;
 horn.file=CreateFileA(horn.wav,0x80000000,1,0,3,0x80,0);if(horn.file==(PTR)(U64)-1){horn.file=0;horn.status="Rover Horns: operator horn.wav is missing. Supply the Omni sound first.";return 0;}
 U64 size=0;unsigned char h[64];if(!horn.size(horn.file,&size)||size<44||size>2000000||!read_exact(h,12)||!tag(h,"RIFF")||!tag(h+8,"WAVE")||(U64)little(h+4)+8!=size){horn.status="Rover Horns: invalid or oversized RIFF WAV.";return 0;}
 U64 offset=12,data=0;DWORD rate=0,frames=0;int format=0;
 for(unsigned chunk=0;chunk<32&&offset+8<=size;chunk++){
  if(!read_exact(h,8))break;DWORD n=little(h+4);offset+=8;if(offset+(U64)n+(n&1)>size)break;
  if(tag(h,"fmt ")){
   if(format||n<16||n>64||!read_exact(h,n))break;
   rate=little(h+4);if(word(h)!=1||word(h+2)!=1||word(h+14)!=16||word(h+12)!=2||rate<8000||rate>96000||little(h+8)!=rate*2)break;
   format=1;
  }else if(tag(h,"data")){
   if(data||!format||!n||(n&1)||n/2>rate*3)break;data=offset;frames=n/2;
  }
  offset+=(U64)n+(n&1);if(offset>size||!horn.seek(horn.file,(long long)offset,0,0))break;
  /* Validate the entire bounded container before returning to its PCM payload.
   * Otherwise malformed trailing chunks bypass both RIFF and chunk-count gates. */
  if(offset==size&&data&&horn.seek(horn.file,(long long)data,0,0)){horn.rate=rate;horn.frames=frames;return 1;}
 }
 horn.status="Rover Horns: use mono PCM16 WAV, 8-96 kHz, no longer than 3 seconds.";return 0;
}
#endif

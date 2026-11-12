#ifndef MOON_ROVER_HORN_LIFECYCLE_H
#define MOON_ROVER_HORN_LIFECYCLE_H
/* Keep PCM and roots resident until Sound acknowledges release. A failed close
 * or release is retried before input, reads or PCM replacement can resume. */
static int drop(void){
 horn.retiring=1;
 if(horn.file){if(!horn.close(horn.file))return 0;horn.file=0;}
 if(horn.asset){if(!horn.assets->release(horn.asset))return 0;}
 else horn.sounds->stop(HORN_EMITTER);
 horn.asset=0;ddrop(&horn.vehicle);horn.ready=horn.loaded=horn.frames=horn.failed=horn.failure_notified=horn.retiring=0;return 1;
}
static int begin(PTR vehicle){
 horn.status=0;horn.vehicle=dhold(vehicle);if(!horn.vehicle||!wav_open())return 0;
 horn.status=loading_notice();return 1;
}
static void load_tick(void){
 if(!horn.file||horn.ready)return;DWORD count=horn.frames-horn.loaded;if(count>8192)count=8192;
 if(!read_exact(horn.pcm+horn.loaded,count*2)){horn.failed=1;horn.status="Rover Horns: WAV read failed.";return;}
 horn.loaded+=count;
 if(horn.loaded==horn.frames){
  if(!horn.close(horn.file)){horn.failed=1;horn.status="Rover Horns: WAV close failed.";return;}horn.file=0;
  MoonSoundCue cue={MA_ABI,sizeof(cue),"rover-horns.operator",horn.pcm,horn.frames,horn.rate,horn.gain,3,45,.5f};
  horn.asset=horn.assets->register_pcm(&cue,1);horn.ready=horn.asset!=0;horn.failed=!horn.ready;
  horn.status=horn.ready?"Rover Horns: operator sound ready.":"Rover Horns: sound registration failed.";event(horn.status);
 }
}
static int position(PTR vehicle,float out[3]){
 int ok=1;PTR tr=call(horn.transform,vehicle,0,&ok);if(!ok||!alive(tr))return 0;
 DVec p=vehicle_read_vector(horn.position,tr,&ok);if(!ok)return 0;
 out[0]=p.x;out[1]=p.y;out[2]=p.z;
 for(unsigned i=0;i<3;i++)if(!(out[i]>=-10000000&&out[i]<=10000000))return 0;
 return 1;
}
static int before(int id,Context *c){
 (void)id;if(!on_thread())return 0;configure();PTR walker=(PTR)c->rcx,local=0;
 int local_ok=moon_local_actor_read(&local);
 /* Peer updates cannot sample our edges or retire a local driver's emitter. */
 if(local_ok&&local&&local!=walker)return 0;
 if(!local_ok||!local){rh_controls_press((unsigned)horn.vk,0);if(drop()){ddrop(&horn.world);horn.next_press=horn.last_time=0;}return 0;}
 PTR world=0,vehicle=0;int ok=1,active=input_ready(walker,&world,&vehicle,0);
 if(!alive(world))world=0;
 if(world!=dget(horn.world)){
  rh_controls_press((unsigned)horn.vk,0);if(!drop())return 0;
  ddrop(&horn.world);horn.next_press=horn.last_time=0;horn.world=dhold(world);return 0;
 }
 if(horn.retiring){rh_controls_press((unsigned)horn.vk,0);drop();return 0;}
 float now=vehicle_read_number(time_m,0,&ok),press_until=0;
 if(!ok||!vehicle_read_deadline(now,.5f,&press_until))active=0;
 else {if(now<horn.last_time)horn.next_press=0;horn.last_time=now;}
 if(active){PTR driver=call(horn.driver,vehicle,0,&ok);active=ok&&driver==walker&&vehicle_read_boolean(horn.driven,vehicle,&ok)&&ok;}
 int changed=horn.vehicle&&vehicle!=dget(horn.vehicle);
 int trigger=rh_controls_press((unsigned)horn.vk,active&&!changed);
 if(!active||changed){if(horn.vehicle)drop();return 0;}
 float point[3];if(!position(vehicle,point)){drop();return 0;}
 if(horn.vehicle)horn.sounds->move(HORN_EMITTER,point);
 if(trigger&&now>=horn.next_press){horn.next_press=press_until;
  if(!horn.vehicle){if(!begin(vehicle)){const char *reason=horn.status;drop();notice(walker,reason?reason:"Rover Horns: audio initialization failed.");}}
  else if(horn.ready){
   if(!horn.assets->playing(HORN_EMITTER)){
    int played=horn.sounds->play(horn.asset,HORN_EMITTER,point,1,1);event(played?"horn-play":"horn-play-failed");
    if(!played)notice(walker,"Rover Horns: sound playback unavailable; try again.");
   }
  }else notice(walker,horn.status==horn.loading_status?loading_notice():horn.status?horn.status:"Rover Horns: operator audio is not ready.");
 }
 if(horn.retiring)return 0;
 if(horn.vehicle&&!horn.failed)load_tick();
 if(horn.failed){if(!horn.failure_notified){notice(walker,horn.status?horn.status:"Rover Horns: loading failed.");horn.failure_notified=1;}drop();}
 return 0;
}
#endif

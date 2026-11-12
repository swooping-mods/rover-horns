#ifndef ROVER_HORNS_INPUT_H
#define ROVER_HORNS_INPUT_H
#include "../../runtime/native/input.h"
/* Existing operator INI virtual keys seed the named action's default. Custom
 * Keybinds can override it without changing driver/gate/cooldown semantics. */
static inline int rh_controls_honk(unsigned key){return moon_input_key_action("rover-horns.honk",key,1).held;}
static inline int rh_controls_press(unsigned key,int allowed){return moon_input_key_action("rover-horns.honk",key,allowed).pressed;}
static inline int rh_controls_label(unsigned key,char *out,unsigned cap){return moon_input_key_label("rover-horns.honk",key,out,cap);}
#endif

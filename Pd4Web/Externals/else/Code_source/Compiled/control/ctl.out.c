// porres 2018

#include "m_pd.h"
#include "s_stuff.h"

typedef struct _ctlout{
    t_object  x_ob;
    t_float   x_channel;
    t_float   x_n;
    t_int     x_ext;
}t_ctlout;

static t_class *ctlout_class;

void ctlout_midiout(int value){
#ifdef USEAPI_ALSA
  if(sys_midiapi == API_ALSA)
      sys_alsa_putmidibyte(0, value);
  else
#endif
    sys_putmidibyte(0, value);
}

static void ctlout_output(t_ctlout *x, t_float f){
    outlet_float(((t_object *)x)->ob_outlet, f);
    if(!x->x_ext)
        ctlout_midiout(f);
}

static void ctlout_ext(t_ctlout *x, t_floatarg f){
    x->x_ext = f != 0;
}

static void ctlout_float(t_ctlout *x, t_float f){
    if(f >= 0 && f <= 127){
        t_int channel = (int)x->x_channel;
        if(channel <= 0)
            channel = 1;
        if(channel > 16)
            channel = 16;
        if(x->x_n <= 0)
            x->x_n = 0;
        if(x->x_n > 127)
            x->x_n = 127;
        ctlout_output(x, 176 + ((channel-1) & 0x0F));
        ctlout_output(x,  (t_int)x->x_n);
        ctlout_output(x, (t_int)f);
    }
}

static void *ctlout_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_ctlout *x = (t_ctlout *)pd_new(ctlout_class);
    x->x_channel = 1,
    x->x_n = 0;
    x->x_ext = 0;
    if(ac){
        if(atom_getsymbol(av) == gensym("-ext")){
            x->x_ext = 1;
            ac--, av++;
        }
        if(ac == 1){
            if(av->a_type == A_FLOAT)
                x->x_channel = (t_int)atom_getfloatarg(0, ac, av);
        }
        else if(ac == 2){
            x->x_n = atom_getfloatarg(0, ac, av);
            ac--, av++;
            x->x_channel = (t_int)atom_getfloatarg(0, ac, av);
        }
    }
    floatinlet_new((t_object *)x, &x->x_n);
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    return(x);
}

void setup_ctl0x2eout(void){
    ctlout_class = class_new(gensym("ctl.out"), (t_newmethod)ctlout_new,
        0, sizeof(t_ctlout), 0, A_GIMME, 0);
    class_addfloat(ctlout_class, ctlout_float);
    class_addmethod(ctlout_class, (t_method)ctlout_ext, gensym("ext"), A_FLOAT, 0);
}

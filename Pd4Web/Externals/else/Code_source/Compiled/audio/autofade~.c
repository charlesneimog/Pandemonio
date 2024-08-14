// porres 2017-2024

#include "m_pd.h"
#include "buffer.h"

#define MAX_INOUT 4096

typedef struct _autofade{
    t_object  x_obj;
    int       x_n;
    int       x_control_gate;
    int       x_lastkgate;
    int       x_n_chans;
    int       x_fade;                        // fade size in samples
    int       x_nleft;                       // n samples left
    float     x_ksr, x_fade_ms, x_inc;       // sr in khz, fade size in ms and inc step
    t_float **x_ins, **x_outs;               // inputs and outputs
    t_float  *x_inputs;                      // inputs copy
    t_float  *x_gate;                        // gate
    t_int     x_gate_on;                     // gate status
    t_float   x_lastgate, x_lastfade, x_start, x_end, x_delta, x_phase;
    t_int     x_table;
}t_autofade;

static t_class *autofade_class;

static void autofade_float(t_autofade *x, t_floatarg f){
    x->x_control_gate = f != 0;
}

static void autofade_fade(t_autofade *x, t_floatarg f){
    x->x_fade_ms = f < 0 ? 0 : f;
    x->x_fade = (int)((x->x_fade_ms * x->x_ksr) + 0.5);
}

static void autofade_quartic(t_autofade *x){
    x->x_table = 0;
}

static void autofade_lin(t_autofade *x){
    x->x_table = 1;
}

static void autofade_linsin(t_autofade *x){
    x->x_table = 2;
}

static void autofade_sqrt(t_autofade *x){
    x->x_table = 3;
}

static void autofade_sin(t_autofade *x){
    x->x_table = 4;
}

static void autofade_hannsin(t_autofade *x){
    x->x_table = 5;
}

static void autofade_hann(t_autofade *x){
    x->x_table = 6;
}

static t_int *autofade_perform(t_int *w){
    t_autofade *x = (t_autofade *)(w[1]);
    t_float *ins = x->x_inputs;
    int i;
    for(int j = 0; j < x->x_n_chans; j++){ // copy input
        for(i = 0; i < x->x_n; i++)
            ins[j*x->x_n+i] = x->x_ins[j][i];
    }
    t_float *gate = x->x_gate;
    t_float lastgate = x->x_lastgate;
    t_float lastkgate = x->x_lastkgate;
    t_float lastfade = x->x_lastfade;
    t_float phase = x->x_phase;
    for(int n = 0; n < x->x_n; n++){ // n sample in block
        if((gate[n] <= 0 && lastgate > 0)
           || (x->x_control_gate == 0 && lastkgate == 1)
           || (gate[n] > 0 && lastgate <= 0)
           || (x->x_control_gate == 1 && lastkgate == 0)
           ){
            x->x_nleft = x->x_fade;
            float inc = x->x_nleft > 0 ? 1./x->x_fade : 0;
            x->x_start = lastfade;
            x->x_end = x->x_control_gate ? x->x_control_gate : gate[n];
            x->x_delta = x->x_end - x->x_start;
            x->x_gate_on = gate[n] > 0 || x->x_control_gate;
//            x->x_gate_on = gate[n] > 0;
            if(x->x_gate_on){ // gate on
                phase = 0.;
                x->x_inc = inc;
            }
            else{ // gate off
                phase = 1.;
                x->x_inc = -inc;
            }
        }
        lastgate = gate[n];
        lastkgate = x->x_control_gate;
        float fadeval = read_fadetab(phase, x->x_table);
//        float fadeval = phase;
        lastfade = fadeval;
//        post("fadeval = %g", fadeval);
        if(x->x_nleft > 0){
            phase += x->x_inc;
            x->x_nleft--;
        }
        else
            phase = x->x_gate_on;
        float finalfade;
        if(x->x_gate_on)
            finalfade = x->x_start + (fadeval * x->x_delta);
        else // gate off
            finalfade = fadeval * x->x_start;
        for(i = 0; i < x->x_n_chans; i++)
            x->x_outs[i][n] = ins[i*x->x_n + n] * finalfade;
    }
    x->x_lastgate = lastgate;
    x->x_lastkgate = lastkgate;
    x->x_lastfade = lastfade;
    x->x_phase = phase;
    return(w+2);
}

static void autofade_dsp(t_autofade *x, t_signal **sp){
    t_signal **sigp = sp;
    int i, n = sp[0]->s_n;
    for(i = 0; i < x->x_n_chans; i++)       // 'n' inlets
        *(x->x_ins+i) = (*sigp++)->s_vec;
    x->x_gate = (*sigp++)->s_vec;           // gate inlet
    for(i = 0; i < x->x_n_chans; i++)       // 'n' outlets
        *(x->x_outs+i) = (*sigp++)->s_vec;
    float ksr = sp[0]->s_sr * .001;
    if(ksr != x->x_ksr){
        x->x_ksr = ksr;
        autofade_fade(x, x->x_fade_ms);
    }
    if(n != x->x_n){
        x->x_inputs = (t_float *)resizebytes(x->x_inputs,
            x->x_n*x->x_n_chans * sizeof(t_float), n*x->x_n_chans * sizeof(t_float));
        x->x_n = n;
    }
    dsp_add(autofade_perform, 1, x);
}

static void *autofade_free(t_autofade *x){
    freebytes(x->x_inputs, x->x_n*x->x_n_chans*sizeof(*x->x_inputs));
    freebytes(x->x_ins, x->x_n_chans * sizeof(*x->x_ins));
    freebytes(x->x_outs, x->x_n_chans * sizeof(*x->x_outs));
    return(void *)x;
}

static void *autofade_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_autofade *x = (t_autofade *)pd_new(autofade_class);
    init_fade_tables();
    t_float fadems = 10.;
    x->x_n_chans = 1;
    x->x_ksr = sys_getsr() * .001;
    x->x_n = sys_getblksize();
    x->x_table = 0; // quartic
    if(ac && av->a_type == A_SYMBOL){ // optional 1st symbol arg
        t_symbol *curarg = atom_getsymbol(av);
        if(curarg == gensym("quartic"))
            x->x_table = 0;
        else if(curarg == gensym("lin"))
            x->x_table = 1;
        else if(curarg == gensym("linsin"))
            x->x_table = 2;
        else if(curarg == gensym("sqrt"))
            x->x_table = 3;
        else if(s == gensym("sin"))
            x->x_table = 4;
        else if(curarg == gensym("hannsin"))
            x->x_table = 5;
        else if(curarg == gensym("hann"))
            x->x_table = 6;
        ac--, av++;
    }
    if(ac){
        fadems = atom_getfloat(av);
        ac--, av++;
    }
    if(ac){
        int ch = atom_getint(av);
        x->x_n_chans = ch < 1 ? 1 : ch > MAX_INOUT ? MAX_INOUT : ch;
        ac--, av++;
    }
    x->x_ins = getbytes(x->x_n_chans * sizeof(*x->x_ins));
    x->x_outs = getbytes(x->x_n_chans * sizeof(*x->x_outs));
    x->x_inputs = (t_float *)getbytes(x->x_n*x->x_n_chans*sizeof(*x->x_inputs));
    x->x_end = x->x_nleft = x->x_inc = x->x_lastgate = 0.;
    autofade_fade(x, fadems);
    int i;
    for(i = 0; i < x->x_n_chans; i++)
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    for(i = 0; i < x->x_n_chans; i++)
        outlet_new(&x->x_obj, gensym("signal"));
    return(x);
}

void autofade_tilde_setup(void){
    autofade_class = class_new(gensym("autofade~"), (t_newmethod)autofade_new,
        (t_method)autofade_free, sizeof(t_autofade), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(autofade_class, nullfn, gensym("signal"), 0);
    class_addmethod(autofade_class, (t_method)autofade_dsp, gensym("dsp"), A_CANT, 0);
    class_addfloat(autofade_class, autofade_float); // undocumented feature for rec.file~
    class_addmethod(autofade_class, (t_method)autofade_fade, gensym("fade"), A_FLOAT, 0);
    class_addmethod(autofade_class, (t_method)autofade_quartic, gensym("quartic"), 0);
    class_addmethod(autofade_class, (t_method)autofade_lin, gensym("lin"), 0);
    class_addmethod(autofade_class, (t_method)autofade_sqrt, gensym("sqrt"), 0);
    class_addmethod(autofade_class, (t_method)autofade_sin, gensym("sin"), 0);
    class_addmethod(autofade_class, (t_method)autofade_hann, gensym("hann"), 0);
    class_addmethod(autofade_class, (t_method)autofade_linsin, gensym("linsin"), 0);
    class_addmethod(autofade_class, (t_method)autofade_hannsin, gensym("hannsin"), 0);
}

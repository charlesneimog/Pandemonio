// Porres 2017-2023

#include "m_pd.h"
#include "magic.h"
#include "buffer.h"

typedef struct _cosine{
    t_object    x_obj;
    double     *x_phase;
    int         x_nchans;
    t_int       midi;
    t_int       soft;
    t_float     x_freq;
    t_inlet    *x_inlet_phase;
    t_inlet    *x_inlet_sync;
    t_outlet   *x_outlet;
    double      x_sr_rec;
// MAGIC:
    t_glist    *x_glist; // object list
    t_float    *x_signalscalar; // right inlet's float field
    int         x_hasfeeders; // right inlet connection flag
    t_float     x_phase_sync_float; // float from magic
}t_cosine;

static t_class *cosine_class;

double cosine_wrap_phase(double phase){
    while(phase >= 1)
        phase -= 1.;
    while(phase < 0)
        phase += 1.;
    return(phase);
}

static t_int *cosine_perform(t_int *w){
    t_cosine *x = (t_cosine *)(w[1]);
    int n = (int)(w[2]);
    int ch3 = (int)(w[3]);
    t_float *in1 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
// Magic Start
    t_float *scalar = x->x_signalscalar;
    if(!else_magic_isnan(*x->x_signalscalar)){
        t_float input_phase = fmod(*scalar, 1);
        if(input_phase < 0)
            input_phase += 1;
        for(int j = 0; j < x->x_nchans; j++)
            x->x_phase[j] = input_phase;
        else_magic_setnan(x->x_signalscalar);
    }
// Magic End
    double *phase = x->x_phase;
    for(int j = 0; j < x->x_nchans; j++){
        for(int i = 0; i < n; i++){
            double hz = in1[j*n + i];
            if(x->midi)
                hz = hz <= 0 ? 0 : pow(2, (hz - 69)/12) * 440;
            double phase_step = hz * x->x_sr_rec; // phase_step
            double phase_offset = ch3 == 1 ? in3[i] : in3[j*n + i];
            out[j*n + i] = read_sintab(cosine_wrap_phase(phase[j] + .25 + phase_offset));
            phase[j] = cosine_wrap_phase(phase[j] + phase_step);
        }
    }
    x->x_phase = phase;
    return(w+7);
}

static t_int *cosine_perform_sig(t_int *w){
    t_cosine *x = (t_cosine *)(w[1]);
    int n = (int)(w[2]);
    int ch2 = (int)(w[3]);
    int ch3 = (int)(w[4]);
    t_float *in1 = (t_float *)(w[5]);
    t_float *in2 = (t_float *)(w[6]);
    t_float *in3 = (t_float *)(w[7]);
    t_float *out = (t_float *)(w[8]);
    double *phase = x->x_phase;
    for(int j = 0; j < x->x_nchans; j++){
        for(int i = 0; i < n; i++){
            double hz = in1[j*n + i];
            if(x->midi)
                hz = hz <= 0 ? 0 : pow(2, (hz - 69)/12) * 440;
            double phase_step = hz * x->x_sr_rec; // phase_step
            t_float trig = ch2 == 1 ? in2[i] : in2[j*n + i];
            double phase_offset = ch3 == 1 ? in3[i] : in3[j*n + i];
            if(x->soft)
                phase_step *= (x->soft);
            if(trig > 0 && trig <= 1){
                if(x->soft)
                    x->soft = x->soft == 1 ? -1 : 1;
                else
                    phase[j] = trig;
            }
            else
                phase[j] = cosine_wrap_phase(phase[j] + .25 + phase_offset);
            out[j*n + i] = read_sintab(phase[j]);
            phase[j] = cosine_wrap_phase(phase[j] + phase_step);
        }
    }
    x->x_phase = phase;
    return(w+9);
}

static void cosine_dsp(t_cosine *x, t_signal **sp){
    x->x_sr_rec = 1.0 / (double)sp[0]->s_sr;
    int chs = sp[0]->s_nchans, ch2 = sp[1]->s_nchans, ch3 = sp[2]->s_nchans, n = sp[0]->s_n;
    signal_setmultiout(&sp[3], chs);
    if(x->x_nchans != chs){
        x->x_phase = (double *)resizebytes(x->x_phase,
            x->x_nchans * sizeof(double), chs * sizeof(double));
        x->x_nchans = chs;
    }
    if((ch2 > 1 && ch2 != x->x_nchans) || (ch3 > 1 && ch3 != x->x_nchans)){
        dsp_add_zero(sp[3]->s_vec, chs*n);
        pd_error(x, "[cosine~]: channel sizes mismatch");
        return;
    }
    x->x_hasfeeders = else_magic_inlet_connection((t_object *)x, x->x_glist, 1, &s_signal); // magic feeder flag
    if(x->x_hasfeeders){
        dsp_add(cosine_perform_sig, 8, x, n, ch2, ch3,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
    else
        dsp_add(cosine_perform, 6, x, n, ch3, sp[0]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void cosine_midi(t_cosine *x, t_floatarg f){
    x->midi = (int)(f != 0);
}

static void cosine_soft(t_cosine *x, t_floatarg f){
    x->soft = (int)(f != 0);
}

static void *cosine_free(t_cosine *x){
    inlet_free(x->x_inlet_sync);
    inlet_free(x->x_inlet_phase);
    outlet_free(x->x_outlet);
    freebytes(x->x_phase, x->x_nchans * sizeof(*x->x_phase));
    return(void *)x;
}

static void *cosine_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_cosine *x = (t_cosine *)pd_new(cosine_class);
    t_float f1 = 0, f2 = 0;
    x->midi = x->soft = 0;
    x->x_phase = (double *)getbytes(sizeof(*x->x_phase));
    while(ac && av->a_type == A_SYMBOL){
        if(atom_getsymbol(av) == gensym("-midi"))
            x->midi = 1;
        else if(atom_getsymbol(av) == gensym("-soft"))
            x->soft = 1;
        ac--, av++;
    }
    if(ac && av->a_type == A_FLOAT){
        f1 = av->a_w.w_float;
        ac--, av++;
        if(ac && av->a_type == A_FLOAT){
            f2 = av->a_w.w_float;
            ac--, av++;
        }
    }
    t_float init_freq = f1;
    t_float init_phase = f2;
    init_phase = init_phase  < 0 ? 0 : init_phase >= 1 ? 0 : init_phase; // clipping phase input
    if(init_phase == 0 && init_freq > 0)
        x->x_phase[0] = 1.;
    else
        x->x_phase[0] = init_phase;
    init_sine_table();
    x->x_freq = init_freq;
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_sync, 0);
    x->x_inlet_phase = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_phase, init_phase);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
// Magic
    x->x_glist = canvas_getcurrent();
    x->x_signalscalar = obj_findsignalscalar((t_object *)x, 1);
    return(x);
}

void cosine_tilde_setup(void){
    cosine_class = class_new(gensym("cosine~"), (t_newmethod)cosine_new, (t_method)cosine_free,
        sizeof(t_cosine), CLASS_MULTICHANNEL, A_GIMME, 0);
    CLASS_MAINSIGNALIN(cosine_class, t_cosine, x_freq);
    class_addmethod(cosine_class, (t_method)cosine_soft, gensym("soft"), A_DEFFLOAT, 0);
    class_addmethod(cosine_class, (t_method)cosine_midi, gensym("midi"), A_DEFFLOAT, 0);
    class_addmethod(cosine_class, (t_method)cosine_dsp, gensym("dsp"), A_CANT, 0);
}

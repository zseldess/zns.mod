/*
 zns.mod - like %, but modulo range signed according to right operand
 
    Developed by Zachary Seldess, King Abdullah University of Science and Technology
*/

#include "ext.h" // standard Max include, always required
#include "ext_obex.h" // required for new style Max object
#include <math.h>

////////////////////////// object struct
typedef struct _mod 
{
    t_object s_obj; // the object itself (must be first)
    double s_value; // input value
    long s_type; // int or float modulo (0 == int, 1 == float)
    double mod_v; // modulo float value
    long m_in; // space for inlet number used by proxy
    void *m_proxy; // proxy inlet
    void *m_outlet1; // left outlet
} t_mod;

///////////////////////// function prototypes
void *mod_new(t_symbol *s, long argc, t_atom *argv);
void mod_free(t_mod *x);
void mod_assist(t_mod *x, void *b, long m, long a, char *s);

void mod_int(t_mod *x, long n);
void mod_float(t_mod *x, double f);
void mod_list(t_mod *x, t_symbol *s, long argc, t_atom *argv);
void mod_anything(t_mod *x, t_symbol *s, long argc, t_atom *argv);
void mod_bang(t_mod *x);

void custom_imod(t_mod *x, long n);
void custom_fmod(t_mod *x, double f);

//////////////////////// global class pointer variable
void *mod_class;


int main(void)
{	
    t_class *c;

    c = class_new("zns.mod", (method)mod_new, (method)mod_free, (long)sizeof(t_mod), 0L, A_GIMME, 0);

    class_addmethod(c, (method)mod_int, "int", A_LONG, 0);
    class_addmethod(c, (method)mod_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)mod_list, "list", A_GIMME, 0);    
    class_addmethod(c, (method)mod_anything, "anything", A_GIMME, 0);
    class_addmethod(c, (method)mod_bang, "bang", 0);
    
    class_addmethod(c, (method)stdinletinfo, "inletinfo", A_CANT, 0); // all right inlets are cold
    class_addmethod(c, (method)mod_assist, "assist", A_CANT, 0); // you CAN'T call this from the patcher

    class_register(CLASS_BOX, c); /* CLASS_NOBOX */
    mod_class = c;

    return 0;
}

void *mod_new(t_symbol *s, long argc, t_atom *argv)
{
    t_mod *x = NULL; 
	
    if ((x = (t_mod *)object_alloc(mod_class))) {
        x->m_proxy = proxy_new((t_object *)x, 1, &x->m_in); // proxy inlet
        x->m_outlet1 = outlet_new((t_object *)x, NULL); // left outlet
        /*
        object_post((t_object *)x, "a new %s object was instantiated: 0x%X", s->s_name, x);
        object_post((t_object *)x, "it has %ld arguments", argc);
        // report all provided arguments
        long i; 
        for (i = 0; i < argc; i++) {
            if ((argv + i)->a_type == A_LONG) {
                object_post((t_object *)x, "arg %ld: long (%ld)", i, atom_getlong(argv+i));
            } 
            else if ((argv + i)->a_type == A_FLOAT) {
                object_post((t_object *)x, "arg %ld: float (%f)", i, atom_getfloat(argv+i));
            } 
            else if ((argv + i)->a_type == A_SYM) {
                object_post((t_object *)x, "arg %ld: symbol (%s)", i, atom_getsym(argv+i)->s_name);
            }
            else {
                object_error((t_object *)x, "forbidden argument");
            }
        }
        */
        // set default object state
        x->s_value = 0.0;
        x->s_type = 0;
        x->mod_v = 1.0;
        
        // initialize with arg, if provided
        if (argc >= 1) {
            if ((argv)->a_type == A_LONG) {
                x->s_type = 0;
                x->mod_v = atom_getfloat(argv);           
            }
            if ((argv)->a_type == A_FLOAT) {
                x->s_type = 1;
                x->mod_v = atom_getfloat(argv);
            }
        }
    }
    return (x);
}

void mod_free(t_mod *x)
{
    ;
}

void mod_assist(t_mod *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) { // inlet
        switch (a) {
            case 0:
                sprintf(s, "Set left operand. Trigger the calculation");
                break;
            case 1:
                sprintf(s, "Set right operand");
                break;
            default:
                break;
        }
        //sprintf(s, "I am inlet %ld", a);
    } 
    else {	// outlet
        sprintf(s, "Result = Left %% Right");
        //sprintf(s, "I am outlet %ld", a);
    }
}

void custom_imod(t_mod *x, long n) {
    if (x->mod_v >= 0.0 && x->mod_v <= 1.0) {
        x->mod_v = 1.0;
    }
    if (x->mod_v <= 0.0 && x->mod_v >= -1.0) {
        x->mod_v = -1.0;
    }
    x->s_value = n % (long)x->mod_v;
    if (x->mod_v * x->s_value < 0.0) {
        x->s_value += x->mod_v; 
    }
    outlet_int(x->m_outlet1, x->s_value);
}

void custom_fmod(t_mod *x, double f) {
    x->s_value = fmod(f, x->mod_v);
    if (x->mod_v * x->s_value < 0.0) {
        x->s_value += x->mod_v; 
    }
    outlet_float(x->m_outlet1, x->s_value);
}

void mod_int(t_mod *x, long n)
{
    switch (proxy_getinlet((t_object *)x)) {
        case 0:
            if (x->s_type == 0) {
                custom_imod(x, n);
            }
            else {
                custom_fmod(x, (double)n);
            }
            break;
        case 1:
            x->mod_v = n;
            break;
    }
}

void mod_float(t_mod *x, double f)
{
    switch (proxy_getinlet((t_object *)x)) {
        case 0:
            if (x->s_type == 0) {
                custom_imod(x, (long)f);
            }
            else {
                custom_fmod(x, f);
            }
            break;
        case 1:
            if (x->s_type == 0) {
                x->mod_v = (long)f;
            }
            else {
                x->mod_v = f;
            }
            break;
    }
}

void mod_list(t_mod *x, t_symbol *s, long argc, t_atom *argv)
{
    /*
    // report message
    long i;
    t_atom *ap;
    
    post("message selector is %s",s->s_name);
    post("there are %ld arguments",argc);
    // increment ap each time to get to the next atom
    for (i = 0, ap = argv; i < argc; i++, ap++) {
        switch (atom_gettype(ap)) {
            case A_LONG:
                post("%ld: %ld",i+1,atom_getlong(ap));
                break;
            case A_FLOAT:
                post("%ld: %.2f",i+1,atom_getfloat(ap));
                break;
            case A_SYM:
                post("%ld: %s",i+1, atom_getsym(ap)->s_name);
                break;
            default:
                post("%ld: unknown atom type (%ld)", i+1, atom_gettype(ap));
                break;
        }
    }
    */
    if ((argv)->a_type == A_LONG || (argv)->a_type == A_FLOAT) {
        switch (proxy_getinlet((t_object *)x)) {
            case 0:
                if (x->s_type == 0) {
                    custom_imod(x, atom_getlong(argv));
                }
                else {
                    custom_fmod(x, atom_getfloat(argv));
                }
                break;
            case 1:
                if (x->s_type == 0) {
                    x->mod_v = atom_getlong(argv);
                }
                else {
                    x->mod_v = atom_getfloat(argv);
                }
                break;
        }
    }
}

void mod_anything(t_mod *x, t_symbol *s, long argc, t_atom *argv)
{
    /*
    // report message
    long i;
    t_atom *ap;
    
    post("message selector is %s",s->s_name);
    post("there are %ld arguments",argc);
    // increment ap each time to get to the next atom
    for (i = 0, ap = argv; i < argc; i++, ap++) {
        switch (atom_gettype(ap)) {
            case A_LONG:
                post("%ld: %ld",i+1,atom_getlong(ap));
                break;
            case A_FLOAT:
                post("%ld: %.2f",i+1,atom_getfloat(ap));
                break;
            case A_SYM:
                post("%ld: %s",i+1, atom_getsym(ap)->s_name);
                break;
            default:
                post("%ld: unknown atom type (%ld)", i+1, atom_gettype(ap));
                break;
        }
    }
    */    
    // if "set" selector, attempt to set value
    t_symbol *sel_set;
    sel_set = gensym("set");
    if (s == sel_set && argc >= 1) {
        if (argv->a_type == A_LONG || argv->a_type == A_FLOAT) {
            switch (proxy_getinlet((t_object *)x)) {
                case 0:
                    if (x->s_type == 0) {
                        x->mod_v = atom_getlong(argv);
                    }
                    else {
                        x->mod_v = atom_getfloat(argv);
                    }
                    break;
                case 1:
                    break;
            }
        } 
    }
}

void mod_bang(t_mod *x)
{
    switch (proxy_getinlet((t_object *)x)) {
        case 0:
            if (x->s_type == 0) {
                outlet_int(x->m_outlet1, x->s_value);
            }
            else {
                outlet_float(x->m_outlet1, x->s_value);
            }
            break;
        case 1:
            break;
    }
}

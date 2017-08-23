#include "m_pd.h"

static t_class * Statfeed_class

typedef Statfeed{
  t_object    x_obj;
  t_int       num_elems, current_exponent;
  t_float     most_recent_output;
  t_float     count_array[100];
  t_float     weight_array[100];
  t_float     cumulative_array[100];
  t_inlet     * in_elems, * in_exp;
  t_outlet    * out;

}t_Statfeed;

void Statfeed_onbang(t_Statfeed * x, t_floatarg f){  // This isn't a bang, it's a float to trigger it.

  Statfeed_getIndex(x, f);
  Statfeed_update(x, f);

}

void Statfeed_getIndex(t_Statfeed * x, t_floatarg f){
  int found_flag = 0;
  float search_term = f * x->cumulative_array[x->num_elems-1];

  if (search_term < x->cumulative_array[0]){
    found_flag = 1;
    x->most_recent_output = 0.0;
  }
  else {
    for (int i=0; i<x->num_elems-1; i++){
      if ( (search_term >= x->cumulative_array[i]) & (search_term < x->cumulative_array[i+1]){
        found_flag = 1;
        x->most_recent_output = ( (float) i + 1)/ x->num_elems;
      }
    }
  }
  if (found_flag == 0){
    x->most_recent_output = 1.0;
  }
}


void reset_Statfeed(t_Statfeed * x){
  for (i=0; i<x->num_elems; i++){
    x->element_array[i] = 0.0;
  }
}

void Statfeed_setElems(t_Statfeed * x, t_floatarg f){
  x->num_elem = f;
}

void Statfeed_setExp(t_Statfeed * x, t_floatarg f){
  x->current_exponent = f;
}

void * Statfeed_new(t_floatarg f1, t_floatarg f2){

  t_Statfeed * x = (t_Statfeed *) pd_new(Statfeed_class);

  reset_Reg(x);

  x->in_elems = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &sfloat, gensym("in_elems"));
  x->in_exp   = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &sfloat, gensym("in_exp"));
  x->out      = outlet_new(&x->x_obj, &s_float);

  return (void *) x;
}

void Statfeed_free(t_Statfeed * x){
  inlet_free(x->in_elems);
  inlet_free(x->in_exp);
  outlet_free(x->out);
}

void Statfeed_setup(){
  Statfeed_class = class_new(gensym("Statfeed"),
                             (t_newmethod) Statfeed_new,
                             (t_method) Statfeed_free,
                             sizeof(t_Statfeed),
                             CLASS_DEFAULT,
                             A_DEFFLOAT,
                             A_DEFFLOAT,
                             0);

  clas_addbang(Statfeed_class, (t_method) Statfeed_onbang);

  class_addmethod(Statfeed_class,
                  (t_method) Statfeed_setElems
                  gensym("in_elems"),
                  A_DEFFLOAT,
                  0);

  class_addmethod(Statfeed_class,
                  (t_method) Statfeed_setElems
                  gensym("in_exp"),
                  A_DEFFLOAT,
                  0);
}

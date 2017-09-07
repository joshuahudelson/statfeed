#include "m_pd.h"
#include "math.h"

static t_class * statfeed_class;

typedef struct statfeed{
  t_object    x_obj;
  t_int       num_elems, current_exponent;
  t_int       most_recent_output;
  t_float     count_array[100];
  t_float     weight_array[100];
  t_float     cumulative_array[100];
  t_inlet     * in_elems, * in_exp;
  t_outlet    * out;

}t_statfeed;

void statfeed_getIndex(t_statfeed * x, t_floatarg f){
  // make this use "low" and "high"

  int found_flag = 0;
  float search_term = f * x->cumulative_array[x->num_elems-1];
  post("search term: %f", f);

  if (search_term < x->cumulative_array[0]){
    found_flag = 1;
    x->most_recent_output = 0;
  }
  else {
    for (int i=0; i<x->num_elems-1; i++){
      if ( (search_term >= x->cumulative_array[i]) & (search_term < x->cumulative_array[i+1]) ){
        found_flag = 1;
        x->most_recent_output = i + 1;
      }
    }
  }
  if (found_flag == 0){
    x->most_recent_output = 1;
  }
}

void statfeed_increment(t_statfeed * x){
  for (int i=0; i<x->num_elems; i++){
    x->count_array[i] += 1;
  }
}

void statfeed_zero(t_statfeed * x){
  x->count_array[x->most_recent_output] = 0;
}

void statfeed_scale(t_statfeed * x){
  int largest = 0;
  for (int i=0; i<x->num_elems;i++){
    if (x->count_array[i] > largest){
      largest = x->count_array[i];
    }
  }

  for (int i=0;i<x->num_elems;i++){
    x->weight_array[i] = (float) x->count_array[i]/largest;
  }
}

void statfeed_exponentiate(t_statfeed * x){
  for (int i=0;i<x->num_elems;i++){
    x->weight_array[i] = pow(x->count_array[i], x->current_exponent);
    post("elem, weight: %f, %f", x->count_array[i], x->weight_array[i]);
  }
}

void statfeed_sum(t_statfeed * x){
  x->cumulative_array[0] = x->weight_array[0];
  for (int i=1; i<x->num_elems-1; i++){
    x->cumulative_array[i] = x->cumulative_array[i-1] + x->weight_array[i];
  }
}

void statfeed_update(t_statfeed * x, t_floatarg f){

  statfeed_increment(x);
  statfeed_zero(x);
  statfeed_scale(x);
  statfeed_exponentiate(x);
  statfeed_sum(x);
}

void reset_statfeed(t_statfeed * x){
  for (int i=0; i<x->num_elems; i++){
    x->count_array[i] = 1.0;
  }
}

void statfeed_setElems(t_statfeed * x, t_floatarg f){
  x->num_elems = f;
}

void statfeed_setExp(t_statfeed * x, t_floatarg f){
  x->current_exponent = f;
}

void statfeed_onbang(t_statfeed * x, t_floatarg f){  // This isn't a bang, it's a float to trigger it.
  post("Bangs do nothing for me.");
}

void statfeed_onfloat(t_statfeed * x, t_floatarg f){
  post("Input float: %f", f);
  statfeed_getIndex(x, f);
  statfeed_update(x, f);
  outlet_float(x->out, x->most_recent_output);
}

void * statfeed_new(t_floatarg f1, t_floatarg f2){

  t_statfeed * x = (t_statfeed *) pd_new(statfeed_class);

  reset_statfeed(x);

  x->in_elems = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("in_elems"));
  x->in_exp   = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("in_exp"));
  x->out      = outlet_new(&x->x_obj, &s_float);

  for (int i=0; i<100; i++){
    x->count_array[i] = 1;
  }

  statfeed_exponentiate(x);
  statfeed_sum(x);

  return (void *) x;
}

void statfeed_free(t_statfeed * x){
  inlet_free(x->in_elems);
  inlet_free(x->in_exp);
  outlet_free(x->out);
}

void statfeed_setup(void){
  statfeed_class = class_new(gensym("statfeed"),
                             (t_newmethod) statfeed_new,
                             (t_method) statfeed_free,
                             sizeof(t_statfeed),
                             CLASS_DEFAULT,
                             A_DEFFLOAT,
                             A_DEFFLOAT,
                             0);

  class_addbang(statfeed_class, (t_method) statfeed_onbang);

  class_addfloat(statfeed_class, (t_method) statfeed_onfloat);

  class_addmethod(statfeed_class,
                  (t_method) statfeed_setElems,
                  gensym("in_elems"),
                  A_DEFFLOAT,
                  0);

  class_addmethod(statfeed_class,
                  (t_method) statfeed_setExp,
                  gensym("in_exp"),
                  A_DEFFLOAT,
                  0);
}

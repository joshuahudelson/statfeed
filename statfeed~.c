/*  Copyright (c) 2015 Joshua Hudelson
 *
 *  Statfeed~ is a sound synthesis external for Pure Data.
 *  It generates an audio signal based on the "Statistical
 *  Feedback" process, as described by Charles Ames and
 *  Larry Polansky.
 *
 *  For more information on Statistical Feedback, see Ames'
 *  "Statistics and Compositional Balance" in Perspectives
 *  of New Music, Vol. 28, No. 1, Winter, 1990, pp. 80-111
 *  and Polansky's "A Few More Words about James Tenney:
 *  Dissonant Counterpoint and Statistical Feedback" in the
 *  Journal of Mathematics and Music, Vol. Volume 5, Number 2,
 *  1 July 2011, pp. 63-82.
 *
 *  Work on this external began in June of 2014 at the
 *  Workshop in Algorithmic Computer Music at U.C. Santa
 *  Cruz.  Special thanks to Larry Polansky, David Kant,
 *  Jaime Oliver, and Noah Hudelson for their invaluable help.
 */

#include "m_pd.h"
#include "stdlib.h"
#include "math.h"

static t_class *statfeed_tilde_class;

typedef struct _statfeed_tilde {

    t_object x_obj;

    t_sample signal1;          //  Signal input: number between -1.0 and 1.0.
    t_sample inlet1;           //  Float input: number of "bins" (elements) to use.
    t_sample inlet2;           //  Float input: exponent by which to raise the count in each bin.

    t_inlet *x_in1;
    t_inlet *x_in2;
    t_outlet *x_out;

    float **lookupless;       //  2D array of scaled bin values (0.0 to 1.0) raised to exponent values (0 to 1000).
    float **lookupmore;
    int numpossibleincrements; //  The higest value a bin can have.

    int *bins;                 //  Array to keep track of each bin's value.
    float *exponbins;          //  Array to keep track of each bin's value after being raised to the exponent.
    float *cumuexponbins;      //  Array to keep track of the cumulative value of each exponbin's value.

    int maxnumbins;            //  The maximum number of bins possible.
    int numbinsinuse;          //  The number of bins in use at the current moment (received from inlet1).

    int maxexpon;              //  The highest exponent possible.
    float expon;               //  The exponent in use at the current moment (received from inlet2).

    float choice;              //  The input value that determines the next result (received from signal1).
    int choiceindex;           //  The index number of a bin associated with choice.
    float outputter;           //  The final result (sent out x_out), scaled to between -1.0 and 1.0.

    int top, bottom, mid;      //  Variables used in a binary search in the Perform function.

} t_statfeed_tilde;

//-------------------------------------------------------PERFORM FUNCTION
t_int *statfeed_tilde_perform(t_int *w)
{
  t_statfeed_tilde *x = (t_statfeed_tilde *)(w[1]);
  t_sample  *in1 =    (t_sample *)(w[2]);
  t_sample  *out =    (t_sample *)(w[3]);
  int          n =           (int)(w[4]);

int i;

//  Make sure that Inlet 1 (not the signal inlet) is within range.  Then give the value to numbinsinuse.
    if (x->inlet1 < 2){
        x->numbinsinuse = 2;
    }
    else if(x->inlet1 > x->maxnumbins){
        x->numbinsinuse = x->maxnumbins-1;
    }
    else{
        x->numbinsinuse = x->inlet1;
    }

//  Make sure that Inlet 2 is within range.  Then give value to expon.
    if (x->inlet2 < 0){
        x->expon = 0;
    }
    else if (x->inlet2 > x->maxexpon){
        x->expon = (float) x->maxexpon-1.0;
    }
    else{
        x->expon = x->inlet2;
    }

//  Any bins not in use get their counts also set to zero, for safety.
for(i=x->numbinsinuse;i<x->maxnumbins;i++){
    x->bins[i] = 0;
}

//  The DSP part.
while (n--){

int localmax = 1;

for(i=0;i<x->numbinsinuse;i++){
	if(x->bins[i] > localmax){
	localmax = x->bins[i];
	}
}

if (x->expon<1.0){
	x->exponbins[0] = x->lookupless[(int) (((x->numpossibleincrements-1)*((float) x->bins[0])/ ((float) localmax)))][(int) ((x->numpossibleincrements - 1)*x->expon)];
	x->cumuexponbins[0] = x->exponbins[0];
}

else{
	x->exponbins[0] = x->lookupmore[(int) (((x->numpossibleincrements-1)*((float) x->bins[0])/ ((float) localmax)))][(int) x->expon];
	x->cumuexponbins[0] = x->exponbins[0];
}


//  Calculate the rest of both arrays.
for(i=1;i<x->numbinsinuse;i++){
	if(x->expon<1.0){
		x->exponbins[i] = x->lookupless[(int) (((x->numpossibleincrements-1)*(x->bins[i]/ (float) localmax)))][(int) ((x->numpossibleincrements - 1)*x->expon)];
		x->cumuexponbins[i] = x->cumuexponbins[i-1] + x->exponbins[i];
	}
	else{
		x->exponbins[i] = x->lookupmore[((int) ((x->numpossibleincrements - 1)*(x->bins[i]/(float) localmax)))][(int) x->expon];
		x->cumuexponbins[i] = x->cumuexponbins[i-1] + x->exponbins[i];
	}
}

//  Give incoming random number to choice, then make sure the number is within range.
x->choice = *in1++;

if(x->choice > 1.0){
    x->choice = 1.0;
}
else if(x->choice < -1.0){
    x->choice = -1.0;
}

//  Scale choice and use it to determine what value to search for within cumuexponbins.
x->choice = ((x->choice + 1.0) * 0.5) * x->cumuexponbins[x->numbinsinuse-1];

//  If choice is now zero, we'll have problems, so make it equal a single increment.
if(x->choice == 0.0){
	x->choice = 0.001;
}

if (x->cumuexponbins[0] >= x->choice){
	x->choiceindex = 0;
}
else {
	for(i=1;i<x->numbinsinuse;i++){
     		if ((x->cumuexponbins[i] >= x->choice) && (x->cumuexponbins[i-1] < x->choice)) {
			x->choiceindex = i;
			break;	
		}
	}
}


/*

//  Do a binary search for the choice's value within cumuexponbins and set choiceindex equal to that index.
x->bottom = 0;
x->top = x->numbinsinuse-1;

while (x->top >= x->bottom){

    x->mid = (int) (x->bottom + ((x->top - x->bottom) * 0.5));

    if (x->choice > x->cumuexponbins[x->mid]){
        if (x->choice <= x->cumuexponbins[x->mid + 1]){
            x->choiceindex = x->mid + 1;
            break;
        }
        else{
            x->bottom = x->mid + 1;
        }
    }
    else{
        if(x->choice > x->cumuexponbins[x->mid-1]){
            x->choiceindex = x->mid;
            break;
        }
        else{
            x->top = x->mid - 1;
        }
    }
}
*/


//  Increment all bins in use.
for(i=0;i<x->numbinsinuse;i++){
    x->bins[i] += 1;
    if (x->bins[i] > x->numpossibleincrements){
            x->bins[i] = x->numpossibleincrements;
    }
}

// Set outputter equal to the choice index scaled to between -0.5 and 0.5.
x->outputter = (((float) x->choiceindex / (float) (x->numbinsinuse - 1)) - .5);

//  Scale outputter to between -1.0 and 1.0 (keeping this separate from above because it might be necessary to narrow the range, slightly, in the future).
*out++ =  x->outputter; // * 2.0;

//  Set the chosen bin to zero.
x->bins[x->choiceindex] = 0;

}

return (w+5);

}

//-------------------------------------------------------BANG FUNCTION
void statfeed_tilde_bang(t_statfeed_tilde *x)
{
    post("Maxexpon = %d", x->maxexpon);
    post("Maxnumbins = %d", x->maxnumbins);
}

//-------------------------------------------------------DSP FUNCTION
void statfeed_tilde_dsp(t_statfeed_tilde *x, t_signal **sp)
{
  dsp_add(statfeed_tilde_perform, 4, x,
          sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

//-------------------------------------------------------NEW FUNCTION
void *statfeed_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_statfeed_tilde *x = (t_statfeed_tilde *)pd_new(statfeed_tilde_class);

//  Create inlets and link to variables.
    x->x_in1=floatinlet_new (&x->x_obj, &x->inlet1);
    x->x_in2=floatinlet_new (&x->x_obj, &x->inlet2);

//  Create outlet.
    x->x_out=outlet_new(&x->x_obj, gensym("signal"));

//  Initialize state variables.
x->maxnumbins = 1000;
x->maxexpon = 1000;
x->numpossibleincrements = 1000;

//  If there are arguments, initialize to these, instead.
switch(argc){
  default:
  case 2:
    x->maxexpon = (int) atom_getfloat(argv+1);
  case 1:
    x->maxnumbins = (int) atom_getfloat(argv);
    break;
  case 0:
    break;
}

int i, j;

//  Create the look-up tables
x->lookupless = (float**) getbytes(x->numpossibleincrements*sizeof(float*));
for(i=0;i<x->numpossibleincrements;i++){
        x->lookupless[i] = (float*) getbytes(x->maxexpon*sizeof(float*));
}

x->lookupmore = (float**) getbytes(x->numpossibleincrements*sizeof(float*));
for(i=0;i<x->numpossibleincrements;i++){
        x->lookupmore[i] = (float*) getbytes(x->maxexpon*sizeof(float*));
}

//  Fill in the values for the table.
for(i=0;i<x->numpossibleincrements;i++){
    for(j=0;j<x->maxexpon;j++){
        x->lookupmore[i][j] = pow(((float) i / (float) x->numpossibleincrements), j);
    }
}
for(i=0;i<x->numpossibleincrements;i++){
    for(j=0;j<x->maxexpon;j++){
        x->lookupless[i][j] = pow(((float) i / (float) x->numpossibleincrements), ((float) j / (float) x->maxexpon));
    }
}


//  Create arrays for bins, exponbins, and cumuexponbins.
x->bins = (int*) getbytes(x->maxnumbins*sizeof(int*));
x->exponbins = (float*) getbytes(x->maxnumbins*sizeof(float*));
x->cumuexponbins = (float*) getbytes(x->maxnumbins*sizeof(float*));

//  Initialize those bins: all of them at maximum.
for(i=0;i<x->maxnumbins;i++){
    //x->bins[i] = x->numpossibleincrements;
	x->bins[i] = 1;
}

return (void *)x;

}

//-------------------------------------------------------SETUP FUNCTION
void statfeed_tilde_setup(void) {
    statfeed_tilde_class = class_new(gensym("statfeed~"),
        (t_newmethod)statfeed_tilde_new,
        0, sizeof(t_statfeed_tilde),
        CLASS_DEFAULT,
        A_GIMME, 0);

//  Respond to the "dsp" message from PD.
    class_addmethod(statfeed_tilde_class,
        (t_method)statfeed_tilde_dsp, gensym("dsp"), 0);

//  Additional Methods (for first inlet).
  class_addbang(statfeed_tilde_class, statfeed_tilde_bang);

  CLASS_MAINSIGNALIN(statfeed_tilde_class, t_statfeed_tilde, signal1);

post("Copyright (c) 2015 Joshua Hudelson.");
post("");
post("Statfeed~ is a sound synthesis external for Pure Data.");
post("It generates an audio signal based on the \"Statistical");
post("Feedback\" process, as described by Charles Ames and");
post("Larry Polansky.");
post("");
post("For more information on Statistical");
post("Feedback, see Ames'\"Statistics and Compositional");
post("Balance\" in Perspectives of New Music, Vol. 28, No. 1,");
post("Winter, 1990, pp. 80-111 and Polansky's \"A Few More");
post("Words about James Tenney: Dissonant Counterpoint and");
post("Statistical Feedback\" in the Journal of Mathematics");
post("and Music, Vol. Volume 5, Number 2, 1 July 2011, pp.");
post("63-82.");
post("");
post("Work on this external began in June of 2014 at the");
post("Workshop in Algorithmic Computer Music at U.C. Santa");
post("Cruz.  Special thanks to Larry Polansky, David Kant,");
post ("Jaime Oliver, and Noah Hudelson for their invaluable");
post("help.");;
}

//-------------------------------------------------------FREE FUNCTION
void statfeed_tilde_free(t_statfeed_tilde *x)
{
//  Do I need these?
    inlet_free(x->x_in1);
    inlet_free(x->x_in2);
    outlet_free(x->x_out);

//  Do I need to do more here--for example, free all the sub-arrays within the lookuptable?
    freebytes(x->lookupmore, x->numpossibleincrements);
    freebytes(x->lookupless, x->numpossibleincrements);
    freebytes(x->bins, x->maxnumbins);
    freebytes(x->exponbins, x->maxnumbins);
    freebytes(x->cumuexponbins, x->maxnumbins);
}

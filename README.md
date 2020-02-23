# Statfeed

*** Directions ***

The first inlet receives an audio signal.  To run the
external in its standard, probabilistic mode, connect
it to the output of a noise~ object.

The second inlet receives a float that determines
the number of elements (which directly corresponds 
to the periodicity of the output signal).  Minimum 
is 2; maximum is 1000.

The third inlet receives a float which determines the
exponent of the growth function.  This corresponds to how 
predictable the periodicity of the output signal will be.
Minimum is 0 (white noise); maximum is 1000.


*** Info ***

Statistical Feedback external for Pure Data

Copyright (c) 2015 Joshua Hudelson

Statfeed~ is a sound synthesis external for Pure Data.
It generates an audio signal based on the "Statistical
Feedback" process, as described by Charles Ames and
Larry Polansky.

For more information on Statistical Feedback, see Ames'
"Statistics and Compositional Balance" in Perspectives
of New Music, Vol. 28, No. 1, Winter, 1990, pp. 80-111
and Polansky's "A Few More Words about James Tenney:
Dissonant Counterpoint and Statistical Feedback" in the
Journal of Mathematics and Music, Vol. Volume 5, Number 2,
1 July 2011, pp. 63-82.

Work on this external began in June of 2014 at the
Workshop in Algorithmic Computer Music at U.C. Santa
Cruz.  Special thanks to Larry Polansky, David Kant,
Jaime Oliver, and Noah Hudelson for their invaluable help.

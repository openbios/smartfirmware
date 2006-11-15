/*****
From the "Jargon File":

Duff's device: n. The most dramatic use yet seen of {fall
through} in C, invented by Tom Duff when he was at Lucasfilm.
Trying to {bum} all the instructions he could out of an inner
loop that copied data serially onto an output port, he decided to
{unroll} it.  He then realized that the unrolled version could
be implemented by *interlacing* the structures of a switch and
a loop:
*****/

void
func(char *from, int count)
{
	register n = (count + 7) / 8;       /* count > 0 assumed */
	extern volatile int *to;

	switch (count % 8)
	{
	case 0: do {    *to = *from++;
	case 7:         *to = *from++;
	case 6:         *to = *from++;
	case 5:         *to = *from++;
	case 4:         *to = *from++;
	case 3:         *to = *from++;
	case 2:         *to = *from++;
	case 1:         *to = *from++;
	} while (--n > 0);
	}
}

/*****
Having verified that the device is valid portable C, Duff announced
it.  C's default {fall through} in case statements has long been
its most controversial single feature; Duff observed that "This
code forms some sort of argument in that debate, but I'm not sure
whether it's for or against."
*****/

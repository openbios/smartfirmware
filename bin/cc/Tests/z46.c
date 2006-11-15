/************

Date:    Fri, 29 Sep 2000 17:25:57 PDT
To:      Parag Patel <parag@codegen.com>
From:    "Thomas J. Merritt" <tjm@codegen.com>
Org.:    CodeGen, Inc., San Francisco, CA
Subject: cc-fcode

The following code breaks cc-fcode.  The null statement in func1 causes
the g_null_expr node to used in the tree.  When you done compling 
func1 you delete the object pointed to by g_null_expr.  Then in func2
you reuse the pointer to the deleted object and it now contains junk
which causes a SIGSEGV.  I know the easy answer is don't do that.  But...

-- TJ
************/

int x;

int
func1()
{
	if (x)
		;
	else
		x = 1;
}

int
func2()
{
	if (x)
		;
	else
		x = 2;
}

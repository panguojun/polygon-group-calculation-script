[0.707,0,0.707];
[1,0,0];
[0,1,0,45];
[0,1,0, 15];
[0,1,0, 90];
[0,1,0, 135];

#branch(d){
	pushc();
	push();
		ext(5.);
		face();
		
		pushc();
		ang = 20.;
		@5.{
			push();
			ang = ang * 0.7 - 10.;
			pit(ang);
			scl(0.95);
			ext(1.);
			face();
		}
		
		?(d < 3.){
			branch(d + 1.);
		}
		pop(5.);
		popc();
		
		pushc();
		ang = -20.;
		@5.{
			push();
			ang = ang * 0.8 + 10.;
			pit(ang);
			ext(1.);
			scl(0.95);
			face();
		}	
		?(d < 4.){
			branch(d + 1.);
		}
		pop(5.);
		popc();
		
	pop();	
	popc();
	$0.;	
}

'shell 
#shell()
{
	pushc();
	ang = 1.;
	d = 1.;
	push();
		@25.{
			push();
			ang = ang * 1.1;
			d = d * 0.5;
			pit(ang);
			ext(0.1 * d);
			mov(0.1,0.,0.);
			scl(0.95);
			face();
		}	
	pop();
	popc();
	$0.;
}

rgb(200.,200.,200.);

t = 1,e=1;
@8.{
e = e + t;
t = t * 3;
}

>calign(e);
branch(0.);

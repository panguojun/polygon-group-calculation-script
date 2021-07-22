[1,0,0];
[0,1,0,45];
[0,3,0,0];
[0,1,0,-45];
[0,1,0,5];

#branch(d){
	push();
		ext(5.);
		face();
		
		ang = 10.;
		@15.{
			push();
			ang = ang * 0.7;
			pit(ang);
			i = i + 1.;
			ext(1.);
			face();
		}	
		?(d < 2.){
				push();
				branch(d +1.);
				pop();
			}
		pop(15.);
		ang = -10.;
		@15.{
			push();
			ang = ang * 0.8;
			pit(ang);
			i = i + 1.;
			ext(1.);
			face();
		}	
		?(d < 5.){
				push();
				branch(d +1.);
				pop();
			}
		pop(15.);
	$0.;	
}


e = 1,t=1;
@7.{
	e = e + t;
	t = t * 2;
}

push(e);
branch(1.);

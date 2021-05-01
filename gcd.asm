//---top---
//r1 = i
//r2 = j
//r3 = temp?
//---mod---
//r4 = a
//r5 = oldA
//r6 = b
//r7 = oldB
//r8 = quotient
//r9 = mod
//r10 = result
//---gcd---

//for(i = 18; i <= 24; i++)
//	print gcd(180, i)

//int gcd(int a, int b) {
//	if (b == 0)
//		return a;
//	else
//		return gcd(b, a mod b);
//}

mod: //mod(a, b) currently set to mod(180, 24)
movi 180 r4 // a = 180
mov r4 r5 // oldA = a

movi 24 r6 //b = 24
mov r6 r7 //oldB = b

idiv r6 r4
mov r4 r8 //quotient = a div b

mul r8 r6
mov r6 r9 //mod = b * quotient

sub r9 r5
mov r5 r10 //result = a - mod
out r10

movi 18 r1 // i = 18 (uses r1 as i)

loop:
out r1 //print i (modify this to gcd value later)

movi 1 r2 //r2 = 1
add r2 r1 //r1 += 1 (basically i++)

mov r1 r3 //r3 = r1 //if i < 25
movi 25 r2 //r2 = 25
sub r2 r3 // r3 =- r2

movi loop r2 //r2 = top
jnz r3 r2 //if(r3!=0) jumps to r2 (back to top)

halt

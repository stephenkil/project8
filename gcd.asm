movi 18 r1 // i = 18 (uses r1 as i)

top:
add r17 r1
out r1 //print i

movi 1 r2 //r2 = 1
add r2 r1 //r1 += 1 (basically i++)

mov r1 r3 //r3 = r1
movi 25 r2 //r2 = 25
sub r2 r3 // r3 =- r2

movi top r2 //r2 = top
jnz r3 r2 //if(r2!=0) jumps to r2

gcd:

halt

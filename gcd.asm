//-----register assignments-----
//r1 = i
//r2 = increment value
//r3 = temp
//r4 = a
//r5 = oldA
//r6 = b
//r7 = oldB
//r8 = quotient
//r9 = temp
//r10 = calculatedMod
//r12 = loop
//r13 = end
//r14 = printI
//r15 = gcd

//-----initializing variables-----
movi 17 r1 // i = 17 (effectively i = 18, since loop starts with an i++)
movi mod r11 //r11 = mod
movi loop r12 //r12 = loop
movi end r13 //r13 = end
movi printI r14 //r14 = printI
movi gcd r15 //r15 = gcd
//--------------------------------
loop:

movi 1 r2 //r2 = 1
add r2 r1 //i += 1 (increment)

mov r1 r3 //r3 = r1
movi 25 r2 //r2 = 25
sub r2 r3 // r3 =- r2

movi 180 r4 // a = 180
mov r4 r5 // oldA = a

mov r1 r6 //b = i
mov r6 r7 //oldB = b

jnz r3 r11 //if i < 25, jumps to mod
jmp r13 //jumps to end
//--------------------------------
mod: //mod(a, b)

idiv r6 r4
mov r4 r8 //quotient = a div b

mul r8 r6
mov r6 r9 //temp = b * quotient

sub r9 r5
mov r5 r10 //calculatedMod = a - temp
//out r10 //prints calculatedMod
push r10 //pushes calculatedMod into stack

jnz r10 r15 //if(calculatedMod!=0) jumps to gcd
jmp r14 //if(calculated==0), jumps to printI
//--------------------------------
gcd: //gcd(a, b)

//declare new a and b (for recursive case)
//mov r6 r4 //a = b
//mov r4 r5 // oldA = a
//mov ?? r6 //b = mod(i, calculatedMod)
//mov r6 r7 //oldB = b

movi 0 r20 //placeholder for recursive gcd values
out r20
//jnz r10 r15 //if(calculatedMod!=0) jumps to loop
jmp r12 //if(calculated==0), jumps to loop
//--------------------------------
end:
halt //ends the program
//--------------------------------
printI: //non-recursive case
out r1 //prints i
jmp r12//jumps to loop

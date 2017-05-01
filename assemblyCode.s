 
/*

// *** This method is from the p2switches.c file, 3rd method ***

static unsigned char switch_mask;
static unsigned char switches_last_reported;
static unsigned char switches_current;

unsigned int 
p2sw_read() {
  unsigned int sw_changed = switches_current ^ switches_last_reported;
  switches_last_reported = switches_current;
  return switches_current | (sw_changed << 8);
}
*/

push    r4
mov     r1, r4
add     #2, r4
sub     #2, r1
mov.b   &switches_current, r14
mov.b   &switches_last_reported, r15
xor.b   r14, r15
mov.b   r15, -4(r4)
mov.b   #0, -3(r4)
mov.b   &switches_current, r15
mov.b   r15, &switches_last_reported
mov.b   &switches_current, r15
mov.b   r15, r14
mov     -4(r4), r15
and     #255, r15
swpb    r15
bis     r14, r15
add     #2, r1
pop     r4

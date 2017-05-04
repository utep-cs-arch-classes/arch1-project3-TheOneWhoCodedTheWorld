;/*
;void wdt_c_handler()
;{
;  static short count = 0;
;  P1OUT |= GREEN_LED;
;  count ++;
;  u_int switches = p2sw_read(), i;
;    char str[5];
;  if (count == 15) {
;      mlAdvance(&ml0, &fieldFence, &plyr1, &plyr2);
;    for (i = 0; i < 4; i++)
;      str[i] = (switches & (1<<i)) ? 0: 1;
;    
;    if(str[0]) {//button 1
;        ml1.velocity.axes[1] = -5;
;        ml1.velocity.axes[0] = 0;
;    }else{
;      ml1.velocity.axes[1] = 0;
;      ml1.velocity.axes[0] = 0;
;    }
;    if(str[1]) {//button 2
;       ml1.velocity.axes[1] = 5;
;       ml1.velocity.axes[0] = 0;
;    }
;    if(str[2]){//button 3
;      ml0.velocity.axes[1] = -5;
;      ml0.velocity.axes[0] = 0;
;    }else{
;      ml0.velocity.axes[1] = 0;
;      ml0.velocity.axes[0] = 0;
;    }
;    if(str[3]){//button 4
;      ml0.velocity.axes[1] = 5;
;      ml0.velocity.axes[0] = 0;
;    }
;
;    //Player 1 scores
;    if(player1Scored > '2'){
;      drawChar5x7(11,screenHeight-8,'3',COLOR_BLACK,COLOR_PINK);
;      ml0.velocity.axes[0] = 0;
;      ml0.velocity.axes[1] = 0;
;      ml1.velocity.axes[0] = 0;
;      ml1.velocity.axes[1] = 0;
;      ml3.velocity.axes[0] = 0;
;      ml3.velocity.axes[1] = 0;
;      drawString5x7(14, 74,"|   GAME OVER   |", COLOR_BLUE, ;COLOR_GREEN);
;      drawString5x7(14, 82,"| PLAYER 1 WINS |", COLOR_BLUE, ;COLOR_GREEN);
;    }
;    else
;    {
;      ;drawChar5x7(11,screenHeight-8,player1Scored,COLOR_BLACK,COLOR_PINK);
;    }
;   
;    //Player 2 scores
;    if(player2Scored > '2')
;    {
;      ;drawChar5x7(screenWidth-13,screenHeight-8,'3',COLOR_BLUE,COLOR_PINK);
;      ml0.velocity.axes[0] = 0;
;      ml0.velocity.axes[1] = 0;
;      ml1.velocity.axes[0] = 0;
;      ml1.velocity.axes[1] = 0;
;      ml3.velocity.axes[0] = 0;
;      ml3.velocity.axes[1] = 0;
;      drawString5x7(14, 74,"|   GAME OVER   |", COLOR_BLUE, ;COLOR_GREEN);
;      drawString5x7(14, 82,"| PLAYER 2 WINS |", COLOR_BLUE, ;COLOR_GREEN);
;    }
;    else
;    {
;       ;drawChar5x7(screenWidth-13,screenHeight-8,player2Scored,COLOR_BLUE,COLOR;_PINK);
;    }
;    
;    if (p2sw_read())
;    {
;      redrawScreen = 1;
;    }
;    count = 0;
;  }
;  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
;}
;
;*/

;.file "shapemotion.c"
;.arch msp430g2553
;.p2align 1,0
;.data

    .global wdt_c_handler
    .extern layerGetBounds
    .extern mlAdvance
    .extern drawChar5x7
    .extern drawString5x7
wdt_c_handler:
    sub #6, r1 ; make room for auto variables
    mov r12, 2(r1) ; initialize count
    BIS r13, 4(r1) ; P1OUT |= GREEN_LED
    add #1, 2(r1)  ; count++
    mov #p2sw_read, 6(r1) ; switches = p2sw_read 
    mov #p2sw_read, 8(r1) ; switches = i
    add #2, 6(r1) ; char str[5]
    cmp #15, 2(r1)
    jge buttons
    call #mlAdvance
    mov #0, -4(r2)
    jmp loop
loop:
    add #1, -4(r2)
incr:
    mov.b #1, r14
    cmp #4, -4(r2)
    jl endLoop
    mov.b #0, r14
endLoop:
    cmp.b #0, r14
    jne loop
buttons:
    cmp #0, 8(r1)
    jeq else
    mov.b #-5, -5(r3)
    mov.b #0, 0(r15)
    jmp secIf
else:
    mov.b #0, 0(r15)
    mov.b #0, 0(r15)
secIf:
    mov.b #-5(r3), r15
    cmp #0, 8(r1)
    jeq thirdIf
    mov.b #5, -5(r3)
thirdIf:
    cmp #0, 8(r1)
    jeq thirdElse
    mov.b #-5, -5(r3)
    mov.b #0, 2(r15)
thirdElse:
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
lastIf:
    mov.b #5(r3), -5(r3)
    cmp #0, 8(r1)
    mov.b #5, -5(r3)
    jmp plyer1S
plyer1S:
    cmp #9, &player1Scored
    jge p1sElse
    call #drawChar5x7
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    call #drawString5x7
    call #drawString5x7
p1sElse:
    call #drawChar5x7
player2S:
    cmp #9, &player2Scored
    jge p2sElse
    call #drawChar5x7
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    mov.b #0, 2(r15)
    call #drawString5x7
    call #drawString5x7
p2sElse:
    call #drawChar5x7
endIf:
    call #p2sw_read
    mov #1, &redrawScreen
    jmp end
end:
    mov #0, 2(r1)
    add #6, r1
    pop r12

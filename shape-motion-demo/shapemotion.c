/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6

char player1Scored = '0';
char player2Scored = '0';

AbRect rect10 = {abRectGetBounds, abRectCheck, {2 ,10}};/**< 10x10 rectangle */

/* playing field */
AbRectOutline fieldOutline = { 
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 2.5, screenHeight/2 - 5}
};

/* Ping pong */
Layer layer3 = {
  (AbShape *)&circle4,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /* bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_ORANGE
};

/* playing field as a layer */
Layer fieldLayer = {		
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2}, /**< center */
  {0,0}, {0,0},		    /* last & next pos */
  COLOR_RED,
  &layer3
};

/* Layer with a BLACK PALETTE  */
Layer layer1 = {
  (AbShape *)&rect10,
  {14, 30},            /* center */
  {0,0}, {0,0}, /* last & next pos */
  COLOR_BLACK,
  &fieldLayer,
};

/*  Layer with a GREEB PALETTE */
Layer layer0 = {
  (AbShape *)&rect10,
  {screenWidth-14, screenHeight-30}, /* bit below & right of center */
  {0,0}, {0,0},			     /* last & next pos */
  COLOR_BLUE,
  &layer1,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer mlPong = { &layer3, {3,3}, 0 }; /**< not all layers move */
MovLayer mlBlcP = { &layer1, {0,0}, &mlPong }; 
MovLayer mlBluP0 = { &layer0, {0,0}, &mlBlcP }; 

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8); /* disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer; /* For any layer Object */
    l->posLast = l->pos; 
    l->pos = l->posNext;
  }
  or_sr(8);			/* disable interrupts (GIE on) */

  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  

/* Create buzzer noise */
void buzzer_init(short cycles) {
  timerAUpmode();
  P2SEL2 &= ~(BIT6 | BIT7);
  P2SEL &= ~BIT7;
  P2SEL |= BIT6;
  P2DIR = BIT6;
  CCR0 = cycles;
  CCR1 = cycles >> 1;
}

 /* Create a fence region */
Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}};

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence, Region *p1, Region *p2)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for(axis = 0; axis < 2; axis++){
        if ((shapeBoundary.topLeft.axes[axis] < fence -> topLeft.axes[axis]) ||
            (shapeBoundary.botRight.axes[axis] > fence -> botRight.axes[axis])) {
	  int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	  newPos.axes[axis] += (2*velocity);
        } /* if outside of fence */
    }
    
    /* Checks for collision of the paddle and the ball for player 1 */
    if ((shapeBoundary.topLeft.axes[0] < p1->botRight.axes[0]) && 
       (shapeBoundary.topLeft.axes[0] > p1->topLeft.axes[0])) {
          if((shapeBoundary.botRight.axes[1] >= p1->topLeft.axes[1]) && 
             (shapeBoundary.topLeft.axes[1] <= p1->botRight.axes[1])) {
	       ml->velocity.axes[0] = -ml->velocity.axes[0];
          }
    }
    
    /* Checks for collision of the paddle and the ball for player 2 */
    if ((shapeBoundary.botRight.axes[0] > p2->topLeft.axes[0]) 
        && (shapeBoundary.botRight.axes[0] < p2->botRight.axes[0])) {
            if((shapeBoundary.botRight.axes[1] >= p2->topLeft.axes[1]) 
                && (shapeBoundary.topLeft.axes[1] <= p2->botRight.axes[1])) {
               ml->velocity.axes[0] = -ml->velocity.axes[0];
            }
    }

    /* 
     * If one of the sides of the screen gets hit
     * Make a sound and update score
    */
    if(shapeBoundary.botRight.axes[0] > fence -> botRight.axes[0]){
      buzzer_init(4500);
      player1Scored++;
    }else if(shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]){
      buzzer_init(2000);
      player2Scored++;
    }else{
      buzzer_init(0);
    }
    
    ml->layer->posNext = newPos;
    
  }
}

u_int bgColor = COLOR_PINK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */
Region player1; // Grabs BLACK PADDLE's layer object as a whole
Region player2; // Grabs BLUE PADDLE's layer object as a whole

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0); // Initialize all layer objects 
  layerDraw(&layer0); // Draws all layer objects
  layerGetBounds(&fieldLayer, &fieldFence); 
  layerGetBounds(&layer1, &player1);   // ** Makes layer objects solid and move all together ** //
  layerGetBounds(&layer0, &player2);
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;  /**< Green led off witHo CPU */
      or_sr(0x10);	    /**< CPU OFF */
    }
    P1OUT ^= GREEN_LED;     /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&mlBluP0, &layer0);
  }
}

/* Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;	    /* Green LED on when cpu on */
  count++;

  /*
   * Got help: the unsigned variable 'switches' reads the buttons that have changed.
   * Also keeps track of the current buttons. 
   * The variable 'i' is 
  */
  u_int switches = p2sw_read(), i; 
  char swt[5]; // Keeps track of the bits changing 
  
  if(count == 15) {
  
    mlAdvance(&mlBluP0, &fieldFence, &player1, &player2); // Updates all layers to move

    // Cycles through the Buttons states
    for(i = 0; i < 4; i++) {
      /*
       * Bits are shifted 1 to the left, if 1 or 0,
       * store bit at i
      */
      swt[i] = (switches & (1 << i)) ? 0: 1;  
    }

    // Button 1: Move BLACK PADDLE upward by a certain velocity 
    if(swt[0]) {
        mlBlcP.velocity.axes[1] = -5;
        mlBlcP.velocity.axes[0] = 0;
    }else{ // If Button 1 not pressed, PADDLE does not continue to move
      mlBlcP.velocity.axes[1] = 0;
      mlBlcP.velocity.axes[0] = 0;
    }
    // Button 2: Move BLACK PADDLE downward by a certain velocity
    if(swt[1]) {
       mlBlcP.velocity.axes[1] = 5;
       mlBlcP.velocity.axes[0] = 0;
    } // No else necessary, PADDLE will NOT move downward
    // Button 3: Move BLUE PADDLE upward by a certain velocity
    if(swt[2]){
      mlBluP0.velocity.axes[1] = -5;
      mlBluP0.velocity.axes[0] = 0;
    }else{  // If Button 3 not pressed, PADDLE does not continue to move
      mlBluP0.velocity.axes[1] = 0;
      mlBluP0.velocity.axes[0] = 0;
    }
    // Button 2: Move BLACK PADDLE downward by a certain velocity
    if(swt[3]){
      mlBluP0.velocity.axes[1] = 5;
      mlBluP0.velocity.axes[0] = 0;
    } // No else necessary, PADDLE will NOT move downward

    // If Player 1 wins
    if(player1Scored > '9'){
      drawChar5x7(11,screenHeight-8,'3',COLOR_BLACK,COLOR_PINK);
      mlBluP0.velocity.axes[0] = 0;
      mlBluP0.velocity.axes[1] = 0;
      mlBlcP.velocity.axes[0] = 0;
      mlBlcP.velocity.axes[1] = 0;
      mlPong.velocity.axes[0] = 0;
      mlPong.velocity.axes[1] = 0;
      drawString5x7(14, 74,"|   GAME OVER   |", COLOR_BLACK, COLOR_GREEN);
      drawString5x7(14, 82,"| PLAYER 1 WINS |", COLOR_BLACK, COLOR_GREEN);
    }
    else
    {
      drawChar5x7(11,screenHeight-8,player1Scored,COLOR_BLACK,COLOR_PINK);
    }
   
    // If Player 2 wins
    if(player2Scored > '9')
    {
      drawChar5x7(screenWidth-13,screenHeight-8,'3',COLOR_BLUE,COLOR_PINK);
      mlBluP0.velocity.axes[0] = 0;
      mlBluP0.velocity.axes[1] = 0;
      mlBlcP.velocity.axes[0] = 0;
      mlBlcP.velocity.axes[1] = 0;
      mlPong.velocity.axes[0] = 0;
      mlPong.velocity.axes[1] = 0;
      drawString5x7(14, 74,"|   GAME OVER   |", COLOR_BLUE, COLOR_GREEN);
      drawString5x7(14, 82,"| PLAYER 2 WINS |", COLOR_BLUE, COLOR_GREEN);
    }
    else
    {
       drawChar5x7(screenWidth-13,screenHeight-8,player2Scored,COLOR_BLUE,COLOR_PINK);
    }
    
    if (p2sw_read())
    {
      redrawScreen = 1;
    }
    count = 0;
  }
  // P1OUT &= ~GREEN_LED; /* Green LED off when cpu off */
}
